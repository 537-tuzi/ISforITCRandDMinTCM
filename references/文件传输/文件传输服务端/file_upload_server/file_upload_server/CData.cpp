#include "CData.h"

// ==============================
// 静态变量必须在这里定义！！！
// ==============================
queue<SEND_DATA*> CData::send_queue;
pthread_mutex_t CData::m_queueMutex;
pthread_cond_t  CData::m_condition;
int CData::pad_fd = 0;
map<int, FILE_UPLOAD_REQ> CData::file_fragments;
// 文件分片专用互斥锁 静态初始化
pthread_mutex_t CData::file_fragments_mutex = PTHREAD_MUTEX_INITIALIZER;

void CData::init()
{
    // 初始化互斥锁 + 条件变量
    pthread_mutex_init(&m_queueMutex, nullptr);
    pthread_cond_init(&m_condition, nullptr);
}

void CData::push(SEND_DATA* pData)
{
    if (!pData) return;

    // 加锁
    pthread_mutex_lock(&m_queueMutex);

    // 入队
    send_queue.push(pData);

    // 通知消费者
    pthread_cond_signal(&m_condition);

    // 解锁
    pthread_mutex_unlock(&m_queueMutex);
}

SEND_DATA* CData::pop()
{
    pthread_mutex_lock(&m_queueMutex);

    // 等待队列不为空
    while (send_queue.empty()) {
        pthread_cond_wait(&m_condition, &m_queueMutex);
    }

    // 取数据
    SEND_DATA* pData = send_queue.front();
    send_queue.pop();

    pthread_mutex_unlock(&m_queueMutex);

    return pData;
}