#include "Dialogs.h"
#include "util/UiScale.h"
#include "util/UIFactory.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QList>
#include <QPair>

// ---------- 通用弹窗骨架 ----------
// 返回创建的底部按钮（按传入顺序），点击行为由调用方显式接线；titleRed=标题朱砂色（MD §8.2）
// 关闭走系统标题栏按钮，不在界面内自绘（2026-10-30 决定）
static QList<QPushButton*> setupModal(QDialog* dlg, const QString& title, QWidget* body,
                                      const QList<QPair<QString, QString>>& buttons,
                                      bool titleRed = false)
{
    dlg->setFixedWidth(qRound(def::MODAL_W * UiScale::current()));   // 随窗口缩放放大
    dlg->setObjectName("modal");
    QVBoxLayout* v = new QVBoxLayout(dlg);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    QWidget* head = new QWidget;
    head->setObjectName("mHead");
    QHBoxLayout* hl = new QHBoxLayout(head);
    hl->setContentsMargins(18, 10, 18, 10);
    QLabel* t = new QLabel(title);
    t->setObjectName(titleRed ? QStringLiteral("mTitleRed") : QStringLiteral("mTitle"));
    hl->addWidget(t);
    hl->addStretch();
    v->addWidget(head);

    QWidget* b = new QWidget;
    b->setObjectName("mBody");
    QVBoxLayout* bv = new QVBoxLayout(b);
    bv->setContentsMargins(18, 12, 18, 12);
    bv->addWidget(body);
    v->addWidget(b, 1);

    QWidget* foot = new QWidget;
    foot->setObjectName("mFoot");
    QHBoxLayout* fl = new QHBoxLayout(foot);
    fl->setContentsMargins(18, 10, 18, 10);
    fl->addStretch();
    QList<QPushButton*> made;
    foreach (auto btn, buttons) {   // 文本,objectName
        QPushButton* pb = new QPushButton(btn.first);
        pb->setObjectName(btn.second);
        pb->setCursor(Qt::PointingHandCursor);
        fl->addWidget(pb);
        made.append(pb);
    }
    v->addWidget(foot);
    return made;
}

// 在 setupModal 返回的按钮列表中按 objectName 取按钮
static QPushButton* findBtn(const QList<QPushButton*>& list, const char* name)
{
    foreach (QPushButton* b, list)
        if (b->objectName() == QLatin1String(name)) return b;
    return nullptr;
}

// ---------- "标签: 值"明细行改用 uif::makeFieldRow（util/UIFactory） ----------

// ---------- 8.1 手动预约 ----------
ApptDialog::ApptDialog(const pd::Sched& sched, QWidget* parent)
    : QDialog(parent), m_sched(sched)
{
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    QGridLayout* g = new QGridLayout;
    g->setHorizontalSpacing(12);
    g->setVerticalSpacing(8);

    struct Fld { const char* label; QComboBox** cb; };
    Fld flds[] = {
        {"第一步·选科室", &m_dep},
        {"第二步·选医生", &m_doc},
        {"第三步·选日期", &m_day},
        {"第四步·选时段", &m_time},
    };
    for (int i = 0; i < 4; ++i) {
        QLabel* l = new QLabel(QString::fromUtf8(flds[i].label));
        l->setObjectName("fldLabel");
        QComboBox* c = new QComboBox;
        g->addWidget(l, (i / 2) * 2, i % 2);
        g->addWidget(c, (i / 2) * 2 + 1, i % 2);
        *flds[i].cb = c;
    }
    bv->addLayout(g);

    QLabel* note = new QLabel(QStringLiteral("就诊前 30 分钟之内不能取消预约。"));
    note->setObjectName("noteRed");
    note->setWordWrap(true);
    bv->addSpacing(4);
    bv->addWidget(note);

    fill(m_dep, m_sched.keys(), QStringLiteral("请选择科室"));
    fill(m_doc, {}, QStringLiteral("先选科室"));
    fill(m_day, {}, QStringLiteral("先选医生"));
    fill(m_time, {}, QStringLiteral("先选日期"));

    connect(m_dep, &QComboBox::currentTextChanged, this, [this](const QString& d) {
        fill(m_doc, d.isEmpty() ? QStringList() : m_sched[d].keys(),
             QStringLiteral("请选择医生"));
        fill(m_day, {}, QStringLiteral("先选医生"));
        fill(m_time, {}, QStringLiteral("先选日期"));
    });
    connect(m_doc, &QComboBox::currentTextChanged, this, [this](const QString& doc) {
        const QString d = m_dep->currentText();
        fill(m_day, (d.isEmpty() || doc.isEmpty()) ? QStringList()
                                                   : m_sched[d][doc].keys(),
             QStringLiteral("请选择日期"));
        fill(m_time, {}, QStringLiteral("先选日期"));
    });
    connect(m_day, &QComboBox::currentTextChanged, this, [this](const QString& day) {
        const QString d = m_dep->currentText();
        const QString doc = m_doc->currentText();
        fill(m_time, (d.isEmpty() || doc.isEmpty() || day.isEmpty())
                         ? QStringList() : m_sched[d][doc][day],
             QStringLiteral("请选择时段"));
    });

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认挂号"), QStringLiteral("btnPri"));
    const QList<QPushButton*> made = setupModal(this, QStringLiteral("预约挂号"), body, btns);
    connect(findBtn(made, "btnOut"), &QPushButton::clicked, this, &QDialog::reject);
    // 确认挂号：未选全不关闭
    if (QPushButton* ok = findBtn(made, "btnPri")) {
        connect(ok, &QPushButton::clicked, this, [this] {
            if (result().contains(QString()))
                emit incomplete();
            else
                accept();
        });
    }
}

