#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount) {
    m_stop = false;
    if (threadCount > 0) {
        this->m_threadCount = threadCount;
        pthread_mutex_init(&m_queueMutex, nullptr);
        pthread_cond_init(&m_condition, nullptr);
        m_workers.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; i++) {
            int ret = pthread_create(&m_workers[i], nullptr, workerThread, this);
            if (ret != 0) {
                cerr << "[ThreadPool] pthread_create error: " << ret << endl;
            }
            else {
                cout << "[ThreadPool] 工作线程 " << i << " 创建成功" << endl;
            }
        }
    }
    cout << "[ThreadPool] 线程池初始化完成，线程数=" << m_threadCount << endl;
}

ThreadPool::~ThreadPool() {
    cout << "[ThreadPool] 析构开始，设置停止标志..." << endl;
    pthread_mutex_lock(&m_queueMutex);
    m_stop = true;
    pthread_cond_broadcast(&m_condition);
    pthread_mutex_unlock(&m_queueMutex);

    cout << "[ThreadPool] 等待所有工作线程退出..." << endl;
    for (size_t i = 0; i < m_workers.size(); i++) {
        pthread_join(m_workers[i], nullptr);
        cout << "[ThreadPool] 工作线程 " << i << " 已退出" << endl;
    }

    pthread_mutex_lock(&m_queueMutex);
    int leaked = 0;
    while (!m_tasks.empty()) {
        delete m_tasks.front();
        m_tasks.pop();
        leaked++;
    }
    pthread_mutex_unlock(&m_queueMutex);
    if (leaked > 0) {
        cout << "[ThreadPool] 清理了 " << leaked << " 个未执行的任务" << endl;
    }

    pthread_mutex_destroy(&m_queueMutex);
    pthread_cond_destroy(&m_condition);
    cout << "[ThreadPool] 析构完成" << endl;
}

void ThreadPool::submit(BusinessTask* task) {
    if (!task) {
        cout << "[ThreadPool] submit 收到 nullptr，忽略" << endl;
        return;
    }

    pthread_mutex_lock(&m_queueMutex);
    if (m_stop) {
        pthread_mutex_unlock(&m_queueMutex);
        cout << "[ThreadPool] 已停止，拒绝新任务" << endl;
        delete task;
        return;
    }
    m_tasks.push(task);
    size_t qsize = m_tasks.size();
    pthread_cond_signal(&m_condition);
    pthread_mutex_unlock(&m_queueMutex);

    cout << "[ThreadPool] 任务已提交，当前队列大小=" << qsize << endl;
}

size_t ThreadPool::queueSize() {
    pthread_mutex_lock(&m_queueMutex);
    size_t size = m_tasks.size();
    pthread_mutex_unlock(&m_queueMutex);
    return size;
}

void* ThreadPool::workerThread(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    pthread_t self = pthread_self();
    cout << "[ThreadPool] 工作线程启动，tid=" << self << endl;

    int task_count = 0;
    while (true) {
        BusinessTask* task = nullptr;

        pthread_mutex_lock(&pool->m_queueMutex);

        while (pool->m_tasks.empty() && !pool->m_stop) {
            pthread_cond_wait(&pool->m_condition, &pool->m_queueMutex);
        }

        if (pool->m_stop && pool->m_tasks.empty()) {
            pthread_mutex_unlock(&pool->m_queueMutex);
            cout << "[ThreadPool] 工作线程 tid=" << self
                << " 收到停止信号，共执行 " << task_count << " 个任务，准备退出" << endl;
            break;
        }

        if (!pool->m_tasks.empty()) {
            task = pool->m_tasks.front();
            pool->m_tasks.pop();
        }
        pthread_mutex_unlock(&pool->m_queueMutex);

        if (task) {
            task_count++;
            cout << "[ThreadPool] 工作线程 tid=" << self
                << " 开始执行第 " << task_count << " 个任务" << endl;
            try {
                task->execute();
                cout << "[ThreadPool] 工作线程 tid=" << self
                    << " 任务执行完成" << endl;
            }
            catch (const std::exception& e) {
                cerr << "[ThreadPool] 任务异常: " << e.what() << endl;
            }
            catch (...) {
                cerr << "[ThreadPool] 任务未知异常" << endl;
            }
            delete task;
        }
    }

    return nullptr;
}