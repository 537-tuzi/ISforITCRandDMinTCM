#include "MySchedView.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QBrush>
#include <QStringList>

namespace {
// 演示基准周周一（当前周标签 10-27 ~ 11-02；格内容按此周静态固定，切周不变）
const QDate kDemoMonday(2026, 10, 27);
}

MySchedView::MySchedView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageMySched");
    m_weekStart = kDemoMonday;

    // ===== 演示排班（张主任/中医内科，上限 20）：周一/周三上午、周二下午、周四晚班 =====
    sdt::SchedCell c;
    c.doctor = QStringLiteral("张主任");
    c.dept   = QStringLiteral("中医内科");
    c.maxN   = 20;
    c.date = kDemoMonday.addDays(0).toString(QStringLiteral("yyyy-MM-dd")); c.shift = 1; c.booked = 3; m_cells.append(c);  // 周一 上午
    c.date = kDemoMonday.addDays(1).toString(QStringLiteral("yyyy-MM-dd")); c.shift = 2; c.booked = 5; m_cells.append(c);  // 周二 下午
    c.date = kDemoMonday.addDays(2).toString(QStringLiteral("yyyy-MM-dd")); c.shift = 1; c.booked = 8; m_cells.append(c);  // 周三 上午
    c.date = kDemoMonday.addDays(3).toString(QStringLiteral("yyyy-MM-dd")); c.shift = 3; c.booked = 2; m_cells.append(c);  // 周四 晚班

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    QFrame* card = new QFrame;
    card->setObjectName("card");
    root->addWidget(card, 1);

    QVBoxLayout* cv = new QVBoxLayout(card);
    cv->setContentsMargins(0, 0, 0, 0);
    cv->setSpacing(0);

    // ===== 顶部：标题 + 周导航【上一周】当前周标签【下一周】 =====
    QWidget* head = new QWidget;
    QHBoxLayout* hv = new QHBoxLayout(head);
    hv->setContentsMargins(16, 12, 16, 8);
    hv->setSpacing(12);
    QLabel* title = new QLabel(QStringLiteral("我的排班"));
    title->setObjectName("chTitle");
    hv->addWidget(title);
    hv->addStretch();

    QPushButton* prev = new QPushButton(QStringLiteral("上一周"));
    prev->setObjectName("btnOutSm");
    prev->setCursor(Qt::PointingHandCursor);
    m_weekLabel = new QLabel;
    m_weekLabel->setObjectName("chSub");
    m_weekLabel->setAlignment(Qt::AlignCenter);
    m_weekLabel->setMinimumWidth(110);   // 固定最小宽，切周时标签不跳动
    QPushButton* next = new QPushButton(QStringLiteral("下一周"));
    next->setObjectName("btnOutSm");
    next->setCursor(Qt::PointingHandCursor);
    hv->addWidget(prev);
    hv->addWidget(m_weekLabel);
    hv->addWidget(next);
    cv->addWidget(head);

    // ===== 周视图表格：行=班次，列=周一..周日（表头带日期 MM-DD） =====
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(16, 4, 16, 12);
    bv->setSpacing(8);

    m_table = new QTableWidget(3, 7);
    m_table->setObjectName("tb");
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->verticalHeader()->setDefaultSectionSize(60);   // 行高 60
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionsClickable(false);
    m_table->verticalHeader()->setSectionsClickable(false);
    m_table->setCornerButtonEnabled(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);   // 只读
    m_table->setSelectionMode(QAbstractItemView::NoSelection);     // 只读页不加选中态
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setShowGrid(false);
    bv->addWidget(m_table, 1);

    // 角注说明
    QLabel* note = new QLabel(QStringLiteral("排班由管理员设置，医生只读"));
    note->setObjectName("chSub");
    bv->addWidget(note);

    cv->addWidget(body, 1);

    connect(prev, &QPushButton::clicked, this, &MySchedView::onPrevWeek);
    connect(next, &QPushButton::clicked, this, &MySchedView::onNextWeek);

    rebuild();
}

void MySchedView::onPrevWeek()
{
    m_weekStart = m_weekStart.addDays(-7);   // 标签日期 -7 天（跨月自动进位）
    rebuild();
}

void MySchedView::onNextWeek()
{
    m_weekStart = m_weekStart.addDays(7);    // 标签日期 +7 天
    rebuild();
}

/* 整表刷新：周标签 + 列表头（周几+当周日期）+ 行表头（三班次）+ 排班格。
   演示数据静态固定：切周仅标签与表头日期 ±7 天，格内容不随周变化。 */
void MySchedView::rebuild()
{
    // 当前周标签：如 10-27 ~ 11-02
    m_weekLabel->setText(QStringLiteral("%1 ~ %2").arg(
        m_weekStart.toString(QStringLiteral("MM-dd")),
        m_weekStart.addDays(6).toString(QStringLiteral("MM-dd"))));

    // 列表头：周一..周日 + 当周日期（单行"周一 10-27"，表头默认不绘制换行）
    static const char* kWeekName[7] = { "周一", "周二", "周三", "周四", "周五", "周六", "周日" };
    QStringList heads;
    for (int i = 0; i < 7; ++i)
        heads << QStringLiteral("%1 %2").arg(QString::fromUtf8(kWeekName[i]),
                                             m_weekStart.addDays(i).toString(QStringLiteral("MM-dd")));
    m_table->setHorizontalHeaderLabels(heads);

    // 行表头：三班次（def::shiftName：1上午 2下午 3晚班）
    QStringList shifts;
    for (int s = 1; s <= 3; ++s) shifts << def::shiftName(s);
    m_table->setVerticalHeaderLabels(shifts);

    // 先全部置"—"（灰，居中，无排班）
    for (int r = 0; r < 3; ++r) {
        for (int col = 0; col < 7; ++col) {
            QTableWidgetItem* it = new QTableWidgetItem(QStringLiteral("—"));
            it->setTextAlignment(Qt::AlignCenter);
            it->setForeground(QBrush(QColor("#a9a698")));
            m_table->setItem(r, col, it);
        }
    }

    // 再叠加演示排班格：✓ 已排（已约 n/上限）
    for (const sdt::SchedCell& sc : m_cells) {
        const int col = kDemoMonday.daysTo(QDate::fromString(sc.date, QStringLiteral("yyyy-MM-dd")));
        if (col < 0 || col > 6 || sc.shift < 1 || sc.shift > 3) continue;
        QTableWidgetItem* it = m_table->item(sc.shift - 1, col);
        if (!it) continue;
        it->setText(QStringLiteral("✓ 已排（已约 %1/%2）").arg(sc.booked).arg(sc.maxN));
        it->setForeground(QBrush(QColor("#234b40")));
    }
}
