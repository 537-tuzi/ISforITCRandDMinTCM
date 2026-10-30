#include "HistoryView.h"
#include "util/Toast.h"
#include "util/UIFactory.h"
#include "util/UiScale.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDate>

namespace {

// 演示处方药材行简写（三段对应 RxItem 的 name/dosage/usage）
sdt::RxItem mkRx(const QString& name, const QString& dosage, const QString& usage)
{
    sdt::RxItem it;
    it.name = name;
    it.dosage = dosage;
    it.usage = usage;
    return it;
}

// 病例详情弹窗：标题"病例详情·<病例号>"，字段 + 处方逐行 + 底部【关闭】。
// 仅本页使用，内聚在 cpp；骨架对齐病人端弹窗（mHead/mBody/mFoot）。
class CaseDialog : public QDialog
{
public:
    CaseDialog(const QString& no, const sdt::VisitCase& c, QWidget* parent)
        : QDialog(parent)
    {
        const QString title = QStringLiteral("病例详情·") + no;
        setWindowTitle(QStringLiteral("病例详情"));
        setObjectName("modal");
        setFixedWidth(qRound(560 * UiScale::current()));   // 随窗口缩放放大

        QVBoxLayout* v = new QVBoxLayout(this);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);

        // ---- 标题栏 ----
        QWidget* head = new QWidget;
        head->setObjectName("mHead");
        QHBoxLayout* hl = new QHBoxLayout(head);
        hl->setContentsMargins(18, 10, 18, 10);
        QLabel* t = new QLabel(title);
        t->setObjectName("mTitle");
        hl->addWidget(t);
        hl->addStretch();
        v->addWidget(head);

        // ---- 正文：基本信息（顺序：患者/科室/诊疗日期/诊断/医生）+ 处方 ----
        QWidget* body = new QWidget;
        body->setObjectName("mBody");
        QVBoxLayout* bv = new QVBoxLayout(body);
        bv->setContentsMargins(18, 12, 18, 12);
        bv->setSpacing(4);
        bv->addWidget(uif::makeFieldRow(QStringLiteral("患者"), c.patient, QStringLiteral("liRow")));
        bv->addWidget(uif::makeFieldRow(QStringLiteral("科室"), c.dept, QStringLiteral("liRow")));
        bv->addWidget(uif::makeFieldRow(QStringLiteral("诊疗日期"), c.date, QStringLiteral("liRow")));
        bv->addWidget(uif::makeFieldRow(QStringLiteral("诊断"), c.diagnosis, QStringLiteral("liRow")));
        bv->addWidget(uif::makeFieldRow(QStringLiteral("医生"), QStringLiteral("张主任"), QStringLiteral("liRow")));

        QLabel* cap = new QLabel(QStringLiteral("处方（共 %1 味）").arg(c.rx.size()));
        cap->setObjectName("chSub");
        bv->addSpacing(6);
        bv->addWidget(cap);

        // 处方逐行："· 药材 剂量——用法"
        QStringList lines;
        for (const sdt::RxItem& it : c.rx) {
            lines << QStringLiteral("· ") + it.name + QLatin1Char(' ') + it.dosage
                     + QStringLiteral("——") + it.usage;
        }
        QLabel* rx = new QLabel(lines.join(QLatin1Char('\n')));
        rx->setObjectName("mText");
        rx->setWordWrap(true);
        bv->addWidget(rx);
        v->addWidget(body, 1);

        // ---- 底部：关闭 ----
        QWidget* foot = new QWidget;
        foot->setObjectName("mFoot");
        QHBoxLayout* fl = new QHBoxLayout(foot);
        fl->setContentsMargins(18, 10, 18, 10);
        fl->addStretch();
        QPushButton* close = new QPushButton(QStringLiteral("关闭"));
        close->setObjectName("btnOut");
        close->setCursor(Qt::PointingHandCursor);
        fl->addWidget(close);
        v->addWidget(foot);

