#include "SchedMgrView.h"
#include "util/Toast.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QFrame>
#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QBrush>
#include <QDate>

/* 演示排班原文（医生|星期几0-6|班次1-3|已约/上限），新增排班也追加于此 */
static QStringList g_cells = {
    QStringLiteral("张主任|0|1|3/20"),
    QStringLiteral("张主任|2|2|0/20"),
    QStringLiteral("张主任|3|3|5/20"),
    QStringLiteral("李医生|0|3|1/20"),
    QStringLiteral("李医生|1|1|0/20"),
    QStringLiteral("王医生|1|2|2/20"),
    QStringLiteral("王医生|4|1|0/20"),
};

SchedMgrView::SchedMgrView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageSmgr");
    m_allCells = g_cells;

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    QFrame* card = new QFrame;
    card->setObjectName("card");
    root->addWidget(card, 1);

    QVBoxLayout* cv = new QVBoxLayout(card);
    cv->setContentsMargins(0, 0, 0, 0);

    // ===== 工具栏：科室 + 周导航 + 添加排班 =====
    QWidget* head = new QWidget;
    QHBoxLayout* hh = new QHBoxLayout(head);
    hh->setContentsMargins(16, 12, 16, 8);
    hh->setSpacing(10);
    QLabel* title = new QLabel(QStringLiteral("排班调度"));
    title->setObjectName("chTitle");
    m_dept = new QComboBox;
    m_dept->addItems(QStringList() << QStringLiteral("中医内科") << QStringLiteral("针灸推拿科"));
    QPushButton* prev = new QPushButton(QStringLiteral("上一周"));
    prev->setObjectName("btnOutSm");
    prev->setCursor(Qt::PointingHandCursor);
    m_weekLabel = new QLabel;
    m_weekLabel->setObjectName("chSub");
    m_weekLabel->setAlignment(Qt::AlignCenter);
    m_weekLabel->setMinimumWidth(110);
    QPushButton* next = new QPushButton(QStringLiteral("下一周"));
    next->setObjectName("btnOutSm");
    next->setCursor(Qt::PointingHandCursor);
    QPushButton* add = new QPushButton(QStringLiteral("添加排班"));
    add->setObjectName("btnPri");
    add->setCursor(Qt::PointingHandCursor);
    hh->addWidget(title);
    hh->addSpacing(8);
    hh->addWidget(m_dept);
    hh->addStretch();
    hh->addWidget(prev);
    hh->addWidget(m_weekLabel);
    hh->addWidget(next);
    hh->addStretch();
    hh->addWidget(add);
    cv->addWidget(head);

    // ===== 周视图表格：行=医生，列=周一..周日 =====
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(16, 4, 16, 12);
    bv->setSpacing(8);
    m_table = new QTableWidget(0, 7);
    m_table->setObjectName("tb");
    m_table->verticalHeader()->setVisible(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setDefaultSectionSize(64);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    bv->addWidget(m_table, 1);
    QLabel* note = new QLabel(QStringLiteral("冲突检测按应用层查询实现（决策清单 D11）；演示排班各周相同"));
    note->setObjectName("chSub");
    bv->addWidget(note);
    cv->addWidget(body, 1);

    connect(m_dept, &QComboBox::currentTextChanged, this, &SchedMgrView::onDeptChanged);
    connect(prev, &QPushButton::clicked, this, &SchedMgrView::onPrevWeek);
    connect(next, &QPushButton::clicked, this, &SchedMgrView::onNextWeek);
    connect(add, &QPushButton::clicked, this, &SchedMgrView::onAddSched);

    rebuild();
}

QVector<SchedMgrView::Doc> SchedMgrView::doctors() const
{
    QVector<Doc> ds;
    if (m_dept->currentText() == QStringLiteral("中医内科")) {
        ds << Doc{ QStringLiteral("张主任"), QStringLiteral("中医内科") }
           << Doc{ QStringLiteral("李医生"), QStringLiteral("中医内科") };
    } else {
        ds << Doc{ QStringLiteral("王医生"), QStringLiteral("针灸推拿科") };
    }
    return ds;
}

QStringList SchedMgrView::cells() const
{
    QStringList out;
    const QVector<Doc> ds = doctors();
    for (const QString& c : m_allCells)
        foreach (const Doc& d, ds)
            if (c.startsWith(d.name + QLatin1Char('|')))
                out << c;
    return out;
}

void SchedMgrView::onDeptChanged() { rebuild(); }
void SchedMgrView::onPrevWeek()    { --m_weekOff; rebuild(); }
void SchedMgrView::onNextWeek()    { ++m_weekOff; rebuild(); }

