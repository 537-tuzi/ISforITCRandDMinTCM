#include "LoginView.h"
#include "util/Toast.h"
#include "util/Speak.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

LoginView::LoginView(QWidget* parent) : QWidget(parent)
{
    setObjectName("authPanel");
    setFixedSize(480, 640);    // 窗口比例 3:4（2026-10-30 决定），卡片贴边距填满

    QWidget* box = new QWidget;
    box->setObjectName("authBox");
    box->setFixedSize(448, 608);

    QVBoxLayout* b = new QVBoxLayout(box);
    b->setContentsMargins(48, 36, 48, 20);
    b->setSpacing(0);

    QLabel* title = new QLabel(QStringLiteral("中医舌苔智能识别系统"));
    title->setObjectName("authTitle");
    title->setAlignment(Qt::AlignCenter);
    b->addWidget(title);
    b->addSpacing(24);

    QWidget* tabs = new QWidget;
    QHBoxLayout* th = new QHBoxLayout(tabs);
    th->setContentsMargins(0, 0, 0, 0);
    th->setSpacing(24);
    m_tabYb  = new QPushButton(QStringLiteral("医保账号登录"));
    m_tabSms = new QPushButton(QStringLiteral("手机验证码登录"));
    m_tabYb->setObjectName("loginTab");
    m_tabSms->setObjectName("loginTab");
    m_tabYb->setCheckable(true); m_tabSms->setCheckable(true);
    m_tabYb->setChecked(true);
    m_tabYb->setCursor(Qt::PointingHandCursor);
    m_tabSms->setCursor(Qt::PointingHandCursor);
    th->addWidget(m_tabYb);
    th->addWidget(m_tabSms);
    th->addStretch();
    b->addWidget(tabs);
    b->addSpacing(20);

    m_panes = new QStackedWidget;
    b->addWidget(m_panes, 1);

    // ---- 医保账号页 ----
    QWidget* py = new QWidget;
    QVBoxLayout* pyb = new QVBoxLayout(py);
    pyb->setContentsMargins(0, 0, 0, 0);
    pyb->setSpacing(14);

    QLabel* l1 = new QLabel(QStringLiteral("医保账号"));
    l1->setObjectName("fldLabel");
    m_ybNo = new QLineEdit;
    m_ybNo->setPlaceholderText(QStringLiteral("请输入医保卡号"));
    pyb->addWidget(l1); pyb->addWidget(m_ybNo);

    QLabel* l2 = new QLabel(QStringLiteral("医保密码"));
    l2->setObjectName("fldLabel");
    m_ybPw = new QLineEdit;
    m_ybPw->setEchoMode(QLineEdit::Password);
    m_ybPw->setPlaceholderText(QStringLiteral("请输入密码"));
    pyb->addWidget(l2); pyb->addWidget(m_ybPw);
    QLabel* t1 = new QLabel(QStringLiteral("演示密码：123456"));
    t1->setObjectName("tip");
    pyb->addWidget(t1);
    pyb->addSpacing(6);

    QPushButton* bYb = new QPushButton(QStringLiteral("登 录"));
    bYb->setObjectName("btnPriLg");
    bYb->setCursor(Qt::PointingHandCursor);
    connect(bYb, &QPushButton::clicked, this, &LoginView::doYbLogin);
    pyb->addWidget(bYb);
    m_ybBtn = bYb;
    pyb->addStretch();
    m_panes->addWidget(py);

    // ---- 验证码页 ----
    QWidget* ps = new QWidget;
    QVBoxLayout* psb = new QVBoxLayout(ps);
    psb->setContentsMargins(0, 0, 0, 0);
    psb->setSpacing(14);

    QLabel* l3 = new QLabel(QStringLiteral("手机号"));
    l3->setObjectName("fldLabel");
    m_tel = new QLineEdit;
    m_tel->setPlaceholderText(QStringLiteral("请输入注册手机号"));
    psb->addWidget(l3); psb->addWidget(m_tel);

    QLabel* l4 = new QLabel(QStringLiteral("短信验证码"));
    l4->setObjectName("fldLabel");
    QWidget* rw = new QWidget;
    QHBoxLayout* rh = new QHBoxLayout(rw);
    rh->setContentsMargins(0, 0, 0, 0);
    rh->setSpacing(8);
    m_code = new QLineEdit;
    m_code->setPlaceholderText(QStringLiteral("请输入验证码"));
    m_smsBtn = new QPushButton(QStringLiteral("获取验证码"));
    m_smsBtn->setObjectName("btnOut");
    m_smsBtn->setCursor(Qt::PointingHandCursor);
    QPushButton* say = new QPushButton(QStringLiteral("念给我听"));
    say->setObjectName("btnOut");
    say->setToolTip(QStringLiteral("看不清？念给您听"));
    say->setCursor(Qt::PointingHandCursor);
    rh->addWidget(m_code, 1);
    rh->addWidget(m_smsBtn);
    rh->addWidget(say);
    psb->addWidget(l4); psb->addWidget(rw);
    QLabel* t2 = new QLabel(QStringLiteral("演示验证码：123456"));
    t2->setObjectName("tip");
    psb->addWidget(t2);
    psb->addSpacing(6);

    QPushButton* bSms = new QPushButton(QStringLiteral("登 录"));
    bSms->setObjectName("btnPriLg");
    bSms->setCursor(Qt::PointingHandCursor);
    connect(bSms, &QPushButton::clicked, this, &LoginView::doSmsLogin);
    psb->addWidget(bSms);
    psb->addStretch();
    m_panes->addWidget(ps);

    QLabel* hint = new QLabel(QStringLiteral("密码连错 3 次将临时锁定医保登录 15 分钟\n"
                                             "没建过档的用户，登录后会自动引导到「初次建档」"));
    hint->setObjectName("hint");
    hint->setAlignment(Qt::AlignCenter);
    b->addSpacing(18);
    b->addWidget(hint);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->addWidget(box);

    m_cd = new QTimer(this);
    connect(m_cd, &QTimer::timeout, this, &LoginView::tickCountdown);
    m_lockCd = new QTimer(this);
    connect(m_lockCd, &QTimer::timeout, this, &LoginView::tickLockCountdown);

    connect(m_tabYb,  &QPushButton::clicked, this, [this] { switchTab(true); });
    connect(m_tabSms, &QPushButton::clicked, this, [this] { switchTab(false); });
    connect(m_smsBtn, &QPushButton::clicked, this, &LoginView::requestSms);
    connect(say,      &QPushButton::clicked, this, &LoginView::speakCode);
}