void ApptDialog::fill(QComboBox* box, const QStringList& items, const QString& ph)
{
    box->blockSignals(true);
    box->clear();
    box->addItem(ph, QString());
    foreach (const QString& i, items) box->addItem(i, i);
    box->blockSignals(false);
}

QStringList ApptDialog::result() const
{
    return QStringList{ m_dep->currentData().toString(), m_doc->currentData().toString(),
                        m_day->currentData().toString(), m_time->currentData().toString() };
}

// ---------- 8.2 自动预约 ----------
AutoApptDialog::AutoApptDialog(const QString& sevType, QWidget* parent) : QDialog(parent)
{
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);

    QLabel* lead = new QLabel(QStringLiteral("您的舌象结果为 ")
                              + QStringLiteral("<span style='color:#b23b2e;font-weight:700;'>")
                              + sevType + QStringLiteral("</span>")
                              + QStringLiteral("（重症），系统已按规则自动预约了最近可就诊的医生："));
    lead->setObjectName("mText");
    lead->setWordWrap(true);
    bv->addWidget(lead);

    QGridLayout* g = new QGridLayout;
    g->setHorizontalSpacing(16);
    g->addWidget(uif::makeFieldRow(QStringLiteral("科室"), QStringLiteral("脾胃科")), 0, 0);
    g->addWidget(uif::makeFieldRow(QStringLiteral("医生"), QStringLiteral("赵国安（主任医师）")), 0, 1);
    g->addWidget(uif::makeFieldRow(QStringLiteral("日期"), QStringLiteral("10-30（周五 · 今天）")), 1, 0);
    g->addWidget(uif::makeFieldRow(QStringLiteral("时段"), QStringLiteral("晚班 19:00-21:00")), 1, 1);
    bv->addLayout(g);

    QLabel* note = new QLabel(QStringLiteral("此预约由系统按重症规则自动生成，不能手动修改；"
                                             "如确实无法就诊，可在首页「待办事项」中取消（就诊前 30 分钟内不可取消）。"));
    note->setObjectName("noteGreen");
    note->setWordWrap(true);
    bv->addWidget(note);

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("稍后处理"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认预约"), QStringLiteral("btnPri"));
    const QList<QPushButton*> made =
        setupModal(this, QStringLiteral("重症提醒 · 系统已自动挂号"), body, btns, true);
    connect(findBtn(made, "btnOut"), &QPushButton::clicked, this, &QDialog::reject);
    connect(findBtn(made, "btnPri"), &QPushButton::clicked, this, &QDialog::accept);
}

