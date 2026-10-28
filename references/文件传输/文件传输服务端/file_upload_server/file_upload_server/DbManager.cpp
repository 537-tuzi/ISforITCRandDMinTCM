//#include "DbManager.h"
//
//DbManager::DbManager() {
//    cout << "DbManager被创建" << endl;
//}
//
//DbManager& DbManager::getInstance()
//{
//    static DbManager instance;
//    return instance;
//}
//
//Connection* DbManager::get_connection()
//{
//	Connection* conn = nullptr;
//	bool result;
//	try {
//		Driver* driver = get_driver_instance();
//
//		conn = driver->connect(DB_HOST+" ?sslMode=DISABLED", DB_USER, DB_PASS);
//		if (conn) {
//			conn->setSchema(DB_NAME);
//			//cout << "数据库连接成功" << endl;
//			return conn;
//		}
//		else
//		{
//			cerr << "driver->connect 返回了空指针" << endl;
//			return nullptr;
//		}
//		
//	}
//	catch (SQLException& e) {
//		cerr << "数据库连接失败：" << e.what() << endl;
//		return nullptr;
//	}
//	
//}
//
//void DbManager::close_connection(Connection* conn, PreparedStatement* pstmt, ResultSet* res)
//{
//	if (res != nullptr)
//		delete res;
//	if (pstmt != nullptr)
//		delete pstmt;
//	if (conn != nullptr) {
//		conn->close();
//		delete conn;
//	}
//		
//}


#include "DbManager.h"
#include <iostream>

DbManager::DbManager()
    : m_running(true)
{
    // 预先创建部分连接（预热）
    for (int i = 0; i < 5; ++i) {
        Connection* conn = create_connection();
        if (conn) {
            m_pool.push(conn);
        }
    }
    cout << "DbManager连接池初始化完成，预热连接数：" << m_pool.size() << endl;
}

DbManager::~DbManager()
{
    shutdown();
}

DbManager& DbManager::getInstance()
{
    static DbManager instance;
    return instance;
}

Connection* DbManager::create_connection()
{
    try {
        Driver* driver = get_driver_instance();
        string url = DB_HOST +
            "?sslMode=DISABLED"
            "&connectTimeout=3000"
            "&socketTimeout=5000";
        Connection* conn = driver->connect(url, DB_USER, DB_PASS);
        if (conn) {
            conn->setSchema(DB_NAME);
        }
        return conn;
    }
    catch (SQLException& e) {
        cerr << "创建数据库连接失败：" << e.what() << endl;
        return nullptr;
    }
}

Connection* DbManager::get_connection()
{
    unique_lock<mutex> lock(m_mtx);

    // 方案B：不使用 lambda，直接用变量判断，彻底规避编译器解析Bug
    bool connAvailable = false;
    // 计时开始
    auto start = chrono::steady_clock::now();

    // 循环等待
    while (chrono::steady_clock::now() - start < chrono::seconds(3)) {
        if (!m_pool.empty() || !m_running.load()) {
            connAvailable = true;
            break;
        }
        // 释放锁并等待，直到被通知或超时
        m_cv.wait_for(lock, chrono::milliseconds(100));
    }

    if (!connAvailable) {
        cerr << "获取数据库连接超时（连接池已满）" << endl;
        return nullptr;
    }

    if (!m_running.load()) {
        cerr << "连接池已关闭，无法获取连接" << endl;
        return nullptr;
    }

    // 从队列取出连接
    Connection* conn = m_pool.front();
    m_pool.pop();

    // 检查连接是否有效
    if (!conn->isValid()) {
        cerr << "连接无效，重新创建连接" << endl;
        delete conn;
        conn = create_connection();
    }

    // 动态扩容逻辑
    if (m_pool.empty() && m_max_pool_size > 0) {
        int current_total = 1 + m_pool.size(); // 1 是刚取出的连接
        if (current_total < m_max_pool_size) {
            Connection* new_conn = create_connection();
            if (new_conn) {
                m_pool.push(new_conn);
            }
        }
    }

    return conn;
}

void DbManager::return_connection(Connection* conn)
{
    if (!conn || !m_running.load()) {
        if (conn) {
            conn->close();
            delete conn;
        }
        return;
    }

    unique_lock<mutex> lock(m_mtx);
    // 归还连接到队列
    m_pool.push(conn);
    lock.unlock();
    m_cv.notify_one(); // 通知等待的线程有连接可用
}

void DbManager::safe_close(PreparedStatement* pstmt, ResultSet* res)
{
    if (res != nullptr) {
        delete res;
    }
    if (pstmt != nullptr) {
        delete pstmt;
    }
}

void DbManager::shutdown()
{
    m_running = false;
    m_cv.notify_all();

    unique_lock<mutex> lock(m_mtx);
    while (!m_pool.empty()) {
        Connection* conn = m_pool.front();
        m_pool.pop();
        conn->close();
        delete conn;
    }
}