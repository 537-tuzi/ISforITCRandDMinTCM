#include "FileUploadTask.h"
#include "CData.h"
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <atomic>

// 全局文件日志锁定义
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int log_fd = -1;

// ========== 新增：merge 状态标记，防止重复合并 ==========
std::atomic<bool> g_merge_in_progress(false);

void log_to_file(const std::string& msg) {
    pthread_mutex_lock(&log_mutex);
    if (log_fd == -1) {
        log_fd = open("/tmp/server_debug.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    if (log_fd != -1) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

        std::string line = std::string(time_str) + " " + msg + "\n";
        write(log_fd, line.c_str(), line.length());
        fsync(log_fd);
    }
    pthread_mutex_unlock(&log_mutex);
}

FileUploadTask::FileUploadTask(void* data, int size, int fd) : BusinessTask(data, size, fd) {
}

FileUploadTask::~FileUploadTask() {
}

void FileUploadTask::execute() {
    log_to_file("[FileUploadTask] execute 开始! fd=" + std::to_string(this->m_fd)
        + ", size=" + std::to_string(this->m_size));

    if (this->m_size < 268) {
        log_to_file("[FileUploadTask] 错误: m_size=" + std::to_string(this->m_size)
            + " < 268 (最小结构体大小)");
        return;
    }

    FILE_UPLOAD_REQ file_upload_req = { 0 };
    memcpy(&file_upload_req, this->m_data, this->m_size);

    file_upload_req.filename[sizeof(file_upload_req.filename) - 1] = '\\0';

    log_to_file("[FileUploadTask] filename=[" + std::string(file_upload_req.filename)
        + "], chunkIndex=" + std::to_string(file_upload_req.chunkIndex)
        + ", chunkTotal=" + std::to_string(file_upload_req.chunkTotal)
        + ", chunkSize=" + std::to_string(file_upload_req.chunkSize));

    // 校验
    if (file_upload_req.chunkTotal <= 0) {
        log_to_file("[FileUploadTask] 错误: chunkTotal=" + std::to_string(file_upload_req.chunkTotal) + " <= 0");
        return;
    }
    if (file_upload_req.chunkIndex < 0 || file_upload_req.chunkIndex >= file_upload_req.chunkTotal) {
        log_to_file("[FileUploadTask] 错误: chunkIndex=" + std::to_string(file_upload_req.chunkIndex)
            + " 超出范围 [0, " + std::to_string(file_upload_req.chunkTotal) + ")");
        return;
    }

    int maxDataSize = this->m_size - 268;
    if (file_upload_req.chunkSize > maxDataSize) {
        log_to_file("[FileUploadTask] 调整 chunkSize: " + std::to_string(file_upload_req.chunkSize)
            + " -> " + std::to_string(maxDataSize));
        file_upload_req.chunkSize = maxDataSize;
    }
    if (file_upload_req.chunkSize < 0 || file_upload_req.chunkSize > 10240) {
        log_to_file("[FileUploadTask] 错误: chunkSize=" + std::to_string(file_upload_req.chunkSize));
        return;
    }

    // ========== 关键修复：所有操作在锁内完成，包括判断和 merge 触发 ==========
    bool should_merge = false;
    size_t current_size = 0;

    pthread_mutex_lock(&CData::file_fragments_mutex);

    // 检查是否已存在该 chunkIndex（重复碎片）
    auto existing = CData::file_fragments.find(file_upload_req.chunkIndex);
    if (existing != CData::file_fragments.end()) {
        log_to_file("[FileUploadTask] 忽略重复的 chunkIndex=" + std::to_string(file_upload_req.chunkIndex));
        pthread_mutex_unlock(&CData::file_fragments_mutex);
        log_to_file("[FileUploadTask] execute 结束(重复碎片)");
        return;
    }

    // 统一保存碎片（chunkIndex=0 不再特殊处理）
    CData::file_fragments[file_upload_req.chunkIndex] = file_upload_req;
    current_size = CData::file_fragments.size();

    // 安全检查：碎片数不能超过总数
    if (current_size > (size_t)file_upload_req.chunkTotal) {
        log_to_file("[FileUploadTask] 错误: 碎片数 " + std::to_string(current_size)
            + " 超过总数 " + std::to_string(file_upload_req.chunkTotal));
        pthread_mutex_unlock(&CData::file_fragments_mutex);
        return;
    }

    // 判断是否满足合并条件（在锁内判断！）
    should_merge = (current_size == (size_t)file_upload_req.chunkTotal);

    pthread_mutex_unlock(&CData::file_fragments_mutex);

    // 打印日志（锁外）
    log_to_file("[FileUploadTask] 第" + std::to_string(file_upload_req.chunkIndex + 1)
        + "块碎片已保存，当前碎片数=" + std::to_string(current_size)
        + "/" + std::to_string(file_upload_req.chunkTotal));

    // 触发合并（只有一个线程能成功）
    if (should_merge) {
        mergeFile(file_upload_req.filename, file_upload_req.chunkTotal);
    }

    log_to_file("[FileUploadTask] execute 结束");
}

void FileUploadTask::mergeFile(const char* filename, int chunkTotal) {
    // ========== 关键修复：原子标记，防止多个线程同时 merge ==========
    bool expected = false;
    if (!g_merge_in_progress.compare_exchange_strong(expected, true)) {
        log_to_file("[mergeFile] 另一个线程正在合并，跳过");
        return;
    }

    log_to_file("[mergeFile] 开始合并文件：" + std::string(filename) + ", 碎片数=" + std::to_string(chunkTotal));

    const char* outputDir = "./uploaded_files";
    mkdir(outputDir, 0755);

    std::string outputPath = std::string(outputDir) + "/" + filename;

    // ========== 关键修复：在锁内复制数据，然后立即清空 map ==========
    std::map<int, FILE_UPLOAD_REQ> fragments_copy;
    int64_t totalBytes = 0;

    pthread_mutex_lock(&CData::file_fragments_mutex);

    // 双重检查：可能其他线程已经处理过了
    if (CData::file_fragments.empty()) {
        pthread_mutex_unlock(&CData::file_fragments_mutex);
        g_merge_in_progress.store(false);
        log_to_file("[mergeFile] file_fragments 已为空，跳过");
        return;
    }

    // 再次确认碎片数（防止刚好有重复插入导致）
    if (CData::file_fragments.size() != (size_t)chunkTotal) {
        size_t actual_size = CData::file_fragments.size();
        pthread_mutex_unlock(&CData::file_fragments_mutex);
        g_merge_in_progress.store(false);
        log_to_file("[mergeFile] 碎片数不匹配: 实际=" + std::to_string(actual_size)
            + ", 期望=" + std::to_string(chunkTotal) + ", 跳过");
        return;
    }

    // 复制数据到本地
    fragments_copy = CData::file_fragments;

    // 立即清空 map，让后续碎片（如果有）能开始新的收集
    CData::file_fragments.clear();

    pthread_mutex_unlock(&CData::file_fragments_mutex);

    log_to_file("[mergeFile] 已复制 " + std::to_string(fragments_copy.size()) + " 个碎片，准备写入文件");

    // 写入文件（不需要锁，数据已复制到本地）
    std::ofstream outFile(outputPath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        log_to_file("[mergeFile] 错误：无法创建文件 " + outputPath);
        g_merge_in_progress.store(false);
        return;
    }

    for (int i = 0; i < chunkTotal; i++) {
        auto it = fragments_copy.find(i);
        if (it == fragments_copy.end()) {
            log_to_file("[mergeFile] 错误：找不到碎片 " + std::to_string(i));
            outFile.close();
            g_merge_in_progress.store(false);
            return;
        }

        FILE_UPLOAD_REQ& req = it->second;
        int dataSize = req.chunkSize;
        if (dataSize > 10240) dataSize = 10240;
        if (dataSize < 0) dataSize = 0;

        outFile.write(req.chunkData, dataSize);
        totalBytes += dataSize;

        log_to_file("[mergeFile] 写入碎片 " + std::to_string(i)
            + ", chunkSize=" + std::to_string(dataSize)
            + ", 累计=" + std::to_string(totalBytes) + "字节");
    }

    outFile.close();
    log_to_file("[mergeFile] 文件合并完成：" + outputPath
        + ", 总字节数=" + std::to_string(totalBytes));

    // 释放 merge 标记
    g_merge_in_progress.store(false);
    log_to_file("[mergeFile] 已释放 merge 标记");
}