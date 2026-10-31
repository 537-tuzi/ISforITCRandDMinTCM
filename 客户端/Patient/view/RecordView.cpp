#include "RecordView.h"
#include "InfoPanel.h"
#include "util/Toast.h"
#include "Dialogs.h"
#include "util/UIFactory.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QHeaderView>
#include <QBrush>
#include <QResizeEvent>

RecordView::RecordView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageRecord");
    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(4, 10, 12, 12);   // 左侧贴边，宽度让给右侧内容区
    root->setSpacing(8);

    // ===== 演示数据（查看报告弹窗用，手册 3.3.5） =====
    pd::VisitRecord r;
    r.date = QStringLiteral("2026-10-20 上午 09:30"); r.slot = QStringLiteral("上午");
    r.doc = QStringLiteral("脾胃科 / 陈志远（主任医师）");
    r.tongue = QStringLiteral("白腻苔（93%）· 湿气重");
    r.dia = QStringLiteral("脾虚湿盛证");
    r.rx = QStringLiteral("参苓白术散 7 剂，水煎服，每日一剂，分两次温服");
    r.ad = QStringLiteral("忌生冷油腻，规律作息，两周后复诊");
    r.conf = QStringLiteral("93%"); r.when = QStringLiteral("2026-10-20 09:12");
    m_visits.append(r);

    r.date = QStringLiteral("2026-09-08 下午 14:30"); r.slot = QStringLiteral("下午");
    r.doc = QStringLiteral("脾胃科 / 陈志远（主任医师）");
    r.tongue = QStringLiteral("薄白苔（95%）· 大致正常");
    r.dia = QStringLiteral("脾胃失调（轻）");
    r.rx = QStringLiteral("香砂六君丸 2 盒，饭前温水送服");
    r.ad = QStringLiteral("饮食有节，少操心");
    r.conf = QStringLiteral("95%"); r.when = QStringLiteral("2026-09-08 14:05");
    m_visits.append(r);

    r.date = QStringLiteral("2026-07-15 上午 08:30"); r.slot = QStringLiteral("上午");
    r.doc = QStringLiteral("针灸推拿科 / 周红梅（主治医师）");
    r.tongue = QStringLiteral("白腻苔（90%）· 湿气重");
    r.dia = QStringLiteral("湿阻经络证");
    r.rx = QStringLiteral("针灸治疗 5 次 + 薏米赤小豆食疗");
    r.ad = QStringLiteral("适度运动出汗，别住潮湿地方");
    r.conf = QStringLiteral("90%"); r.when = QStringLiteral("2026-07-15 08:10");
    m_visits.append(r);

    // ===== 左侧信息面板 =====
    m_panel = new InfoPanel;
    connect(m_panel, &InfoPanel::logoutClicked, this, &RecordView::logoutClicked);
    root->addWidget(m_panel);

    QWidget* main = new QWidget;
    QVBoxLayout* mv = new QVBoxLayout(main);
    mv->setContentsMargins(0, 0, 0, 0);
    mv->setSpacing(12);

    // ===== 最近一次诊疗记录 =====
    QFrame* c1 = new QFrame;
    c1->setObjectName("card");
    QVBoxLayout* c1v = new QVBoxLayout(c1);
    c1v->setContentsMargins(0, 0, 0, 0);
    QLabel* t1 = new QLabel(QStringLiteral("最近一次诊疗记录"));
    t1->setObjectName("chTitle");
    t1->setContentsMargins(14, 12, 14, 12);
    c1v->addWidget(t1);
    QWidget* b1 = new QWidget;
    QGridLayout* g1 = new QGridLayout(b1);
    g1->setContentsMargins(14, 2, 14, 8);
    g1->setHorizontalSpacing(16);
    g1->addWidget(uif::makeFieldRow(QStringLiteral("看病时间"), QStringLiteral("2026-10-20 上午 09:30"),
                         QStringLiteral("liRow")), 0, 0);
    g1->addWidget(uif::makeFieldRow(QStringLiteral("医生诊断"), QStringLiteral("脾虚湿盛证"),
                         QStringLiteral("liRow")), 0, 1);
    g1->addWidget(uif::makeFieldRow(QStringLiteral("科室医生"), QStringLiteral("脾胃科 / 陈志远（主任医师）"),
                         QStringLiteral("liRow")), 1, 0);
    g1->addWidget(uif::makeFieldRow(QStringLiteral("开的药"),   QStringLiteral("参苓白术散 7 剂，水煎服，每日一剂"),
                         QStringLiteral("liRow")), 1, 1);
    g1->addWidget(uif::makeFieldRow(QStringLiteral("舌头情况"), QStringLiteral("白腻苔 · 湿气重"),
                         QStringLiteral("liRow")), 2, 0);
    g1->addWidget(uif::makeFieldRow(QStringLiteral("医生嘱咐"), QStringLiteral("忌生冷油腻，规律作息，两周后复诊"),
                         QStringLiteral("liRow")), 2, 1);
    c1v->addWidget(b1);
    mv->addWidget(c1);

    // ===== 全部看病记录 =====
    QFrame* c2 = new QFrame;
    c2->setObjectName("card");
    QVBoxLayout* c2v = new QVBoxLayout(c2);
    c2v->setContentsMargins(0, 0, 0, 0);
    QLabel* t2 = new QLabel(QStringLiteral("全部看病记录"));
    t2->setObjectName("chTitle");
    t2->setContentsMargins(14, 12, 14, 12);
    c2v->addWidget(t2);

    QWidget* b2 = new QWidget;
    QVBoxLayout* b2v = new QVBoxLayout(b2);
    b2v->setContentsMargins(14, 2, 14, 10);
    m_table = new QTableWidget(4, 7);
    m_table->setObjectName("tb");
    QStringList heads;
    heads << QStringLiteral("日期") << QStringLiteral("时段") << QStringLiteral("科室")
          << QStringLiteral("医生") << QStringLiteral("舌象") << QStringLiteral("状态")
          << QStringLiteral("操作");
    m_table->setHorizontalHeaderLabels(heads);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 前 6 列按内容收缩，腾出的宽度全部给最右"操作"列（查看报告不再紧缩）
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_table->setShowGrid(false);
    b2v->addWidget(m_table, 1);
    initTable();

    QWidget* pager = new QWidget;
    QHBoxLayout* ph = new QHBoxLayout(pager);
    ph->setContentsMargins(0, 8, 0, 0);
    QPushButton* prev = new QPushButton(QStringLiteral("上一页"));
    prev->setObjectName("btnOutSm");
    prev->setCursor(Qt::PointingHandCursor);
    QLabel* page = new QLabel(QStringLiteral("共 4 条 · 第 1/1 页"));
    page->setObjectName("pagerTxt");
    page->setAlignment(Qt::AlignCenter);
    QPushButton* next = new QPushButton(QStringLiteral("下一页"));
    next->setObjectName("btnOutSm");
    next->setCursor(Qt::PointingHandCursor);
    ph->addWidget(prev);
    ph->addStretch();
    ph->addWidget(page);
    ph->addStretch();
    ph->addWidget(next);
    b2v->addWidget(pager);
    c2v->addWidget(b2, 1);
    mv->addWidget(c2, 1);

    connect(prev, &QPushButton::clicked, this, &RecordView::onPrev);
    connect(next, &QPushButton::clicked, this, &RecordView::onNext);

    root->addWidget(main, 1);
}

