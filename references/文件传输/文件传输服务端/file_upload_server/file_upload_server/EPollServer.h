#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H

#include <sys/epoll.h>
#include <cstddef>
#include "CData.h"
#include "BusinessTask.h"
#include "TaskController.h"
#include "ThreadPool.h"
#include "TcpServer.h"
#define MAX_EVENTS 1024

class EPollServer {
public:
    explicit EPollServer(int port);
    //~EPollServer();
    void run();

private:
    static void* send_handler(void* arg);
    void handleClientRead(int fd, ThreadPool& thread_pool);
    void closeConnection(int fd);   // ← 新增：统一关闭连接、清理资源
    TcpServer* server;
    int listen_fd;
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
};

#endif // EPOLLSERVER_H