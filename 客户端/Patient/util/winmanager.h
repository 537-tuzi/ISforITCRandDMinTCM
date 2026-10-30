#ifndef WINMANAGER_H
#define WINMANAGER_H
// WinManager：统一管理窗口的创建/显示/隐藏/跳转（信号槽接线中枢）
// 原型阶段无真实数据流，业务演示逻辑全部在 view 层内部
#include <QObject>
#include <QString>
#include "util/PatientTypes.h"

class LoginView;
class MainWindow;

class WinManager : public QObject
{
    Q_OBJECT
public:
    explicit WinManager(QObject* parent = nullptr);
    void start();

private slots:
    void onLoginSucceeded();          // 登录成功：未建档则引导建档
    void navTo(pd::Page page);        // 带建档守卫的页面切换
    void logout();                    // 回登录窗口
    void onProfileSaved(const QString& gm, const QString& jw); // 建档保存：全局状态切换

private:
    void buildMain();

    LoginView*  m_login = nullptr;
    MainWindow* m_main  = nullptr;
    bool m_jdDone    = false;
    bool m_mainBuilt = false;
};

#endif
