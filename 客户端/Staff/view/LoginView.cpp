#include "LoginView.h"
#include "util/Toast.h"
#include "util/AppDefs.h"
#include "util/StaffTypes.h"
#include "view/Dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRegularExpression>

LoginView::LoginView(QWidget* parent) : QWidget(parent)
{
    setObjectName("authPanel");
    setFixedSize(480, 520);    // 工作人员登录卡（3:4 近似，贴内容）

    QWidget* box = new QWidget;
    box->setObjectName("authBox");
    box->setFixedSize(448, 488);

    QVBoxLayout* b = new QVBoxLayout(box);
    b->setContentsMargins(48, 36, 48, 24);
    b->setSpacing(0);

    QLabel* title = new QLabel(QStringLiteral("中医舌苔诊疗系统 · 工作人员端"));
    title->setObjectName("authTitle");
    title->setAlignment(Qt::AlignCenter);
    b->addWidget(title);
    b->addSpacing(28);

    QLabel* l1 = new QLabel(QStringLiteral("工号"));
    l1->setObjectName("fldLabel");
    m_no = new QLineEdit;
    m_no->setMaxLength(10);   // 手册 3.4.1"10位"=输入上限；分配固定 6 位
    m_no->setPlaceholderText(QStringLiteral("请输入 6 位工号（演示：100002 医生 / 100001 管理员）"));
    b->addWidget(l1); b->addWidget(m_no); b->addSpacing(10);

    QLabel* l2 = new QLabel(QStringLiteral("密码"));
    l2->setObjectName("fldLabel");
    m_pw = new QLineEdit;
    m_pw->setEchoMode(QLineEdit::Password);
    m_pw->setPlaceholderText(QStringLiteral("请输入密码（演示：123456）"));
    b->addWidget(l2); b->addWidget(m_pw); b->addSpacing(14);

    m_btn = new QPushButton(QStringLiteral("登 录"));
    m_btn->setObjectName("btnPriLg");
    m_btn->setCursor(Qt::PointingHandCursor);
    connect(m_btn, &QPushButton::clicked, this, &LoginView::doLogin);
    b->addWidget(m_btn);
    b->addStretch();

    QHBoxLayout* foot = new QHBoxLayout;
    QPushButton* chg = new QPushButton(QStringLiteral("修改密码"));
    chg->setObjectName("btnLink");
    chg->setCursor(Qt::PointingHandCursor);
    connect(chg, &QPushButton::clicked, this, &LoginView::openChangePwd);
    foot->addStretch();
    foot->addWidget(chg);
    b->addLayout(foot);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->addWidget(box);

    m_lockCd = new QTimer(this);
    connect(m_lockCd, &QTimer::timeout, this, &LoginView::tickLockCountdown);
}

/* 手册 3.2.4：账号密码有效性+账号状态校验；连续 3 次错误锁定 24 小时自动解锁 */
void LoginView::doLogin()
{
    if (m_lockUntilSec > 0) {
        const qint64 left = m_lockUntilSec - QDateTime::currentSecsSinceEpoch();
        if (left > 0) {
            Toast::show(this, QStringLiteral("密码连续错误 3 次，账号已锁定 24 小时"), true);
            return;
        }
        m_lockUntilSec = 0;
    }
    if (m_no->text().trimmed().isEmpty()) {
        Toast::show(this, QStringLiteral("请输入工号"));
        return;
    }
    if (m_pw->text().isEmpty()) {
        Toast::show(this, QStringLiteral("请输入密码"));
        return;
    }
    // 演示账号比对（正式版：服务端按 work_no 查 staff，MD5 比对，状态/锁定校验）
    int role = 0;
    QString name;
    for (int i = 0; i < def::DEMO_ACCOUNT_COUNT; ++i) {
        if (m_no->text().trimmed() == QString::fromUtf8(def::DEMO_ACCOUNTS[i].no)) {
            role = def::DEMO_ACCOUNTS[i].role;
            name = QString::fromUtf8(def::DEMO_ACCOUNTS[i].name);
            break;
        }
    }
    if (role == 0) {
        Toast::show(this, QStringLiteral("工号不存在（演示：100001~100004）"), true);
        return;
    }
    if (m_pw->text() != def::DEMO_SECRET) {
        ++m_errCount;
        if (m_errCount >= def::PWD_MAX_RETRY) {
            Toast::show(this, QStringLiteral("密码连续错误 3 次，账号已锁定 24 小时"), true);
            resetFields();
            m_errCount = 0;
            m_lockUntilSec = QDateTime::currentSecsSinceEpoch() +
                             qint64(def::STAFF_LOCK_HOURS) * 3600;
            m_lockCd->start(1000);
        } else {
            Toast::show(this, QStringLiteral("密码错误，请重新输入（还剩 %1 次机会）")
                        .arg(def::PWD_MAX_RETRY - m_errCount), true);
        }
        return;
    }
    m_errCount = 0;
    emit loginSucceeded(role, m_no->text().trimmed(), name);
}

/* 修改密码（手册 3.4.1/3.5.1+表5/表6：验证原密码，新密码 6-16 位字母+数字） */
void LoginView::openChangePwd()
{
    ChangePwdDialog d(window());
    if (d.exec() == QDialog::Accepted) {
        Toast::show(this, QStringLiteral("密码修改成功，请用新密码重新登录"));
        resetFields();
    }
}

void LoginView::tickLockCountdown()
{
    const qint64 left = m_lockUntilSec - QDateTime::currentSecsSinceEpoch();
    if (left <= 0) {
        m_lockCd->stop();
        m_lockUntilSec = 0;
        m_btn->setEnabled(true);
        m_btn->setText(QStringLiteral("登 录"));
        return;
    }
    m_btn->setEnabled(false);
    m_btn->setText(QStringLiteral("已锁定 %1:%2:%3")
                   .arg(left / 3600, 2, 10, QLatin1Char('0'))
                   .arg((left % 3600) / 60, 2, 10, QLatin1Char('0'))
                   .arg(left % 60, 2, 10, QLatin1Char('0')));
}

void LoginView::resetFields()
{
    m_no->clear();
    m_pw->clear();
}
