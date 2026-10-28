#include "EPollServer.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std;

// ========== 日志工具 ==========
static string now_str() {
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto t = chrono::system_clock::to_time_t(now);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    ostringstream oss;
    oss << "[" << put_time(&tm_info, "%H:%M:%S") << "." << setw(3) << setfill('0') << ms.count() << "]";
    return oss.str();
}

#define LOG(msg) do { cout << now_str() << " " << msg << endl; } while(0)
#define LOG_ERR(msg) do { cerr << now_str() << " [ERR] " << msg << endl; } while(0)

// ========== 极简半包缓存：只记录"读了一半的 BODY" ==========
struct HalfBody {
    enum SERVICE_TYPE type;           // 从 HEAD 解析出的类型
    int pos;            // 已读 BODY 字节数
    vector<char> buf;   // BODY 缓冲区
};

static unordered_map<int, HalfBody> g_half;   // fd -> 未读完的包

static void cleanupHalf(int fd) {
    g_half.erase(fd);
}

EPollServer::EPollServer(int port)
{
    LOG("[EPollServer] 构造函数开始，端口=" << port);
    this->server = new TcpServer(port);
    listen_fd = server->getServerFd();
    LOG("[EPollServer] listen_fd=" << listen_fd);

    epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("[EPollServer] epoll_create 失败");
        close(listen_fd);
        exit(1);
    }
    LOG("[EPollServer] epoll_fd=" << epoll_fd);

    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("[EPollServer] epoll_ctl ADD listen_fd 失败");
        close(listen_fd);
        close(epoll_fd);
        exit(1);
    }
    LOG("[EPollServer] listen_fd 已加入 epoll (LT模式)");
}

void EPollServer::run()
{
    LOG("[EPollServer::run] 线程池创建中...");
    ThreadPool thread_pool(10);
    LOG("[EPollServer::run] 线程池创建完成，线程数=10");

    pthread_t tid;
    int ret_pthread = pthread_create(&tid, NULL, send_handler, NULL);
    if (ret_pthread != 0) {
        LOG_ERR("[EPollServer::run] send_handler 线程创建失败: " << ret_pthread);
    }
    else {
        LOG("[EPollServer::run] send_handler 线程创建成功");
    }

    int loop_count = 0;
    while (1) {
        loop_count++;
        LOG("========== [EPollServer] epoll_wait 第 " << loop_count << " 次 ==========");

        int ret = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        LOG("[EPollServer] epoll_wait 返回 ret=" << ret);

        if (ret == -1) {
            perror("[EPollServer] epoll_wait 错误");
            break;
        }
        if (ret == 0) {
            LOG("[EPollServer] epoll_wait 超时（不应该，因为 timeout=-1）");
            continue;
        }

        for (int i = 0; i < ret; i++) {
            int fd = events[i].data.fd;
            uint32_t ev_flags = events[i].events;

            LOG("[EPollServer] 事件[" << i << "] fd=" << fd
                << ", events=0x" << hex << ev_flags << dec);

            if (ev_flags & EPOLLERR) {
                LOG_ERR("[EPollServer] fd=" << fd << " 发生 EPOLLERR 错误");
            }
            if (ev_flags & EPOLLHUP) {
                LOG_ERR("[EPollServer] fd=" << fd << " 发生 EPOLLHUP 挂断");
            }

            if (fd == listen_fd) {
                LOG("[EPollServer] listen_fd 有事件 → 接受新连接");
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("[EPollServer] accept 失败");
                    continue;
                }
                LOG("[EPollServer] 新连接 client_fd=" << client_fd);

                int flag = fcntl(client_fd, F_GETFL, 0);
                if (flag == -1) {
                    perror("[EPollServer] fcntl F_GETFL 失败");
                    close(client_fd);
                    continue;
                }
                if (fcntl(client_fd, F_SETFL, flag | O_NONBLOCK) == -1) {
                    perror("[EPollServer] fcntl F_SETFL 失败");
                    close(client_fd);
                    continue;
                }
                LOG("[EPollServer] client_fd=" << client_fd << " 已设为非阻塞");

                ev.data.fd = client_fd;
                ev.events = EPOLLIN;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("[EPollServer] epoll_ctl ADD client_fd 失败");
                    close(client_fd);
                    continue;
                }
                LOG("[EPollServer] client_fd=" << client_fd << " 已加入 epoll (LT模式)");
            }
            else {
                LOG("[EPollServer] fd=" << fd << " 有数据可读 → 进入读取处理");
                handleClientRead(fd, thread_pool);
            }
        }
    }

    LOG("[EPollServer::run] 主循环退出");
}