// ---------- 8.3 取消预约 ----------
CancelDialog::CancelDialog(const pd::Appointment& a, QWidget* parent) : QDialog(parent)
{
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    QLabel* lead = new QLabel(QStringLiteral("确认要取消这条预约吗？"));
    lead->setObjectName("mText");
    bv->addWidget(lead);
    QGridLayout* g = new QGridLayout;
    g->setHorizontalSpacing(16);
    const QString dateTxt = a.week.isEmpty()
        ? a.date
        : a.date + QStringLiteral("（") + a.week + QStringLiteral("）");
    g->addWidget(uif::makeFieldRow(QStringLiteral("日期"), dateTxt), 0, 0);
    g->addWidget(uif::makeFieldRow(QStringLiteral("时段"), a.slot), 0, 1);
    g->addWidget(uif::makeFieldRow(QStringLiteral("科室医生"), a.doc), 1, 0, 1, 2);
    bv->addLayout(g);

    // 规则提示（MD §8.3 红底）
    QLabel* rule = new QLabel(QStringLiteral("就诊前 30 分钟内不可取消；取消后如需再看，请重新预约。"));
    rule->setObjectName("noteRed");
    rule->setWordWrap(true);
    bv->addSpacing(4);
    bv->addWidget(rule);

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("再想想"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认取消"), QStringLiteral("btnDangerO"));
    const QList<QPushButton*> made = setupModal(this, QStringLiteral("取消预约"), body, btns);
    connect(findBtn(made, "btnOut"), &QPushButton::clicked, this, &QDialog::reject);
    connect(findBtn(made, "btnDangerO"), &QPushButton::clicked, this, &QDialog::accept);
}

// ---------- 8.4 病例报告 ----------
ReportDialog::ReportDialog(const pd::VisitRecord& r, QWidget* parent) : QDialog(parent)
{
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    bv->setSpacing(12);

    QGridLayout* g = new QGridLayout;
    g->setHorizontalSpacing(16);
    g->addWidget(uif::makeFieldRow(QStringLiteral("看病时间"), r.date), 0, 0);
    g->addWidget(uif::makeFieldRow(QStringLiteral("时段"), r.slot), 0, 1);
    g->addWidget(uif::makeFieldRow(QStringLiteral("科室医生"), r.doc), 1, 0);
    g->addWidget(uif::makeFieldRow(QStringLiteral("舌头情况"), r.tongue), 1, 1);
    g->addWidget(uif::makeFieldRow(QStringLiteral("医生诊断"), r.dia), 2, 0);
    g->addWidget(uif::makeFieldRow(QStringLiteral("开的药"), r.rx), 2, 1);
    g->addWidget(uif::makeFieldRow(QStringLiteral("医生嘱咐"), r.ad), 3, 0, 1, 2);
    bv->addLayout(g);

    QHBoxLayout* cards = new QHBoxLayout;
    cards->setSpacing(14);
    QFrame* ph = new QFrame;
    ph->setObjectName("phCardDark");   // 舌头照片深底取景框占位（MD §8.4）
    ph->setMinimumHeight(140);
    QVBoxLayout* phv = new QVBoxLayout(ph);
    QLabel* phTitle = new QLabel(QStringLiteral("舌头照片"));
    phTitle->setObjectName("chSubDark");
    phv->addWidget(phTitle);
    QLabel* img = new QLabel(QStringLiteral("👅"));
    img->setAlignment(Qt::AlignCenter);
    phv->addWidget(img, 1);
    cards->addWidget(ph);

    QFrame* dc = new QFrame;
    dc->setObjectName("phCard");
    dc->setMinimumHeight(140);
    QVBoxLayout* dcv = new QVBoxLayout(dc);
    QLabel* dcTitle = new QLabel(QStringLiteral("检测情况"));
    dcTitle->setObjectName("chSub");
    dcv->addWidget(dcTitle);
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("置信度"), r.conf));
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("健康状态"),
                          QStringLiteral("身体有恙（建议就诊）"), QString(), QStringLiteral("liValRed")));
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("检测时间"), r.when));
    cards->addWidget(dc);
    bv->addLayout(cards);

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("关闭"), QStringLiteral("btnPri"));
    const QList<QPushButton*> made = setupModal(this, QStringLiteral("看病报告"), body, btns);
    connect(findBtn(made, "btnPri"), &QPushButton::clicked, this, &QDialog::accept);
}
