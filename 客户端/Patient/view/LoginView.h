#ifndef LOGINVIEW_H
#define LOGINVIEW_H
// 登录窗口（自包含演示逻辑：两种方式/倒计时/错3次锁定/未建档引导，对应 MD §3）
#include <QWidget>

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
    void loginSucceeded();       // 校验通过（WinManager 决定是否引导建档）

private slots:
    void doYbLogin();
    void doSmsLogin();
    void requestSms();           // 获取验证码：倒计时 + toast + 播报
    void speakCode();            // 念给我听
    void switchTab(bool yb);
    void tickCountdown();
    void tickLockCountdown();    // 医保锁定倒计时（15 分钟禁登，手册 3.3.1）
    void resetFields();          // 锁定后清空输入框

private:
    QLineEdit* m_ybNo  = nullptr;
    QLineEdit* m_ybPw  = nullptr;
    QLineEdit* m_tel   = nullptr;
    QLineEdit* m_code  = nullptr;
    QPushButton* m_tabYb  = nullptr;
    QPushButton* m_tabSms = nullptr;
    QPushButton* m_smsBtn = nullptr;
    QPushButton* m_ybBtn  = nullptr;   // 医保登录按钮（锁定期间禁用+倒计时）
    QStackedWidget* m_panes = nullptr;
    QTimer* m_cd = nullptr;
    QTimer* m_lockCd = nullptr;  // 医保锁定倒计时器
    qint64 m_lockUntilSec = 0;   // 锁定截止（epoch 秒；原型内存态，正式版走库 lock_until）
    int m_sec = 0;
    int m_errCount = 0;          // 手册 3.3.1：连续 3 次错误锁定
};

#endif