        connect(close, &QPushButton::clicked, this, &QDialog::accept);
    }
};

} // namespace

HistoryView::HistoryView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageHist");
    buildDemoData();

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(12);

    // ===== 筛选卡：患者姓名 + 诊疗时间下拉 + 查询/重置 =====
    QFrame* fc = new QFrame;
    fc->setObjectName("card");
    QHBoxLayout* fh = new QHBoxLayout(fc);
    fh->setContentsMargins(16, 12, 16, 12);
    fh->setSpacing(10);

    QLabel* nl = new QLabel(QStringLiteral("患者姓名"));
    nl->setObjectName("fldLabel");
    m_name = new QLineEdit;
    m_name->setPlaceholderText(QStringLiteral("按姓名筛选"));
    m_name->setClearButtonEnabled(true);   // 输入框尾一键清空
    m_name->setMinimumWidth(140);

    QLabel* tl = new QLabel(QStringLiteral("诊疗时间"));
    tl->setObjectName("fldLabel");
    m_time = new QComboBox;
    m_time->addItems(QStringList() << QStringLiteral("全部")
                                   << QStringLiteral("近一周")
                                   << QStringLiteral("近一月")
                                   << QStringLiteral("近三月"));

    QPushButton* query = new QPushButton(QStringLiteral("查询"));
    query->setObjectName("btnOut");
    query->setCursor(Qt::PointingHandCursor);
    QPushButton* reset = new QPushButton(QStringLiteral("重置"));
    reset->setObjectName("btnOut");
    reset->setCursor(Qt::PointingHandCursor);

    fh->addWidget(nl);
    fh->addWidget(m_name, 1);   // 姓名框吃掉多余宽度
    fh->addWidget(tl);
    fh->addWidget(m_time);
    fh->addSpacing(8);
    fh->addWidget(query);
    fh->addWidget(reset);
    root->addWidget(fc);

    // ===== 病例列表卡 =====
    QFrame* lc = new QFrame;
    lc->setObjectName("card");
    QVBoxLayout* lv = new QVBoxLayout(lc);
    lv->setContentsMargins(0, 0, 0, 0);
    QLabel* title = new QLabel(QStringLiteral("历史诊疗病例"));
    title->setObjectName("chTitle");
    title->setContentsMargins(14, 12, 14, 12);
    lv->addWidget(title);

    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(14, 2, 14, 10);
    bv->setSpacing(0);

    m_table = new QTableWidget(0, 7);
    m_table->setObjectName("tb");
    QStringList heads;
    heads << QStringLiteral("病例号") << QStringLiteral("患者") << QStringLiteral("科室")
          << QStringLiteral("诊疗日期") << QStringLiteral("诊断") << QStringLiteral("药材数")
          << QStringLiteral("操作");
    m_table->setHorizontalHeaderLabels(heads);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(false);
    // 前 6 列按内容收缩，最右"操作"列吃掉剩余宽度（同病人端记录页）
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    bv->addWidget(m_table, 1);

    m_count = new QLabel;
    m_count->setObjectName("pagerTxt");
    m_count->setAlignment(Qt::AlignCenter);
    bv->addSpacing(8);
    bv->addWidget(m_count);
    lv->addWidget(body, 1);
    root->addWidget(lc, 1);

    connect(query, &QPushButton::clicked, this, &HistoryView::onQuery);
    connect(m_name, &QLineEdit::returnPressed, this, &HistoryView::onQuery);   // 回车即查询
    connect(reset, &QPushButton::clicked, this, &HistoryView::onReset);

    refreshTable();   // 首次进入展示全量 6 条
}

/* 演示数据 7 条（表5第5行口径；科室"…"同前列=中医内科，仅李四 2026-09-28 为针灸推拿科；
   最新一条 2026-10-28 保证"近一周"筛选（演示基准日 10-30）有命中） */
