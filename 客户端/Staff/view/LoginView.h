#ifndef STAFFLOGINVIEW_H
#define STAFFLOGINVIEW_H
// 工作人员登录窗（手册 3.4.1/3.5.1：工号+密码；连错3次锁24小时；修改密码三框）
#include <QWidget>
#include <QtGlobal>

class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTimer;

class LoginView : public QWidget
{
    Q_OBJECT
public:
    explicit LoginView(QWidget* parent = nullptr);

signals:
    void loginSucceeded(int role, const QString& workNo, const QString& name);  // 1医生 2管理员

private slots:
    void doLogin();
    void openChangePwd();         // 修改密码（原/新/确认 三框，手册 3.4.1）
    void tickLockCountdown();     // 24 小时锁定倒计时（手册 3.2.4）
    void resetFields();

private:
    QLineEdit* m_no   = nullptr;
    QLineEdit* m_pw   = nullptr;
    QPushButton* m_btn = nullptr;    // 登录按钮（锁定期间禁用+倒计时）
    QTimer* m_lockCd = nullptr;
    qint64 m_lockUntilSec = 0;       // 原型内存态；正式版走 staff.lock_until
    int m_errCount = 0;
};

#endif // STAFFLOGINVIEW_H
