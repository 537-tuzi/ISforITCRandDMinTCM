#include "TcpServer.h"
#include "EPollServer.h"
int main() {
	/*TcpServer server(10086);
	server.start();*/
	CData::init();
	EPollServer server(10086);
	server.run();
}