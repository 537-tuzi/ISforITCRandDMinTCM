#include "DiagnoseView.h"
#include "util/Toast.h"
#include "util/UIFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFrame>
#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

DiagnoseView::DiagnoseView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageDiag");
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(10);

    // ===== 左上：患者信息（3.4.4 图例：手机号完整显示） =====
    QFrame* pc = new QFrame;
    pc->setObjectName("card");
    QVBoxLayout* pv = new QVBoxLayout(pc);
    pv->setContentsMargins(0, 0, 0, 0);
    QLabel* pt = new QLabel(QStringLiteral("患者信息"));
    pt->setObjectName("chTitle");
    pt->setContentsMargins(14, 12, 14, 8);
    pv->addWidget(pt);
    QWidget* pb = new QWidget;
    QGridLayout* pbg = new QGridLayout(pb);
    pbg->setContentsMargins(14, 2, 14, 10);
    pbg->setHorizontalSpacing(16);
    pbg->addWidget(uif::makeFieldRow(QStringLiteral("姓名"), QStringLiteral("张三"), QStringLiteral("liRow")), 0, 0);
    pbg->addWidget(uif::makeFieldRow(QStringLiteral("性别"), QStringLiteral("男"), QStringLiteral("liRow")), 0, 1);
    pbg->addWidget(uif::makeFieldRow(QStringLiteral("年龄"), QStringLiteral("45"), QStringLiteral("liRow")), 1, 0);
    pbg->addWidget(uif::makeFieldRow(QStringLiteral("医保卡号"), QStringLiteral("MP20261024001"), QStringLiteral("liRow")), 1, 1);
    pbg->addWidget(uif::makeFieldRow(QStringLiteral("手机号"), QStringLiteral("13900000001"), QStringLiteral("liRow")), 2, 0);
    pbg->addWidget(uif::makeFieldRow(QStringLiteral("过敏史"), QStringLiteral("青霉素"), QStringLiteral("liRow")), 2, 1);
    pv->addWidget(pb, 1);
    grid->addWidget(pc, 0, 0);

    // ===== 左下：舌苔检测报告（含照片占位） =====
    QFrame* tc = new QFrame;
    tc->setObjectName("card");
    QVBoxLayout* tv = new QVBoxLayout(tc);
    tv->setContentsMargins(0, 0, 0, 0);
    QLabel* tt = new QLabel(QStringLiteral("舌苔检测报告"));
    tt->setObjectName("chTitle");
    tt->setContentsMargins(14, 12, 14, 8);
    tv->addWidget(tt);
    QWidget* tb = new QWidget;
    QVBoxLayout* tbv = new QVBoxLayout(tb);
    tbv->setContentsMargins(14, 2, 14, 12);
    tbv->setSpacing(8);
    tbv->addWidget(uif::makeFieldRow(QStringLiteral("舌苔类型"), QStringLiteral("白腻苔"), QStringLiteral("liRow")));
    tbv->addWidget(uif::makeFieldRow(QStringLiteral("健康状态"), QStringLiteral("身体有恙（建议就诊）"),
                                     QStringLiteral("liRow"), QStringLiteral("liValRed")));
    QFrame* ph = new QFrame;
    ph->setObjectName("phCardDark");
    ph->setMinimumHeight(110);
    QVBoxLayout* phv = new QVBoxLayout(ph);
    QLabel* img = new QLabel(QStringLiteral("👅"));
    img->setAlignment(Qt::AlignCenter);
    phv->addWidget(img, 1);
    QLabel* imgCap = new QLabel(QStringLiteral("检测时间：2026-10-20 09:12 · 置信度 93%"));
    imgCap->setObjectName("chSubDark");
    imgCap->setAlignment(Qt::AlignCenter);
    phv->addWidget(imgCap);
    tbv->addWidget(ph, 1);
    tv->addWidget(tb, 1);
    grid->addWidget(tc, 1, 0);

    // ===== 右上：问诊记录 =====
    QFrame* vc = new QFrame;
    vc->setObjectName("card");
    QVBoxLayout* vv = new QVBoxLayout(vc);
    vv->setContentsMargins(0, 0, 0, 0);
    QLabel* vt = new QLabel(QStringLiteral("问诊记录"));
    vt->setObjectName("chTitle");
    vt->setContentsMargins(14, 12, 14, 8);
    vv->addWidget(vt);
    QWidget* vb = new QWidget;
    QVBoxLayout* vbv = new QVBoxLayout(vb);
    vbv->setContentsMargins(14, 2, 14, 12);
    vbv->setSpacing(8);
    QLabel* l1 = new QLabel(QStringLiteral("主诉"));
    l1->setObjectName("fldLabel");
    m_chief = new QTextEdit;
    m_chief->setPlaceholderText(QStringLiteral("请输入主诉与四诊信息…"));
    m_chief->setFixedHeight(76);
    QLabel* l2 = new QLabel(QStringLiteral("中医诊断"));
    l2->setObjectName("fldLabel");
    m_diag = new QLineEdit;
    m_diag->setPlaceholderText(QStringLiteral("请输入中医诊断，如：脾虚湿盛证"));
    vbv->addWidget(l1); vbv->addWidget(m_chief);
    vbv->addWidget(l2); vbv->addWidget(m_diag);
    vbv->addStretch();
    vv->addWidget(vb, 1);
    grid->addWidget(vc, 0, 1);

    // ===== 右下：中医处方（批量药材） =====
    QFrame* rc = new QFrame;
    rc->setObjectName("card");
    QVBoxLayout* rv = new QVBoxLayout(rc);
    rv->setContentsMargins(0, 0, 0, 0);
    QLabel* rt = new QLabel(QStringLiteral("中医处方"));
    rt->setObjectName("chTitle");
    rt->setContentsMargins(14, 12, 14, 8);
    rv->addWidget(rt);
    QWidget* rb = new QWidget;
    QVBoxLayout* rbv = new QVBoxLayout(rb);
    rbv->setContentsMargins(14, 2, 14, 12);
    rbv->setSpacing(8);

    m_rx = new QTableWidget(0, 3);
    m_rx->setObjectName("tb");
    m_rx->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("药材") << QStringLiteral("剂量") << QStringLiteral("服用方法"));
    m_rx->verticalHeader()->hide();
    m_rx->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_rx->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rx->setShowGrid(false);
    m_rx->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_rx->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    rbv->addWidget(m_rx, 1);

    QWidget* addRow = new QWidget;
    QHBoxLayout* ah = new QHBoxLayout(addRow);
    ah->setContentsMargins(0, 0, 0, 0);
    ah->setSpacing(8);
    m_inName = new QLineEdit;  m_inName->setPlaceholderText(QStringLiteral("药材"));
    m_inDose = new QLineEdit;  m_inDose->setPlaceholderText(QStringLiteral("剂量"));
    m_inUse  = new QLineEdit;  m_inUse->setPlaceholderText(QStringLiteral("服用方法"));
    QPushButton* add = new QPushButton(QStringLiteral("添加药材"));
    add->setObjectName("btnOutSm");
    add->setCursor(Qt::PointingHandCursor);
    QPushButton* clr = new QPushButton(QStringLiteral("清空处方"));
    clr->setObjectName("btnLink");
    clr->setCursor(Qt::PointingHandCursor);
    ah->addWidget(m_inName, 2); ah->addWidget(m_inDose, 1);
    ah->addWidget(m_inUse, 3);  ah->addWidget(add); ah->addWidget(clr);
    rbv->addWidget(addRow);
    rv->addWidget(rb, 1);
    grid->addWidget(rc, 1, 1);

    grid->setColumnStretch(0, 4);
    grid->setColumnStretch(1, 5);
    root->addLayout(grid, 1);

    // ===== 底部操作 =====
    QWidget* acts = new QWidget;
    QHBoxLayout* kh = new QHBoxLayout(acts);
    kh->setContentsMargins(0, 0, 0, 0);
    kh->setSpacing(10);
    QPushButton* draft = new QPushButton(QStringLiteral("保存草稿"));
    draft->setObjectName("btnOut");
    draft->setCursor(Qt::PointingHandCursor);
    QPushButton* submit = new QPushButton(QStringLiteral("提交并生成病例"));
    submit->setObjectName("btnPriLg");
    submit->setCursor(Qt::PointingHandCursor);
    kh->addWidget(draft);
    kh->addWidget(submit);
    kh->addStretch();
    root->addWidget(acts);

    connect(add, &QPushButton::clicked, this, &DiagnoseView::onAddRx);
    connect(clr, &QPushButton::clicked, this, &DiagnoseView::onClearRx);
    connect(draft, &QPushButton::clicked, this, &DiagnoseView::onSaveDraft);
    connect(submit, &QPushButton::clicked, this, &DiagnoseView::onSubmit);

    // 初始演示处方 2 味
    m_inName->setText(QStringLiteral("参苓白术散"));
    m_inDose->setText(QStringLiteral("10g"));
    m_inUse->setText(QStringLiteral("水煎服，每日一剂"));
    onAddRx();
    m_inName->setText(QStringLiteral("薏米"));
    m_inDose->setText(QStringLiteral("30g"));
    m_inUse->setText(QStringLiteral("煮水代茶饮"));
    onAddRx();
    m_inName->clear(); m_inDose->clear(); m_inUse->clear();
}

