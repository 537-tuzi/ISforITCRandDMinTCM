#include "MainWindow.h"
#include "ApptListView.h"
#include "DiagnoseView.h"
#include "HistoryView.h"
#include "MySchedView.h"
#include "DoctorView.h"
#include "DeptView.h"
#include "SchedMgrView.h"
#include "StatsView.h"
#include "LogView.h"
#include "util/UiScale.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QStyle>
#include <QResizeEvent>

MainWindow::MainWindow(int role, const QString& workNo, const QString& name, QWidget* parent)
    : QWidget(parent), m_role(role), m_workNo(workNo), m_name(name)
{
    setObjectName("mainWin");
    resize(800, 640);
    setMinimumSize(680, 540);

    // ===== 顶部导航条（角色分流） =====
    QWidget* bar = new QWidget;
    m_bar = bar;
    bar->setObjectName("topbar");
    bar->setFixedHeight(def::TOPBAR_H);
    QHBoxLayout* bh = new QHBoxLayout(bar);
    bh->setContentsMargins(16, 0, 16, 0);
    bh->setSpacing(10);

    QLabel* brand = new QLabel(role == sdt::Admin ? QStringLiteral("舌苔诊疗 · 管理员端")
                                                  : QStringLiteral("舌苔诊疗 · 医生端"));
    brand->setObjectName("uchipName");
    bh->addWidget(brand);

    m_navs.clear();
    if (role == sdt::Doctor) {
        bh->addWidget(makeNav(QStringLiteral("预约患者"), sdt::Page::ApptList));
        bh->addWidget(makeNav(QStringLiteral("问诊开方"), sdt::Page::Diagnose));
        bh->addWidget(makeNav(QStringLiteral("历史病例"), sdt::Page::History));
        bh->addWidget(makeNav(QStringLiteral("我的排班"), sdt::Page::MySchedule));
    } else {
        bh->addWidget(makeNav(QStringLiteral("医师档案"), sdt::Page::Doctors));
        bh->addWidget(makeNav(QStringLiteral("科室管理"), sdt::Page::Depts));
        bh->addWidget(makeNav(QStringLiteral("排班调度"), sdt::Page::SchedMgr));
        bh->addWidget(makeNav(QStringLiteral("工作量统计"), sdt::Page::Stats));
        bh->addWidget(makeNav(QStringLiteral("系统日志"), sdt::Page::Logs));
    }
    bh->addStretch();

    QLabel* who = new QLabel(QStringLiteral("%1（%2）").arg(m_name, m_workNo));
    who->setObjectName("uchipName");
    bh->addWidget(who);
    QPushButton* out = new QPushButton(QStringLiteral("退出登录"));
    out->setObjectName("btnLink");
    out->setCursor(Qt::PointingHandCursor);
    connect(out, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    bh->addWidget(out);

    // ===== 内容区（按角色装配） =====
    m_stack = new QStackedWidget;
    m_stack->setObjectName("content");
    if (role == sdt::Doctor) {
        m_appt  = new ApptListView;  m_stack->addWidget(m_appt);
        m_diag  = new DiagnoseView;  m_stack->addWidget(m_diag);
        m_hist  = new HistoryView;   m_stack->addWidget(m_hist);
        m_sched = new MySchedView;   m_stack->addWidget(m_sched);
        setCurrent(sdt::Page::ApptList);
    } else {
        m_docs  = new DoctorView;    m_stack->addWidget(m_docs);
        m_depts = new DeptView;      m_stack->addWidget(m_depts);
        m_smgr  = new SchedMgrView;  m_stack->addWidget(m_smgr);
        m_stats = new StatsView;     m_stack->addWidget(m_stats);
        m_logs  = new LogView;       m_stack->addWidget(m_logs);
        setCurrent(sdt::Page::Doctors);
    }

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(bar);
    root->addWidget(m_stack, 1);
}

QPushButton* MainWindow::makeNav(const QString& text, sdt::Page page)
{
    QPushButton* b = new QPushButton(text);
    b->setObjectName("navIt");
    b->setCursor(Qt::PointingHandCursor);
    b->setProperty("page", int(page));
    b->setProperty("cur", false);
    connect(b, &QPushButton::clicked, this, [this, page] { setCurrent(page); });
    m_navs.append(b);
    return b;
}

void MainWindow::setCurrent(sdt::Page page)
{
    int idx = m_stack->indexOf(page == sdt::Page::ApptList  ? (QWidget*)m_appt
                            : page == sdt::Page::Diagnose  ? (QWidget*)m_diag
                            : page == sdt::Page::History   ? (QWidget*)m_hist
                            : page == sdt::Page::MySchedule? (QWidget*)m_sched
                            : page == sdt::Page::Doctors   ? (QWidget*)m_docs
                            : page == sdt::Page::Depts     ? (QWidget*)m_depts
                            : page == sdt::Page::SchedMgr  ? (QWidget*)m_smgr
                            : page == sdt::Page::Stats     ? (QWidget*)m_stats
                                                           : (QWidget*)m_logs);
    m_stack->setCurrentIndex(idx);
    foreach (QPushButton* b, m_navs) {
        b->setProperty("cur", b->property("page").toInt() == int(page));
        b->style()->unpolish(b);
        b->style()->polish(b);
    }
}

/* 窗口缩放：全局样式+导航条高（页面内部各自 resizeEvent 自理，同 Patient 模式） */
void MainWindow::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    if (!m_bar) return;
    const qreal s = def::FONT_COMP * qMax<qreal>(1.0, width() / 800.0);
    if (qAbs(s - UiScale::current()) > 0.02)
        UiScale::apply(s);
    m_bar->setFixedHeight(qRound(def::TOPBAR_H * s));
}
