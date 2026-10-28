#pragma once
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
using namespace std;
using namespace sql;

//class DbManager
//{
//public:
//	// 禁止拷贝构造和赋值操作
//	DbManager(const DbManager&) = delete;
//	DbManager& operator=(const DbManager&) = delete;
//	// 获取单例实例
//	static DbManager& getInstance();
//	Connection* get_connection();
//	void close_connection(Connection* conn, PreparedStatement* pstmt, ResultSet* res);
//private:
//	DbManager();
//	//~DbManager() = delete;  // 防止外部delete
//	// MySQL连接参数
//	const string DB_HOST = "tcp://127.0.0.1:3306";
//	const string DB_USER = "root";
//	const string DB_PASS = "123456";
//	const string DB_NAME = "test_db";
//};
class DbManager
{
public:
    // 禁止拷贝构造和赋值操作
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    // 获取单例实例
    static DbManager& getInstance();

    // 从连接池获取一个可用连接
    Connection* get_connection();

    // 将连接归还到连接池（注意：不要 close，只是放回队列）
    void return_connection(Connection* conn);

    // 安全释放结果集、预处理语句（连接由池管理）
    void safe_close(PreparedStatement* pstmt, ResultSet* res);

    // 关闭连接池（程序退出时调用）
    void shutdown();

private:
    DbManager();
    ~DbManager();

    // 创建一个新连接（内部用）
    Connection* create_connection();

    // 连接池配置
    const string DB_HOST = "tcp://127.0.0.1:3306";
    const string DB_USER = "root";
    const string DB_PASS = "123456";
    const string DB_NAME = "test_db";

    // 连接池队列
    queue<Connection*> m_pool;
    mutex m_mtx;
    condition_variable m_cv;
    atomic<bool> m_running;
    const int m_max_pool_size = 100; // 最大连接数，不要超过 MySQL 的 max_connections
};
