#ifndef WINMANAGER_H
#define WINMANAGER_H
// WinManager：统一管理窗口的创建/显示/隐藏/跳转（信号槽接线中枢）——单例
// 职责：登录↔主窗切换、导航守卫（建档拦截）、全局建档状态、登入/登出的缩放基线切换；
// 后续接 service 层（QTcpSocket+JSON）时也挂在这里，View 层不感知网络细节。
// 注意：View 不应主动调用 WinManager（那会把 View 耦到管理器），
// 页面跳转一律走信号（cellClicked/navRequested/logoutRequested），由本类接线。
#include <QObject>
#include <QString>
#include "util/PatientTypes.h"

class LoginView;
class MainWindow;

class WinManager : public QObject
{
    Q_OBJECT
public:
    static WinManager* instance();   // 全局唯一实例（父对象 qApp，随应用销毁）

    void start();

private slots:
    void onLoginSucceeded();          // 登录成功：未建档则引导建档
    void navTo(pd::Page page);        // 带建档守卫的页面切换
    void logout();                    // 回登录窗口
    void onProfileSaved(const QString& gm, const QString& jw); // 建档保存：全局状态切换

private:
    WinManager(QObject* parent = nullptr);   // 单例：构造私有
    void buildMain();

    LoginView*  m_login = nullptr;
    MainWindow* m_main  = nullptr;
    bool m_jdDone    = false;
    bool m_mainBuilt = false;
};

#endif // WINMANAGER_H
