#pragma once
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include "protecol.h"
#include "CData.h"
#include "string.h"

using namespace std;

class BusinessTask
{
public:
	BusinessTask(void* data, int size, int fd);
	virtual ~BusinessTask();
	virtual void execute() = 0;
protected:
	void* m_data;
	int m_fd;
	void* m_respData; // 响应数据
	int m_respSize; // 响应数据长度
	int m_size;
};

