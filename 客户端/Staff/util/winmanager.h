#ifndef STAFFWINMANAGER_H
#define STAFFWINMANAGER_H
// 工作人员端窗口管理器（单例）：登录→按角色分流主窗口；View 不直接调用本类
#include <QObject>
#include <QString>

class LoginView;
class MainWindow;

class WinManager : public QObject
{
    Q_OBJECT
public:
    static WinManager* instance();

    void start();

private slots:
    void onLoginSucceeded(int role, const QString& workNo, const QString& name);
    void logout();

private:
    WinManager(QObject* parent = nullptr);
    void buildMain(int role, const QString& workNo, const QString& name);

    LoginView*  m_login = nullptr;
    MainWindow* m_main  = nullptr;
    bool m_mainBuilt = false;
};

#endif // STAFFWINMANAGER_H
