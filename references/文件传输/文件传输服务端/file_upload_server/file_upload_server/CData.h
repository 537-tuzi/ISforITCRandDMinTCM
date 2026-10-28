#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
#include <map>
#include "protecol.h"
using namespace std;

// 发送数据结构
typedef struct {
    int fd;         // 客户端socket
    int size;       // 数据长度
    char* data;     // 数据内容
} SEND_DATA;

class CData
{
public:
    // 初始化锁和条件变量（程序启动时调用一次）
    static void init();

    // 放入发送队列（线程安全）
    static void push(SEND_DATA* pData);

    // 取出数据（线程安全）
    static SEND_DATA* pop();
    static int pad_fd;
    static map<int,FILE_UPLOAD_REQ> file_fragments;
    static pthread_mutex_t file_fragments_mutex;  
private:
    // 静态成员变量（仅声明）
    static queue<SEND_DATA*> send_queue;
    static pthread_mutex_t m_queueMutex;
    static pthread_cond_t  m_condition;
    
};