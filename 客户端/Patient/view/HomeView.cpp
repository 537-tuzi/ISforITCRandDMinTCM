#include "HomeView.h"
#include "InfoPanel.h"
#include "util/Toast.h"
#include "util/Speak.h"
#include "Dialogs.h"
#include "util/AppDefs.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QResizeEvent>
#include <QtGlobal>
#include <QtMath>

HomeView::HomeView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageHome");
    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(4, 10, 12, 12);   // 左侧贴边，宽度让给右侧内容区
    root->setSpacing(8);

    // ===== 左侧个人信息面板 =====
    m_panel = new InfoPanel;
    connect(m_panel, &InfoPanel::logoutClicked, this, &HomeView::logoutRequested);
    root->addWidget(m_panel);

    // ===== 右侧四宫格（行列等分，自适应剩余空间） =====
    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(10);

    grid->addWidget(makeEntry(QStringLiteral("初次建档"),
                              QStringLiteral("和 AI 助手说说话，录好过敏史、既往病史，建立健康档案。"),
                              pd::Page::Jd), 0, 0);
    grid->addWidget(makeEntry(QStringLiteral("舌苔检测"),
                              QStringLiteral("对准摄像头拍舌头，自动判断健康状态，给出养生建议，可一键预约。"),
                              pd::Page::Detect), 0, 1);
    grid->addWidget(makeEntry(QStringLiteral("就诊记录"),
                              QStringLiteral("查看以前的看病记录、医生开的药方和病例报告。"),
                              pd::Page::Record), 1, 0);

    // 第 4 格：待办事项（不可点击，手册 3.3.1）
    QFrame* cell = new QFrame;
    cell->setObjectName("cellStatic");
    QVBoxLayout* cb = new QVBoxLayout(cell);
    cb->setContentsMargins(16, 14, 16, 12);
    cb->setSpacing(6);
    QLabel* t = new QLabel(QStringLiteral("待办事项"));
    t->setObjectName("cellTitle");
    cb->addWidget(t);
    m_todoBox = new QVBoxLayout;
    m_todoBox->setSpacing(0);
    cb->addLayout(m_todoBox, 1);
    cb->addStretch();

    grid->addWidget(cell, 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    root->addLayout(grid, 1);

    // 演示初始待办（手册 3.3.5：仅显示未过期且"已预约"）
    pd::Appointment a1;
    a1.date = def::DEMO_DATE; a1.week = def::DEMO_WEEK;
    a1.doc  = def::DEMO_DOC;
    a1.slot = def::DEMO_SLOT;
    a1.booked = true; a1.soon = true;
    m_todos.append(a1);
    pd::Appointment a2;
    a2.date = QStringLiteral("11-02"); a2.week = QStringLiteral("周一");
    a2.doc  = QStringLiteral("内科 · 赵国安");
    a2.slot = QStringLiteral("上午 08:00-12:00");
    a2.booked = true; a2.soon = false;
    m_todos.append(a2);
    refreshTodos();
}

QWidget* HomeView::makeEntry(const QString& title, const QString& desc, pd::Page page)
{
    uif::ClickFrame* cell = new uif::ClickFrame;
    cell->setObjectName("cell");
    QVBoxLayout* v = new QVBoxLayout(cell);
    v->setContentsMargins(16, 14, 16, 12);
    v->setSpacing(8);

    QLabel* tl = new QLabel(title);
    tl->setObjectName("cellTitle");
    if (page == pd::Page::Jd) m_cellTitle = tl;
    v->addWidget(tl, 0, Qt::AlignTop);

    QLabel* d = new QLabel(desc);
    d->setObjectName("cellDesc");
    d->setWordWrap(true);
    if (page == pd::Page::Jd) m_cellDesc = d;
    v->addWidget(d, 1);

    connect(cell, &uif::ClickFrame::clicked, this, [this, page] { emit cellClicked(page); });
    return cell;
}

