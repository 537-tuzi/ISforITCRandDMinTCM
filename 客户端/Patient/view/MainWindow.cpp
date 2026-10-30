#include "MainWindow.h"
#include "HomeView.h"
#include "ProfileView.h"
#include "DetectView.h"
#include "RecordView.h"
#include "util/UiScale.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QStyle>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setObjectName("mainWin");
    // 800×640 主界面；允许缩放，内部容器全部自适应
    resize(800, 640);
    setMinimumSize(680, 540);

    // ===== 顶部导航条 =====
    QWidget* bar = new QWidget;
    m_bar = bar;
    bar->setObjectName("topbar");
    bar->setFixedHeight(64);
    QHBoxLayout* bh = new QHBoxLayout(bar);
    bh->setContentsMargins(16, 0, 16, 0);
    bh->setSpacing(10);

    m_navs.clear();
    QPushButton* nHome = makeNav(QStringLiteral("首页"), pd::Page::Home);
    m_navJd            = makeNav(QStringLiteral("初次建档"), pd::Page::Jd);
    QPushButton* nDet  = makeNav(QStringLiteral("舌苔检测"), pd::Page::Detect);
    QPushButton* nRec  = makeNav(QStringLiteral("就诊记录"), pd::Page::Record);
    bh->addWidget(nHome);
    bh->addWidget(m_navJd);
    bh->addWidget(nDet);
    bh->addWidget(nRec);
    bh->addStretch();

    QLabel* nm = new QLabel(QStringLiteral("李建国"));
    nm->setObjectName("uchipName");
    bh->addWidget(nm);

    // ===== 内容区 =====
    m_stack = new QStackedWidget;
    m_stack->setObjectName("content");
    m_home    = new HomeView;
    m_profile = new ProfileView;
    m_detect  = new DetectView;
    m_record  = new RecordView;
    m_stack->addWidget(m_home);       // 0
    m_stack->addWidget(m_profile);    // 1
    m_stack->addWidget(m_detect);     // 2
    m_stack->addWidget(m_record);     // 3

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(bar);
    root->addWidget(m_stack, 1);

    setCurrent(pd::Page::Home);
}

QPushButton* MainWindow::makeNav(const QString& text, pd::Page page)
{
    QPushButton* b = new QPushButton(text);
    b->setObjectName("navIt");
    b->setCursor(Qt::PointingHandCursor);
    b->setProperty("page", int(page));
    b->setProperty("cur", false);
    connect(b, &QPushButton::clicked, this, [this, page] { emit navRequested(page); });
    m_navs.append(b);
    return b;
}

void MainWindow::setCurrent(pd::Page page)
{
    int idx = 0;
    switch (page) {
    case pd::Page::Jd:     idx = 1; break;
    case pd::Page::Detect: idx = 2; break;
    case pd::Page::Record: idx = 3; break;
    case pd::Page::Home:   idx = 0; break;
    }
    m_stack->setCurrentIndex(idx);
    foreach (QPushButton* b, m_navs) {
        const bool cur = (b->property("page").toInt() == int(page));
        b->setProperty("cur", cur);
        b->style()->unpolish(b);
        b->style()->polish(b);
    }
}

void MainWindow::setJdTitle(const QString& title)
{
    m_navJd->setText(title);
}

/* 窗口放大（如最大化）时按宽度比例放大：本类只管全局样式与导航条高；
   各页面在自身 resizeEvent 里按同一因子调整自己的内部尺寸（控制反转，
   本类不伸手进 View 内部），总因子 = 字体补偿 × 窗口比例 */
void MainWindow::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    if (!m_bar) return;                  // 构造早期的 resize，成员未就绪
    const qreal s = def::FONT_COMP * qMax<qreal>(1.0, width() / 800.0);
    if (qAbs(s - UiScale::current()) > 0.02)   // 阈值防拖拽抖动频繁重排
        UiScale::apply(s);
    m_bar->setFixedHeight(qRound(def::TOPBAR_H * s));
}