void DiagnoseView::onAddRx()
{
    if (m_inName->text().trimmed().isEmpty() || m_inDose->text().trimmed().isEmpty()
        || m_inUse->text().trimmed().isEmpty()) {
        Toast::show(window(), QStringLiteral("请完整填写药材、剂量和服用方法"), true);
        return;
    }
    const int r = m_rx->rowCount();
    m_rx->insertRow(r);
    m_rx->setRowHeight(r, 42);
    const QString vals[3] = { m_inName->text().trimmed(), m_inDose->text().trimmed(),
                              m_inUse->text().trimmed() };
    for (int c = 0; c < 3; ++c) {
        QTableWidgetItem* it = new QTableWidgetItem(vals[c]);
        it->setTextAlignment(Qt::AlignCenter);
        m_rx->setItem(r, c, it);
    }
    m_inName->clear(); m_inDose->clear(); m_inUse->clear();
    m_inName->setFocus();
}

void DiagnoseView::onClearRx()
{
    if (m_rx->rowCount() == 0) return;
    QDialog dlg(window());
    dlg.setObjectName("modal");
    QLabel* body = new QLabel(QStringLiteral("确认清空全部 %1 味药材？").arg(m_rx->rowCount()));
    body->setObjectName("mText");
    body->setWordWrap(true);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    v->addWidget(body);
    QHBoxLayout* h = new QHBoxLayout;
    h->addStretch();
    QPushButton* no = new QPushButton(QStringLiteral("再想想"));
    no->setObjectName("btnOut");
    QPushButton* yes = new QPushButton(QStringLiteral("确认清空"));
    yes->setObjectName("btnDangerO");
    h->addWidget(no); h->addWidget(yes);
    v->addLayout(h);
    dlg.setFixedWidth(480);
    dlg.connect(no, &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(yes, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() == QDialog::Accepted) {
        m_rx->setRowCount(0);
        Toast::show(window(), QStringLiteral("处方已清空"));
    }
}

void DiagnoseView::onSaveDraft()
{
    Toast::show(window(), QStringLiteral("草稿已保存"));
}

void DiagnoseView::onSubmit()
{
    if (m_diag->text().trimmed().isEmpty()) {
        Toast::show(window(), QStringLiteral("请先填写诊断"), true);
        return;
    }
    if (m_rx->rowCount() < 1) {
        Toast::show(window(), QStringLiteral("请至少添加一条药材"), true);
        return;
    }
    QDialog dlg(window());
    dlg.setObjectName("modal");
    QLabel* body = new QLabel(QStringLiteral("确认提交并生成标准化病例？\n病例将关联患者信息、舌苔检测结果与处方，同步至数据库。"));
    body->setObjectName("mText");
    body->setWordWrap(true);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    v->addWidget(body);
    QHBoxLayout* h = new QHBoxLayout;
    h->addStretch();
    QPushButton* no = new QPushButton(QStringLiteral("再想想"));
    no->setObjectName("btnOut");
    QPushButton* yes = new QPushButton(QStringLiteral("确认提交"));
    yes->setObjectName("btnPri");
    h->addWidget(no); h->addWidget(yes);
    v->addLayout(h);
    dlg.setWindowTitle(QStringLiteral("确认提交"));
    dlg.setFixedWidth(480);
    dlg.connect(no, &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(yes, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() == QDialog::Accepted) {
        Toast::show(window(), QStringLiteral("病例已生成：BL20261030%1，已同步至患者档案")
                    .arg(QString::number(m_rxSeq++).rightJustified(3, QLatin1Char('0'))));
    }
}
