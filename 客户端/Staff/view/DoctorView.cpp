#include "DoctorView.h"
#include "util/Toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QRegularExpression>

/* ==================== 弹窗骨架（Dialogs.cpp setupModal 同款，本文件自备） ==================== */

static QList<QPushButton*> setupModal(QDialog* dlg, const QString& title, QWidget* body,
                                      const QList<QPair<QString, QString>>& buttons)
{
    dlg->setFixedWidth(560);
    dlg->setObjectName("modal");
    QVBoxLayout* v = new QVBoxLayout(dlg);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    QWidget* head = new QWidget;
    head->setObjectName("mHead");
    QHBoxLayout* hl = new QHBoxLayout(head);
    hl->setContentsMargins(18, 10, 18, 10);
    QLabel* t = new QLabel(title);
    t->setObjectName("mTitle");
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
    foreach (auto btn, buttons) {
        QPushButton* pb = new QPushButton(btn.first);
        pb->setObjectName(btn.second);
        pb->setCursor(Qt::PointingHandCursor);
        fl->addWidget(pb);
        made.append(pb);
    }
    v->addWidget(foot);
    return made;
}

/* 确认弹窗：标题+正文+取消/确认（重置密码、删除共用） */
static bool confirmModal(QWidget* parent, const QString& title, const QString& text)
{
    QDialog dlg(parent);
    QLabel* body = new QLabel(text);
    body->setObjectName("mText");
    body->setWordWrap(true);
    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认"), QStringLiteral("btnPri"));
    const QList<QPushButton*> made = setupModal(&dlg, title, body, btns);
    dlg.connect(made.at(0), &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(made.at(1), &QPushButton::clicked, &dlg, &QDialog::accept);
    return dlg.exec() == QDialog::Accepted;
}

/* 医师信息弹窗：cur 为空=新增、非空=编辑（工号只读显示）；校验通过把表单值写回 out 并返回 true */
static bool doctorModal(QWidget* parent, const sdt::DoctorRow* cur, sdt::DoctorRow* out)
{
    QDialog dlg(parent);

    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    bv->setSpacing(10);

    if (cur) {
        // 编辑态：工号系统分配不可改，只读展示
        QFrame* ro = new QFrame;
        ro->setObjectName("liRow");
        QHBoxLayout* rl = new QHBoxLayout(ro);
        rl->setContentsMargins(0, 0, 0, 6);
        rl->setSpacing(8);
        QLabel* k = new QLabel(QStringLiteral("工号"));
        k->setObjectName("liKey");
        QLabel* v = new QLabel(cur->workNo);
        v->setObjectName("liVal");
        rl->addWidget(k);
        rl->addStretch();
        rl->addWidget(v);
        bv->addWidget(ro);
    }

    QLabel* lName = new QLabel(QStringLiteral("姓名"));
    lName->setObjectName("fldLabel");
    QLineEdit* edName = new QLineEdit;
    edName->setPlaceholderText(QStringLiteral("请输入姓名"));
    if (cur) edName->setText(cur->name);
    bv->addWidget(lName);
    bv->addWidget(edName);

    QLabel* lDept = new QLabel(QStringLiteral("科室"));
    lDept->setObjectName("fldLabel");
    QComboBox* cbDept = new QComboBox;
    cbDept->addItems(QStringList() << QStringLiteral("中医内科") << QStringLiteral("针灸推拿科"));
    if (cur) cbDept->setCurrentText(cur->dept);
    bv->addWidget(lDept);
    bv->addWidget(cbDept);

    QLabel* lTitle = new QLabel(QStringLiteral("职称"));
    lTitle->setObjectName("fldLabel");
    QComboBox* cbTitle = new QComboBox;
    cbTitle->addItems(QStringList() << QStringLiteral("主任医师") << QStringLiteral("副主任医师")
                                    << QStringLiteral("主治医师") << QStringLiteral("医师"));
    if (cur) cbTitle->setCurrentText(cur->title);
    bv->addWidget(lTitle);
    bv->addWidget(cbTitle);

    QLabel* lPhone = new QLabel(QStringLiteral("联系方式"));
    lPhone->setObjectName("fldLabel");
    QLineEdit* edPhone = new QLineEdit;
    edPhone->setPlaceholderText(QStringLiteral("11 位手机号"));
    if (cur) edPhone->setText(cur->phone);
    bv->addWidget(lPhone);
    bv->addWidget(edPhone);

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(cur ? QStringLiteral("确认保存") : QStringLiteral("确认新增"),
                      QStringLiteral("btnPri"));
    const QList<QPushButton*> made = setupModal(&dlg,
        cur ? QStringLiteral("编辑医师") : QStringLiteral("新增医师"), body, btns);
    dlg.connect(made.at(0), &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(made.at(1), &QPushButton::clicked, &dlg, [&dlg, edName, cbDept, cbTitle, edPhone, out] {
        if (edName->text().trimmed().isEmpty()) {
            Toast::show(&dlg, QStringLiteral("请输入姓名"), true);
            return;
        }
        const QString phone = edPhone->text().trimmed();
        static const QRegularExpression re(QStringLiteral("^\\d{11}$"));
        if (!re.match(phone).hasMatch()) {
            Toast::show(&dlg, QStringLiteral("请输入 11 位手机号"), true);
            return;
        }
        out->name  = edName->text().trimmed();
        out->dept  = cbDept->currentText();
        out->title = cbTitle->currentText();
        out->phone = phone;
        dlg.accept();
    });
    return dlg.exec() == QDialog::Accepted;
}

/* ==================== 表格辅助 ==================== */

/* 只读单元格（禁止编辑，仅可选择） */
static void mkItem(QTableWidget* tb, int row, int col, const QString& text)
{
    QTableWidgetItem* it = new QTableWidgetItem(text);
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tb->setItem(row, col, it);
}

/* 操作列文字链接 */
static QPushButton* mkLink(const QString& text)
{
    QPushButton* b = new QPushButton(text);
    b->setObjectName("btnLink");
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

/* ==================== 页面 ==================== */

DoctorView::DoctorView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageDocs");
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 16);
    root->setSpacing(12);

    // ===== 工具栏：标题 + 搜索框/搜索 + 新增医师 =====
    QHBoxLayout* bar = new QHBoxLayout;
    bar->setSpacing(10);
    QLabel* title = new QLabel(QStringLiteral("医师档案"));
    title->setObjectName("chTitle");
    bar->addWidget(title);

    m_edSearch = new QLineEdit;
    m_edSearch->setPlaceholderText(QStringLiteral("按姓名/工号模糊搜索"));
    m_edSearch->setFixedWidth(260);
    m_edSearch->setClearButtonEnabled(true);
    bar->addWidget(m_edSearch);

    QPushButton* btnSearch = new QPushButton(QStringLiteral("搜索"));
    btnSearch->setObjectName("btnOutSm");
    btnSearch->setCursor(Qt::PointingHandCursor);
    bar->addWidget(btnSearch);
    bar->addStretch();

    QPushButton* btnAdd = new QPushButton(QStringLiteral("新增医师"));
    btnAdd->setObjectName("btnPri");
    btnAdd->setCursor(Qt::PointingHandCursor);
    bar->addWidget(btnAdd);

    connect(btnSearch, &QPushButton::clicked, this, &DoctorView::onSearch);
    connect(m_edSearch, &QLineEdit::returnPressed, this, &DoctorView::onSearch);
    connect(btnAdd, &QPushButton::clicked, this, &DoctorView::onAdd);
    root->addLayout(bar);

    // ===== 医师列表（卡片内表格） =====
    QWidget* card = new QWidget;
    card->setObjectName("card");
    QVBoxLayout* cv = new QVBoxLayout(card);
    cv->setContentsMargins(0, 0, 0, 0);

    m_tb = new QTableWidget;
    m_tb->setObjectName("tb");
    m_tb->setColumnCount(7);
    m_tb->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("工号") << QStringLiteral("姓名") << QStringLiteral("科室")
        << QStringLiteral("职称") << QStringLiteral("联系方式") << QStringLiteral("状态")
        << QStringLiteral("操作"));
    m_tb->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tb->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tb->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tb->setAlternatingRowColors(true);
    m_tb->verticalHeader()->setVisible(false);
    m_tb->verticalHeader()->setDefaultSectionSize(52);          // 行高 52
    for (int c = 0; c <= 5; ++c)                                // 前 5 列+状态：随内容
        m_tb->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_tb->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);   // 操作列撑满
    cv->addWidget(m_tb);
    root->addWidget(card, 1);

    // ===== 演示数据（对应联调库 staff 表；100009 为逻辑删除样例） =====
    const char* demo[4][5] = {
        {"100002", "张主任", "中医内科",   "主任医师",   "13800000002"},
        {"100003", "李医生", "中医内科",   "主治医师",   "13800000003"},
        {"100004", "王医生", "针灸推拿科", "副主任医师", "13800000004"},
        {"100009", "陈医生", "针灸推拿科", "医师",       "13800000009"},
    };
    const int demoStatus[4] = {1, 1, 1, 0};
    for (int i = 0; i < 4; ++i) {
        sdt::DoctorRow r;
        r.workNo = QString::fromUtf8(demo[i][0]);
        r.name   = QString::fromUtf8(demo[i][1]);
        r.dept   = QString::fromUtf8(demo[i][2]);
        r.title  = QString::fromUtf8(demo[i][3]);
        r.phone  = QString::fromUtf8(demo[i][4]);
        r.status = demoStatus[i];
        m_rows.append(r);
    }
    renderTable();
}