void HistoryView::buildDemoData()
{
    CaseRow r;

    r.no = QStringLiteral("BL20261028007");
    r.data.patient = QStringLiteral("张三");
    r.data.dept = QStringLiteral("中医内科");
    r.data.date = QStringLiteral("2026-10-28");
    r.data.diagnosis = QStringLiteral("湿邪困脾（复诊）");
    r.data.rx << mkRx(QStringLiteral("参苓白术散"), QStringLiteral("10g"), QStringLiteral("水煎服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("炒薏米"), QStringLiteral("30g"), QStringLiteral("煮水代茶饮"));
    m_all.append(r);

    r = CaseRow();
    r.no = QStringLiteral("BL20261020001");
    r.data.patient = QStringLiteral("张三");
    r.data.dept = QStringLiteral("中医内科");
    r.data.date = QStringLiteral("2026-10-20");
    r.data.diagnosis = QStringLiteral("脾虚湿盛证");   // 参苓白术散加减
    r.data.rx << mkRx(QStringLiteral("党参"), QStringLiteral("15g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("白术"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("茯苓"), QStringLiteral("15g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("山药"), QStringLiteral("15g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("薏苡仁"), QStringLiteral("20g"), QStringLiteral("水煎内服，每日一剂，分两次温服"));
    m_all.append(r);

    r = CaseRow();
    r.no = QStringLiteral("BL20261013002");
    r.data.patient = QStringLiteral("张三");
    r.data.dept = QStringLiteral("中医内科");
    r.data.date = QStringLiteral("2026-10-13");
    r.data.diagnosis = QStringLiteral("脾胃失调（轻）");
    r.data.rx << mkRx(QStringLiteral("香砂六君丸"), QStringLiteral("6g"), QStringLiteral("口服，每日三次，饭前温水送服"))
              << mkRx(QStringLiteral("陈皮"), QStringLiteral("6g"), QStringLiteral("开水冲泡代茶饮，每日一剂"));
    m_all.append(r);

    r = CaseRow();
    r.no = QStringLiteral("BL20260928003");
    r.data.patient = QStringLiteral("李四");
    r.data.dept = QStringLiteral("针灸推拿科");
    r.data.date = QStringLiteral("2026-09-28");
    r.data.diagnosis = QStringLiteral("湿阻经络证");   // 薏苡仁汤加减，配合针灸
    r.data.rx << mkRx(QStringLiteral("独活"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂"))
              << mkRx(QStringLiteral("羌活"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂"))
              << mkRx(QStringLiteral("薏苡仁"), QStringLiteral("30g"), QStringLiteral("水煎内服，每日一剂"));
    m_all.append(r);

    r = CaseRow();
    r.no = QStringLiteral("BL20260915004");
    r.data.patient = QStringLiteral("王五");
    r.data.dept = QStringLiteral("中医内科");
    r.data.date = QStringLiteral("2026-09-15");
    r.data.diagnosis = QStringLiteral("肝郁气滞证");   // 柴胡疏肝散加减
    r.data.rx << mkRx(QStringLiteral("柴胡"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("香附"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("川芎"), QStringLiteral("6g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("白芍"), QStringLiteral("15g"), QStringLiteral("水煎内服，每日一剂，分两次温服"));
    m_all.append(r);

    r = CaseRow();
    r.no = QStringLiteral("BL20260830005");
    r.data.patient = QStringLiteral("李四");
    r.data.dept = QStringLiteral("中医内科");
    r.data.date = QStringLiteral("2026-08-30");
    r.data.diagnosis = QStringLiteral("气血两虚证");   // 八珍汤加减
    r.data.rx << mkRx(QStringLiteral("党参"), QStringLiteral("15g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("白术"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("茯苓"), QStringLiteral("12g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("当归"), QStringLiteral("10g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("川芎"), QStringLiteral("6g"), QStringLiteral("水煎内服，每日一剂，分两次温服"))
              << mkRx(QStringLiteral("熟地黄"), QStringLiteral("15g"), QStringLiteral("水煎内服，每日一剂，分两次温服"));
    m_all.append(r);

    r = CaseRow();
    r.no = QStringLiteral("BL20260812006");
    r.data.patient = QStringLiteral("赵六");
    r.data.dept = QStringLiteral("中医内科");
    r.data.date = QStringLiteral("2026-08-12");
    r.data.diagnosis = QStringLiteral("风寒袭肺证");   // 杏苏散加减
    r.data.rx << mkRx(QStringLiteral("麻黄"), QStringLiteral("6g"), QStringLiteral("水煎温服，每日一剂"))
              << mkRx(QStringLiteral("桂枝"), QStringLiteral("9g"), QStringLiteral("水煎温服，每日一剂"))
              << mkRx(QStringLiteral("杏仁"), QStringLiteral("9g"), QStringLiteral("水煎温服，每日一剂"));
    m_all.append(r);
}

/* 按当前条件重填表格：姓名=包含匹配（空=不限）；时间下拉以当天为锚（1/2/3=周/月/季） */
void HistoryView::refreshTable()
{
    const QString name = m_name->text().trimmed();
    QDate from;                       // 无效日期=不限时间
    switch (m_time->currentIndex()) {
    case 1: from = QDate::currentDate().addDays(-7);   break;   // 近一周
    case 2: from = QDate::currentDate().addMonths(-1); break;   // 近一月
    case 3: from = QDate::currentDate().addMonths(-3); break;   // 近三月
    default: break;                                    // 全部
    }

    m_rows.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        if (!name.isEmpty() && !m_all.at(i).data.patient.contains(name, Qt::CaseInsensitive))
            continue;
        if (from.isValid()
            && QDate::fromString(m_all.at(i).data.date, QStringLiteral("yyyy-MM-dd")) < from)
            continue;
        m_rows.append(i);
    }

    m_table->clearContents();
    m_table->setRowCount(m_rows.size());
    for (int row = 0; row < m_rows.size(); ++row) {
        const CaseRow& cr = m_all.at(m_rows.at(row));
        m_table->setRowHeight(row, 52);   // 行高固定 52（规格）
        const QString cols[6] = {
            cr.no, cr.data.patient, cr.data.dept,
            cr.data.date, cr.data.diagnosis, QString::number(cr.data.rx.size())
        };
        for (int c = 0; c < 6; ++c) {
            QTableWidgetItem* it = new QTableWidgetItem(cols[c]);
            it->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, c, it);
        }
        // 操作列：【查看详情】
        QWidget* btnHost = new QWidget;
        QHBoxLayout* bh = new QHBoxLayout(btnHost);
        bh->setContentsMargins(2, 2, 2, 2);
        QPushButton* b = new QPushButton(QStringLiteral("查看详情"));
        b->setObjectName("btnOutSm");
        b->setCursor(Qt::PointingHandCursor);
        bh->addWidget(b);
        m_table->setCellWidget(row, 6, btnHost);
        const int idx = m_rows.at(row);
        connect(b, &QPushButton::clicked, this, [this, idx] { openDetail(idx); });
    }

    m_count->setText(QStringLiteral("共 %1 条").arg(m_rows.size()));
}

/* 查询：重算结果；命中为空时提示（红 toast） */
void HistoryView::onQuery()
{
    refreshTable();
    if (m_rows.isEmpty())
        Toast::show(window(), QStringLiteral("未找到符合条件的病例"), true);
}

/* 重置：清空姓名、时间回到"全部"，静默回到全量列表 */
void HistoryView::onReset()
{
    m_name->clear();
    m_time->setCurrentIndex(0);
    refreshTable();
}

/* 病例详情弹窗（弹出式，模态） */
void HistoryView::openDetail(int index)
{
    if (index < 0 || index >= m_all.size()) return;
    CaseDialog d(m_all.at(index).no, m_all.at(index).data, window());
    d.exec();
}