// ========== 统一关闭连接 ==========
void EPollServer::closeConnection(int fd)
{
    LOG("  [fd=" << fd << "] 关闭连接");
    close(fd);
    if (CData::pad_fd == fd) {
        CData::pad_fd = 0;
        LOG("  [fd=" << fd << "] pad_fd 已重置为0");
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    cleanupHalf(fd);   // 清理半包缓存
}

// ========== 简化版读取（保留极简半包缓存）==========
void EPollServer::handleClientRead(int fd, ThreadPool& thread_pool)
{
    int packet_count = 0;

    while (true) {
        packet_count++;
        LOG("  [fd=" << fd << "] ===== 第 " << packet_count << " 个包 =====");

        // ---------- 1. 检查是否有上次没读完的 BODY ----------
        auto it = g_half.find(fd);
        if (it == g_half.end()) {
            // 没有半包，先读 HEAD
            char headBuf[sizeof(HEAD)];
            int headPos = 0;

            while (headPos < (int)sizeof(HEAD)) {
                ssize_t n = recv(fd, headBuf + headPos, sizeof(HEAD) - headPos, 0);
                if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        LOG("  [fd=" << fd << "] HEAD EAGAIN，等下次");
                        return;
                    }
                    LOG_ERR("  [fd=" << fd << "] recv HEAD 出错: " << strerror(errno));
                    closeConnection(fd);
                    return;
                }
                if (n == 0) {
                    LOG("  [fd=" << fd << "] 客户端断开连接");
                    closeConnection(fd);
                    return;
                }
                headPos += n;
            }

            HEAD* head = (HEAD*)headBuf;
            LOG("  [fd=" << fd << "] HEAD 完整: type=" << head->type << ", len=" << head->len);

            // 校验
            if (head->len < (int)sizeof(HEAD)) {
                LOG_ERR("  [fd=" << fd << "] 【错误】head.len(" << head->len
                    << ") < sizeof(HEAD)(" << sizeof(HEAD) << ")");
                closeConnection(fd);
                return;
            }
            if (head->len > 65535) {
                LOG_ERR("  [fd=" << fd << "] 【错误】head.len(" << head->len
                    << ") 超过最大限制 65535");
                closeConnection(fd);
                return;
            }

            int bodyLen = head->len - sizeof(HEAD);
            if (bodyLen > 0) {
                // 创建半包记录，下次继续读 BODY
                g_half[fd] = { head->type, 0, vector<char>(bodyLen) };
                it = g_half.find(fd);
            }
            else {
                // 没有 BODY，直接处理（罕见）
                BusinessTask* task = TaskController::create_task(
                    head->type, nullptr, 0, fd
                );
                if (task) thread_pool.submit(task);
                continue;
            }
        }

        // ---------- 2. 读 BODY（新包或半包续传）----------
        HalfBody& hb = it->second;
        int bodyLen = (int)hb.buf.size();

        while (hb.pos < bodyLen) {
            ssize_t n = recv(fd, hb.buf.data() + hb.pos, bodyLen - hb.pos, 0);
            if (n == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    LOG("  [fd=" << fd << "] BODY EAGAIN，已缓存 " << hb.pos << "/" << bodyLen);
                    return;   // 保留在 g_half 中，下次继续
                }
                LOG_ERR("  [fd=" << fd << "] recv BODY 出错: " << strerror(errno));
                closeConnection(fd);
                return;
            }
            if (n == 0) {
                LOG("  [fd=" << fd << "] BODY recv=0，客户端断开");
                closeConnection(fd);
                return;
            }
            hb.pos += n;
        }

        LOG("  [fd=" << fd << "] 完整包收到! type=" << hb.type
            << ", body_size=" << bodyLen);

        // ---------- 3. 创建任务 ----------
        char* taskBody = new (nothrow) char[bodyLen];
        if (!taskBody) {
            LOG_ERR("  [fd=" << fd << "] 【错误】内存分配失败");
            closeConnection(fd);
            return;
        }
        memcpy(taskBody, hb.buf.data(), bodyLen);

        BusinessTask* task = TaskController::create_task(
            hb.type, taskBody, bodyLen, fd
        );

        if (task != nullptr) {
            thread_pool.submit(task);
            size_t qsize = thread_pool.queueSize();
            LOG("  [fd=" << fd << "] 任务提交成功，线程池队列=" << qsize);
        }
        else {
            LOG_ERR("  [fd=" << fd << "] 【错误】create_task 返回 nullptr");
            delete[] taskBody;
        }

        // 包处理完毕，从半包表中移除，继续读下一个包
        g_half.erase(it);
        LOG("  [fd=" << fd << "] 包处理完毕，准备读下一个...");
    }
}

void* EPollServer::send_handler(void* arg)
{
    LOG("[send_handler] 发送线程启动");
    int send_count = 0;
    while (1) {
        SEND_DATA* sd = CData::pop();
        if (sd) {
            send_count++;
            LOG("[send_handler] 第 " << send_count << " 次发送，fd=" << sd->fd
                << ", size=" << sd->size);

            ssize_t sent = 0;
            ssize_t total_sent = 0;
            while (total_sent < sd->size) {
                sent = send(sd->fd, sd->data + total_sent, sd->size - total_sent, 0);
                if (sent == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        LOG("[send_handler] send EAGAIN，稍后再试");
                        usleep(1000);
                        continue;
                    }
                    LOG_ERR("[send_handler] send 失败: " << strerror(errno));
                    break;
                }
                total_sent += sent;
                LOG("[send_handler] 已发送 " << total_sent << "/" << sd->size << " 字节");
            }

            delete[] sd->data;
            delete sd;
            LOG("[send_handler] 发送完成，释放内存");
        }
        else {
            usleep(1000);
        }
    }
    return nullptr;
}