/* 窗口比例变化：按 UiScale 同一因子调整本页内部尺寸（面板宽 + 表格行高） */
void RecordView::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    if (!m_panel || !m_table) return;
    const qreal s = def::FONT_COMP * qMax<qreal>(1.0, width() / def::DESIGN_W);
    m_panel->setFixedWidth(qRound(def::PANEL_W * s));
    applyRowHeight(qRound(def::TBL_ROW_H * s));
}

void RecordView::applyRowHeight(int h)
{
    for (int i = 0; i < m_table->rowCount(); ++i)
        m_table->setRowHeight(i, h);
}

/* 表 4 行：前 3 行已问诊（查看报告），末行已预约（还没看，禁用） */
void RecordView::onPrev()
{ Toast::show(window(), QStringLiteral("已经是第一页了")); }

void RecordView::onNext()
{ Toast::show(window(), QStringLiteral("已经是最后一页了")); }

void RecordView::openReport(int index)
{
    if (index < 0 || index >= m_visits.size()) return;
    ReportDialog d(m_visits.at(index), window());
    d.exec();
}

/* 表格内容（含状态胶囊文字与操作按钮） */
void RecordView::initTable()
{
    struct Row { const char* d; const char* s; const char* dep; const char* doc;
                 const char* tg; bool done; int rec; };
    const Row rows[] = {
        {"2026-10-20", "上午", "脾胃科",     "陈志远", "白腻苔", true,  0},
        {"2026-09-08", "下午", "脾胃科",     "陈志远", "薄白苔", true,  1},
        {"2026-07-15", "上午", "针灸推拿科", "周红梅", "白腻苔", true,  2},
        {"2026-10-30", "晚班", "脾胃科",     "赵国安", "—",      false, -1},
    };
    for (int i = 0; i < 4; ++i) {
        m_table->setRowHeight(i, 52);
        for (int c = 0; c < 6; ++c) {
            QString txt;
            switch (c) {
            case 0: txt = QString::fromUtf8(rows[i].d); break;
            case 1: txt = QString::fromUtf8(rows[i].s); break;
            case 2: txt = QString::fromUtf8(rows[i].dep); break;
            case 3: txt = QString::fromUtf8(rows[i].doc); break;
            case 4: txt = QString::fromUtf8(rows[i].tg); break;
            case 5: txt = rows[i].done ? QStringLiteral("已问诊")
                                       : QStringLiteral("已预约"); break;
            }
            QTableWidgetItem* it = new QTableWidgetItem(txt);
            it->setTextAlignment(Qt::AlignCenter);
            it->setForeground(QBrush(rows[i].done ? QColor("#5f5f56") : QColor(def::CLR_PRESS)));
            m_table->setItem(i, c, it);
        }
        QWidget* btnHost = new QWidget;
        QHBoxLayout* bh = new QHBoxLayout(btnHost);
        bh->setContentsMargins(2, 2, 2, 2);
        QPushButton* b = new QPushButton(rows[i].done ? QStringLiteral("查看报告")
                                                      : QStringLiteral("还没看"));
        b->setObjectName(rows[i].done ? "btnOutSm" : "btnOutSmDis");
        b->setCursor(Qt::PointingHandCursor);
        b->setEnabled(rows[i].done);
        bh->addWidget(b);
        m_table->setCellWidget(i, 6, btnHost);
        if (rows[i].done) {
            const int rec = rows[i].rec;
            connect(b, &QPushButton::clicked, this, [this, rec] { openReport(rec); });
        }
    }
}
