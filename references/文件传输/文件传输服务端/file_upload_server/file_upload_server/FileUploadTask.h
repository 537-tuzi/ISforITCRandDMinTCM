#pragma once
#include "BusinessTask.h"
#include <string>

// 全局文件日志锁（外部声明）
extern pthread_mutex_t log_mutex;
extern int log_fd;

// 文件日志函数
void log_to_file(const std::string& msg);

class FileUploadTask : public BusinessTask
{
public:
    FileUploadTask(void* data, int size, int fd);
    ~FileUploadTask();

    // 执行任务
    void execute() override;

private:
    // 合并文件
    void mergeFile(const char* filename, int chunkTotal);
};