void LoginView::switchTab(bool yb)
{
    m_tabYb->setChecked(yb);
    m_tabSms->setChecked(!yb);
    m_panes->setCurrentIndex(yb ? 0 : 1);
}

/* 手册 3.3.1：
   密码为空 -> "请输入医保密码"
   密码错误 -> "医保密码错误，请重新输入"
   连续 3 次 -> 锁定 15 分钟（按钮禁用+倒计时，真正禁登；提示语照手册原文），并重置编辑框 */
void LoginView::doYbLogin()
{
    // 锁定期内直接拦下（按钮已禁用，此为双保险）
    if (m_lockUntilSec > 0) {
        const qint64 left = m_lockUntilSec - QDateTime::currentSecsSinceEpoch();
        if (left > 0) {
            Toast::show(this, QStringLiteral("医保登录已临时锁定，请 5 分钟后再尝试登录"), true);
            return;
        }
        m_lockUntilSec = 0;
    }
    if (m_ybNo->text().isEmpty()) {
        Toast::show(this, QStringLiteral("请输入医保卡号"));   // 实践填充（对照表·权限维度）
        return;
    }
    if (m_ybPw->text().isEmpty()) {
        Toast::show(this, QStringLiteral("请输入医保密码"));
        return;
    }
    if (m_ybPw->text() != def::DEMO_SECRET) {
        ++m_errCount;
        if (m_errCount >= def::PWD_MAX_RETRY) {
            Toast::show(this, QStringLiteral("医保登录已临时锁定，请 5 分钟后再尝试登录"), true);
            resetFields();
            m_errCount = 0;
            // 真正锁定：15 分钟禁登倒计时（时长口径见手册符合性对照表 终案 #7）
            m_lockUntilSec = QDateTime::currentSecsSinceEpoch() + def::PWD_LOCK_MIN * 60;
            m_lockCd->start(1000);
            tickLockCountdown();   // 立即禁用，不等首个 tick
            m_tabYb->click();   // 停在医保页展示禁用态
        } else {
            Toast::show(this, QStringLiteral("医保密码错误，请重新输入（还剩 %1 次机会）")
                        .arg(def::PWD_MAX_RETRY - m_errCount), true);
        }
        return;
    }
    m_errCount = 0;
    emit loginSucceeded();
}

void LoginView::doSmsLogin()
{
    if (m_tel->text().isEmpty()) {
        Toast::show(this, QStringLiteral("请输入手机号"));
        return;
    }
    if (m_code->text() != def::DEMO_SECRET) {
        Toast::show(this, QStringLiteral("验证码不对（演示：123456）"), true);
        return;
    }
    emit loginSucceeded();
}

/* 获取验证码：60s 倒计时，toast + 语音双通道 */
void LoginView::requestSms()
{
    m_sec = def::SMS_RESEND_S;
    m_smsBtn->setEnabled(false);
    m_smsBtn->setText(QStringLiteral("重新获取(60s)"));
    m_cd->start(1000);
    Toast::show(this, QStringLiteral("验证码已发送，请查收短信（演示：123456）"));
    speakText(QStringLiteral("验证码已发送，请查收短信"));
}

void LoginView::speakCode()
{
    speakText(QStringLiteral("您的验证码是 1 2 3 4 5 6"));
}

void LoginView::tickCountdown()
{
    if (m_sec <= 0) {
        m_cd->stop();
        m_smsBtn->setEnabled(true);
        m_smsBtn->setText(QStringLiteral("获取验证码"));
    } else {
        m_smsBtn->setText(QStringLiteral("重新获取(%1s)").arg(m_sec));
        --m_sec;
    }
}

void LoginView::resetFields()
{
    m_ybNo->clear();
    m_ybPw->clear();
}

/* 医保锁定倒计时：按钮禁用显示剩余时间，到期自动恢复（手册 3.3.1 锁定 15 分钟） */
void LoginView::tickLockCountdown()
{
    const qint64 left = m_lockUntilSec - QDateTime::currentSecsSinceEpoch();
    if (left <= 0) {
        m_lockCd->stop();
        m_lockUntilSec = 0;
        m_ybBtn->setEnabled(true);
        m_ybBtn->setText(QStringLiteral("登 录"));
        return;
    }
    m_ybBtn->setEnabled(false);
    m_ybBtn->setText(QStringLiteral("已锁定 %1:%2")
                     .arg(left / 60, 2, 10, QLatin1Char('0'))
                     .arg(left % 60, 2, 10, QLatin1Char('0')));
}
