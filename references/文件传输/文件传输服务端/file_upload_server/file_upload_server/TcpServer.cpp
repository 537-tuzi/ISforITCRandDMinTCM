#include "TcpServer.h"
ThreadPool* TcpServer::thread_pool = new ThreadPool(10);
TcpServer::TcpServer(int port)
{
	this->port = port;
	int result = 0;
	this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0) {
		perror("socket error");
		return;
	}

	// ========== 新增端口重用代码 ==========
	//int opt = 1;
	//setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//// 可选：同时开启端口+ip全复用 SO_REUSEPORT
	//setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	struct  sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY; // 让系统分配
	address.sin_port = htons(port);
	result = bind(server_fd, (struct  sockaddr*)&address, sizeof(address));
	if (result < 0) {
		perror("bind error");
		return;
	}
	result = listen(server_fd, 5);
	if (result < 0) {
		perror("listen error");
		return;
	}
	cout << "【服务器】已成功启动，监听端口：" << port << endl;
}

void TcpServer::start()
{
	int client_fd = -1;
	while (1) {
		struct sockaddr_in client_address;
		int addlen = sizeof(client_address);
		client_fd = accept(server_fd, (struct sockaddr*)&client_address, (socklen_t*)&addlen);
		if (client_fd < 0) {
			perror("accept error");
		}
		else
		{
			cout << "【服务器】有一个新的客户端连接，客户端IP: " << inet_ntoa(client_address.sin_addr)
				<< ", 端口号是：" << ntohs(client_address.sin_port) << endl;
			pthread_t tid = 0;
			pthread_create(&tid, NULL, TcpServer::recv_handler, &client_fd);
		}
	}
	close(server_fd);
	cout << "【服务器】已退出" << endl;
}

void* TcpServer::recv_handler(void* arg)
{
	int fd = *(int*)arg;
	pthread_t tid = pthread_self();
	pthread_detach(tid);
	char buffer[1024] = { 0 };
	char send_buf[1024] = { 0 };
	while (1) {
		memset(buffer, 0, sizeof(buffer));
		HEAD head;
		// 先读头，再读体
		ssize_t len = recv(fd, &head, sizeof(HEAD), 0);
		if (len <= 0)
			break;
		len = recv(fd, buffer, head.len, 0);
		if (len <= 0)
			break;
		BusinessTask* task = TaskController::create_task(head.type,buffer,head.len,fd);
		if (task != nullptr) {
			TcpServer::thread_pool->submit(task);
			/*task->execute();
			delete task;*/
		}
	}
}

int TcpServer::getServerFd()
{
	return this->server_fd;
}
