#pragma once
#include <string>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protecol.h"
#include "TaskController.h"
#include <iostream>
#include "ThreadPool.h"
using namespace std;

class TcpServer
{
public:
	// 在构造方法中完成服务器的初始化（socket,bind,listen)
	TcpServer(int port);
	void start(); // 启动服务器
	static void* recv_handler(void* arg); // 接收线程
	int getServerFd();
private:
	int port;
	int server_fd;
	
	static ThreadPool* thread_pool;

};