/* 按当前搜索词重建表格（lambda 捕获数据源索引，与过滤后行号解耦） */
void DoctorView::renderTable()
{
    m_tb->clearContents();
    m_tb->setRowCount(0);
    for (int i = 0; i < m_rows.size(); ++i) {
        const sdt::DoctorRow& r = m_rows.at(i);
        if (!m_key.isEmpty()
            && !r.name.contains(m_key, Qt::CaseInsensitive)
            && !r.workNo.contains(m_key, Qt::CaseInsensitive))
            continue;
        const int row = m_tb->rowCount();
        m_tb->insertRow(row);
        mkItem(m_tb, row, 0, r.workNo);
        mkItem(m_tb, row, 1, r.name);
        mkItem(m_tb, row, 2, r.dept);
        mkItem(m_tb, row, 3, r.title);
        mkItem(m_tb, row, 4, r.phone);

        // 状态胶囊：正常 pillOk / 已删除 pillGray
        QLabel* pill = new QLabel(r.status == 1 ? QStringLiteral("正常")
                                                : QStringLiteral("已删除"));
        pill->setObjectName(r.status == 1 ? QStringLiteral("pillOk")
                                          : QStringLiteral("pillGray"));
        pill->setAlignment(Qt::AlignCenter);
        m_tb->setCellWidget(row, 5, pill);

        if (r.status == 1) {
            // 操作列：正常行 编辑/重置密码/删除 三链接；已删除行无操作
            QWidget* ops = new QWidget;
            QHBoxLayout* ol = new QHBoxLayout(ops);
            ol->setContentsMargins(2, 0, 2, 0);
            ol->setSpacing(0);
            QPushButton* bEdit = mkLink(QStringLiteral("编辑"));
            QPushButton* bPwd  = mkLink(QStringLiteral("重置密码"));
            QPushButton* bDel  = mkLink(QStringLiteral("删除"));
            connect(bEdit, &QPushButton::clicked, this, [this, i] { onEdit(i); });
            connect(bPwd,  &QPushButton::clicked, this, [this, i] { onResetPwd(i); });
            connect(bDel,  &QPushButton::clicked, this, [this, i] { onRemove(i); });
            ol->addStretch();
            ol->addWidget(bEdit);
            ol->addWidget(bPwd);
            ol->addWidget(bDel);
            ol->addStretch();
            m_tb->setCellWidget(row, 6, ops);
        }
    }
}