/* 待办行：第一行=科室·医生+状态胶囊；第二行=日期 起止时间+取消 */
void HomeView::refreshTodos()
{
    while (m_todoBox->count() > 0) {
        QLayoutItem* it = m_todoBox->takeAt(0);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
    if (m_todos.isEmpty()) {
        QLabel* empty = new QLabel(QStringLiteral("暂无未过期的预约"));
        empty->setObjectName("apptEmpty");
        empty->setAlignment(Qt::AlignCenter);
        m_todoBox->addWidget(empty);
        return;
    }
    for (int i = 0; i < m_todos.size(); ++i) {
        const pd::Appointment& a = m_todos[i];
        QFrame* row = new QFrame;
        row->setObjectName("trow");
        QVBoxLayout* v = new QVBoxLayout(row);
        v->setContentsMargins(2, 10, 2, 10);
        v->setSpacing(5);

        // 第一行：科室·医生 + 状态胶囊
        QHBoxLayout* l1 = new QHBoxLayout;
        l1->setSpacing(8);
        QLabel* doc = new QLabel(a.doc);
        doc->setObjectName("tdoc");
        l1->addWidget(doc);
        QLabel* pill = new QLabel;
        if (!a.booked)   { pill->setText(QStringLiteral("已取消"));  pill->setObjectName("pillGray"); }
        else if (a.soon) { pill->setText(QStringLiteral("即将就诊")); pill->setObjectName("pillSoon"); }
        else             { pill->setText(QStringLiteral("已预约"));  pill->setObjectName("pillOk"); }
        l1->addWidget(pill);
        l1->addStretch();
        v->addLayout(l1);

        // 第二行：10-30 19:00-21:00 + 取消
        QHBoxLayout* l2 = new QHBoxLayout;
        l2->setSpacing(8);
        QLabel* dt = new QLabel(a.date + QStringLiteral(" ") + def::slotTime(a.slot));
        dt->setObjectName("t1b");
        l2->addWidget(dt);
        l2->addStretch();
        if (a.booked) {
            QPushButton* cancel = new QPushButton(QStringLiteral("取消预约"));
            cancel->setObjectName("btnLink");
            cancel->setCursor(Qt::PointingHandCursor);
            connect(cancel, &QPushButton::clicked, this, [this, i] { askCancel(i); });
            l2->addWidget(cancel);
        }
        v->addLayout(l2);
        m_todoBox->addWidget(row);
    }
}

/* 取消：手册全局规则——就诊前 30 分钟内禁止取消（toast+语音双通道） */
void HomeView::askCancel(int i)
{
    if (i < 0 || i >= m_todos.size()) return;
    if (!m_todos[i].booked) return;
    if (m_todos[i].soon) {
        Toast::show(window(), QStringLiteral("距就诊开始不足 30 分钟，按规则不能取消预约"), true);
        speakText(QStringLiteral("距就诊开始不足三十分钟，按规则不能取消预约"));
        return;
    }
    CancelDialog dlg(m_todos[i], window());
    if (dlg.exec() == QDialog::Accepted) {
        m_todos[i].booked = false;
        refreshTodos();
        Toast::show(window(), QStringLiteral("已取消预约，如需再看请重新预约"));
    }
}

/* 检测页挂号成功后同步（自动去重：同日期+同医生；自动预约重复时不提示，由检测页报 #18） */
void HomeView::addAppointment(const pd::Appointment& appt, bool autoBook)
{
    for (const pd::Appointment& x : m_todos)
        if (x.date == appt.date && x.doc == appt.doc) {
            if (!autoBook)
                Toast::show(window(), QStringLiteral("该预约已在「待办事项」列表中"));
            return;
        }
    m_todos.prepend(appt);
    refreshTodos();
}

/* 窗口比例变化：按 UiScale 同一因子调整本页内部尺寸（面板宽） */
void HomeView::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    if (!m_panel) return;
    const qreal s = def::FONT_COMP * qMax<qreal>(1.0, width() / 800.0);
    m_panel->setFixedWidth(qRound(def::PANEL_W * s));
}

void HomeView::setJdDone(bool done)
{
    if (done) {
        // 建档完成后入口转"我的档案"（手册表 3：支持手动修正）
        m_cellTitle->setText(QStringLiteral("我的档案"));
        m_cellDesc->setText(QStringLiteral("查看和修改您的过敏史、既往病史等健康档案信息。"));
    }
}