void SchedMgrView::rebuild()
{
    // 周标签：演示基准周一 2026-10-27 ± 偏移
    const QDate base(2026, 10, 27);
    const QDate monday = base.addDays(m_weekOff * 7);
    m_weekLabel->setText(QStringLiteral("%1 ~ %2").arg(
        monday.toString(QStringLiteral("MM-dd")),
        monday.addDays(6).toString(QStringLiteral("MM-dd"))));

    static const char* kDay[7] = { "周一", "周二", "周三", "周四", "周五", "周六", "周日" };
    QStringList heads;
    for (int i = 0; i < 7; ++i)
        heads << QStringLiteral("%1 %2").arg(QString::fromUtf8(kDay[i]),
                    monday.addDays(i).toString(QStringLiteral("MM-dd")));
    m_table->setHorizontalHeaderLabels(heads);

    const QVector<Doc> ds = doctors();
    m_table->setRowCount(ds.size());
    QStringList vHeads;
    const QStringList cs = cells();
    for (int r = 0; r < ds.size(); ++r) {
        vHeads << ds[r].name;
        for (int col = 0; col < 7; ++col) {
            QTableWidgetItem* it = new QTableWidgetItem(QStringLiteral("—"));
            it->setTextAlignment(Qt::AlignCenter);
            it->setForeground(QBrush(QColor("#a9a698")));
            m_table->setItem(r, col, it);
        }
    }
    m_table->setVerticalHeaderLabels(vHeads);

    // 叠加排班格："上午✓3/20"式
    for (const QString& c : cs) {
        const QStringList parts = c.split(QLatin1Char('|'));
        if (parts.size() < 4) continue;
        const QString& doc = parts[0];
        const int day  = parts[1].toInt();
        const int sh   = parts[2].toInt();
        const QString& nb = parts[3];
        for (int r = 0; r < ds.size(); ++r) {
            if (ds[r].name != doc) continue;
            QTableWidgetItem* it = m_table->item(r, day);
            if (!it) break;
            const QString text = (sh == 1 ? QStringLiteral("上午")
                              : sh == 2 ? QStringLiteral("下午")
                                        : QStringLiteral("晚班"))
                              + QStringLiteral("✓") + nb;
            if (it->text() == QStringLiteral("—")) it->setText(text);
            else it->setText(it->text() + QStringLiteral("\n") + text);
            it->setForeground(QBrush(QColor("#234b40")));
            break;
        }
    }
}

/* 添加排班：医生/星期/班次/上限 → 冲突检测（同医生同日同班拒绝）→ 追加并刷新 */
void SchedMgrView::onAddSched()
{
    QDialog dlg(window());
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    bv->setSpacing(10);

    QLabel* l1 = new QLabel(QStringLiteral("医生"));
    l1->setObjectName("fldLabel");
    QComboBox* cbDoc = new QComboBox;
    foreach (const Doc& d, doctors()) cbDoc->addItem(d.name);

    QLabel* l2 = new QLabel(QStringLiteral("日期"));
    l2->setObjectName("fldLabel");
    QComboBox* cbDay = new QComboBox;
    cbDay->addItems(QStringList() << QStringLiteral("周一") << QStringLiteral("周二")
                                  << QStringLiteral("周三") << QStringLiteral("周四")
                                  << QStringLiteral("周五") << QStringLiteral("周六")
                                  << QStringLiteral("周日"));

    QLabel* l3 = new QLabel(QStringLiteral("班次"));
    l3->setObjectName("fldLabel");
    QComboBox* cbShift = new QComboBox;
    for (int s = 1; s <= 3; ++s) cbShift->addItem(def::shiftName(s));

    QLabel* l4 = new QLabel(QStringLiteral("接诊上限"));
    l4->setObjectName("fldLabel");
    QSpinBox* spMax = new QSpinBox;
    spMax->setRange(1, 50);
    spMax->setValue(20);

    bv->addWidget(l1); bv->addWidget(cbDoc);
    bv->addWidget(l2); bv->addWidget(cbDay);
    bv->addWidget(l3); bv->addWidget(cbShift);
    bv->addWidget(l4); bv->addWidget(spMax);

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认添加"), QStringLiteral("btnPri"));
    // 弹窗骨架
    dlg.setFixedWidth(560);
    dlg.setObjectName("modal");
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    QWidget* hd = new QWidget; hd->setObjectName("mHead");
    QHBoxLayout* hl = new QHBoxLayout(hd);
    hl->setContentsMargins(18, 10, 18, 10);
    QLabel* t = new QLabel(QStringLiteral("添加排班")); t->setObjectName("mTitle");
    hl->addWidget(t); hl->addStretch();
    v->addWidget(hd);
    QWidget* bb = new QWidget; bb->setObjectName("mBody");
    QVBoxLayout* bbv = new QVBoxLayout(bb);
    bbv->setContentsMargins(18, 12, 18, 12);
    bbv->addWidget(body);
    v->addWidget(bb, 1);
    QWidget* ft = new QWidget; ft->setObjectName("mFoot");
    QHBoxLayout* fl = new QHBoxLayout(ft);
    fl->setContentsMargins(18, 10, 18, 10);
    fl->addStretch();
    QList<QPushButton*> made;
    foreach (auto b, btns) {
        QPushButton* pb = new QPushButton(b.first);
        pb->setObjectName(b.second);
        pb->setCursor(Qt::PointingHandCursor);
        fl->addWidget(pb);
        made.append(pb);
    }
    v->addWidget(ft);

    dlg.connect(made[0], &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(made[1], &QPushButton::clicked, &dlg, [&]() {
        const QString doc = cbDoc->currentText();
        const int day = cbDay->currentIndex();
        const int sh  = cbShift->currentIndex() + 1;
        // 冲突检测：同医生同日同班已存在 → 拒绝（应用层 COUNT 语义）
        for (const QString& c : m_allCells) {
            const QStringList p = c.split(QLatin1Char('|'));
            if (p.size() >= 3 && p[0] == doc && p[1].toInt() == day && p[2].toInt() == sh) {
                Toast::show(&dlg, QStringLiteral("该医生此时段已有排班，请勿重复"), true);
                return;
            }
        }
        const QString cellText = QStringLiteral("%1|%2|%3|0/%4")
                                 .arg(doc).arg(day).arg(sh).arg(spMax->value());
        m_allCells << cellText;
        g_cells = m_allCells;
        Toast::show(window(), QStringLiteral("排班已添加：%1 %2 %3（上限 %4）")
                    .arg(doc, cbDay->currentText(), cbShift->currentText())
                    .arg(spMax->value()));
        dlg.accept();
        rebuild();
    });
    dlg.exec();
}