/* 搜索（演示：姓名/工号包含匹配，空=全部） */
void DoctorView::onSearch()
{
    m_key = m_edSearch->text().trimmed();
    renderTable();
}

/* 新增医师：工号取现有最大值+1（6 位），初始密码 123456 */
void DoctorView::onAdd()
{
    sdt::DoctorRow row;
    row.status = 1;
    if (!doctorModal(window(), nullptr, &row)) return;
    int maxNo = 100000;
    foreach (const sdt::DoctorRow& r, m_rows)
        maxNo = qMax(maxNo, r.workNo.toInt());
    row.workNo = QString::number(maxNo + 1);
    m_rows.append(row);
    m_edSearch->clear();
    m_key.clear();
    renderTable();
    Toast::show(window(), QStringLiteral("已新增医师：%1，工号 %2（自动分配 6 位），初始密码 123456")
                              .arg(row.name, row.workNo));
}

/* 编辑医师信息：工号只读，其余字段回填可改 */
void DoctorView::onEdit(int idx)
{
    if (idx < 0 || idx >= m_rows.size()) return;
    sdt::DoctorRow nu = m_rows.at(idx);
    if (!doctorModal(window(), &m_rows.at(idx), &nu)) return;
    m_rows[idx] = nu;
    renderTable();
    Toast::show(window(), QStringLiteral("医师信息已更新"));
}

/* 重置密码：确认后恢复默认密码 123456 */
void DoctorView::onResetPwd(int idx)
{
    if (idx < 0 || idx >= m_rows.size()) return;
    const sdt::DoctorRow& r = m_rows.at(idx);
    if (!confirmModal(window(), QStringLiteral("重置密码"),
                      QStringLiteral("确认将 %1 的密码重置为默认密码 123456？").arg(r.name)))
        return;
    Toast::show(window(), QStringLiteral("密码已重置"));
}

/* 删除医师：逻辑删（状态置 0，历史诊疗数据保留） */
void DoctorView::onRemove(int idx)
{
    if (idx < 0 || idx >= m_rows.size()) return;
    const sdt::DoctorRow& r = m_rows.at(idx);
    if (!confirmModal(window(), QStringLiteral("删除医师"),
                      QStringLiteral("确认删除医师 %1？（逻辑删除，历史诊疗数据保留）").arg(r.name)))
        return;
    m_rows[idx].status = 0;
    renderTable();
    Toast::show(window(), QStringLiteral("已删除（逻辑删除）"));
}
