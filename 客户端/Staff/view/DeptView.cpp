#include "DeptView.h"
#include "Dialogs.h"
#include "util/Toast.h"
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
#include <QBrush>

DeptView::DeptView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageDepts");

    // 演示数据（与 zysms_db.department 对应；停用=逻辑删）
    sdt::DeptRow r1;
    r1.id = 1; r1.name = QStringLiteral("中医内科");
    r1.desc = QStringLiteral("中医内科诊疗科室");
    r1.leader = QStringLiteral("张主任（主任医师）");
    m_rows.append(r1);  m_doctors.append(2);
    sdt::DeptRow r2;
    r2.id = 2; r2.name = QStringLiteral("针灸推拿科");
    r2.desc = QStringLiteral("针灸推拿诊疗科室");
    r2.leader = QStringLiteral("王医生（副主任医师）");
    m_rows.append(r2);  m_doctors.append(1);   // 在职医师（陈医生已逻辑删，不计入）
    sdt::DeptRow r3;
    r3.id = 3; r3.name = QStringLiteral("康复理疗科");
    r3.desc = QStringLiteral("（筹建中）");
    r3.status = 0;
    m_rows.append(r3);  m_doctors.append(0);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    QFrame* card = new QFrame;
    card->setObjectName("card");
    root->addWidget(card, 1);

    QVBoxLayout* cv = new QVBoxLayout(card);
    cv->setContentsMargins(0, 0, 0, 0);
    QWidget* head = new QWidget;
    QHBoxLayout* hh = new QHBoxLayout(head);
    hh->setContentsMargins(16, 12, 16, 8);
    QLabel* title = new QLabel(QStringLiteral("科室管理"));
    title->setObjectName("chTitle");
    hh->addWidget(title);
    hh->addStretch();
    QPushButton* add = new QPushButton(QStringLiteral("新增科室"));
    add->setObjectName("btnPri");
    add->setCursor(Qt::PointingHandCursor);
    connect(add, &QPushButton::clicked, this, [this] { openEditor(-1); });
    hh->addWidget(add);
    cv->addWidget(head);

    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(16, 2, 16, 12);
    m_table = new QTableWidget(0, 5);
    m_table->setObjectName("tb");
    QStringList heads;
    heads << QStringLiteral("科室") << QStringLiteral("简介") << QStringLiteral("负责人")
          << QStringLiteral("在职医师数") << QStringLiteral("状态") << QStringLiteral("操作");
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(heads);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    bv->addWidget(m_table, 1);
    cv->addWidget(body, 1);

    rebuild();
}

void DeptView::rebuild()
{
    m_table->setRowCount(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        const sdt::DeptRow& r = m_rows[i];
        const bool active = (r.status == 1);
        m_table->setRowHeight(i, 52);
        const QString cols[5] = { r.name, r.desc.isEmpty() ? QStringLiteral("—") : r.desc,
                                  r.leader.isEmpty() ? QStringLiteral("—") : r.leader,
                                  QString::number(m_doctors[i]),
                                  active ? QStringLiteral("正常") : QStringLiteral("停用") };
        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem* it = new QTableWidgetItem(cols[c]);
            it->setTextAlignment(Qt::AlignCenter);
            if (!active) it->setForeground(QBrush(QColor("#a9a698")));
            m_table->setItem(i, c, it);
        }
        // 操作列：正常行=编辑/停用；停用行=—
        QWidget* host = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(host);
        h->setContentsMargins(2, 2, 2, 2);
        h->setSpacing(6);
        if (active) {
            const int row = i;
            QPushButton* edit = new QPushButton(QStringLiteral("编辑"));
            edit->setObjectName("btnLink");
            edit->setCursor(Qt::PointingHandCursor);
            QPushButton* stop = new QPushButton(QStringLiteral("停用"));
            stop->setObjectName("btnLink");
            stop->setCursor(Qt::PointingHandCursor);
            h->addWidget(edit);
            h->addWidget(stop);
            connect(edit, &QPushButton::clicked, this, [this, row] { openEditor(row); });
            connect(stop, &QPushButton::clicked, this, [this, row] { tryDisable(row); });
        } else {
            QLabel* dash = new QLabel(QStringLiteral("—"));
            dash->setAlignment(Qt::AlignCenter);
            h->addWidget(dash);
        }
        m_table->setCellWidget(i, 5, host);
    }
}

/* 新增/编辑弹窗：row<0 新增（校验名称必填），>=0 编辑回填 */
void DeptView::openEditor(int row)
{
    const bool isEdit = row >= 0;
    const sdt::DeptRow* cur = isEdit ? &m_rows[row] : nullptr;

    QDialog dlg(window());
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    bv->setSpacing(10);

    QLabel* l1 = new QLabel(QStringLiteral("科室名称"));
    l1->setObjectName("fldLabel");
    QLineEdit* edName = new QLineEdit;
    edName->setPlaceholderText(QStringLiteral("请输入科室名称"));
    if (cur) edName->setText(cur->name);
    bv->addWidget(l1); bv->addWidget(edName);

    QLabel* l2 = new QLabel(QStringLiteral("科室简介"));
    l2->setObjectName("fldLabel");
    QLineEdit* edDesc = new QLineEdit;
    if (cur) edDesc->setText(cur->desc);
    bv->addWidget(l2); bv->addWidget(edDesc);

    QLabel* l3 = new QLabel(QStringLiteral("负责人（限主任/副主任医师）"));
    l3->setObjectName("fldLabel");
    QComboBox* cbLeader = new QComboBox;
    cbLeader->addItem(QStringLiteral("（暂不设置）"));
    cbLeader->addItems(QStringList() << QStringLiteral("张主任（主任医师）")
                                     << QStringLiteral("王医生（副主任医师）"));
    bv->addWidget(l3); bv->addWidget(cbLeader);

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(isEdit ? QStringLiteral("确认保存") : QStringLiteral("确认新增"),
                      QStringLiteral("btnPri"));
    const QList<QPushButton*> made = setupModal(&dlg,
        isEdit ? QStringLiteral("编辑科室") : QStringLiteral("新增科室"), body, btns);
    dlg.connect(made.at(0), &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(made.at(1), &QPushButton::clicked, &dlg, [&]() {
        if (edName->text().trimmed().isEmpty()) {
            Toast::show(&dlg, QStringLiteral("请输入科室名称"), true);
            return;
        }
        sdt::DeptRow out;
        out.name = edName->text().trimmed();
        out.desc = edDesc->text().trimmed();
        const QString ld = cbLeader->currentText();
        out.leader = (ld == QStringLiteral("（暂不设置）")) ? QString() : ld;
        if (isEdit) {
            out.id = cur->id;
            out.status = cur->status;
            m_rows[row] = out;
            Toast::show(window(), QStringLiteral("科室已保存"));
        } else {
            out.id = m_rows.size() + 1;
            out.status = 1;
            m_rows.append(out);
            m_doctors.append(0);
            Toast::show(window(), QStringLiteral("科室已新增：%1").arg(out.name));
        }
        dlg.accept();
        rebuild();
    });
    dlg.exec();
}

/* 停用（逻辑删）流程：确认 → 前置约束（在职医师>0 拒绝）→ 状态置 0 */
void DeptView::tryDisable(int row)
{
    const sdt::DeptRow& r = m_rows[row];
    QDialog dlg(window());
    QLabel* body = new QLabel(QStringLiteral("确认停用科室「%1」？停用前须确保无在职医师与未来排班。").arg(r.name));
    body->setObjectName("mText");
    body->setWordWrap(true);
    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认停用"), QStringLiteral("btnDangerO"));
    const QList<QPushButton*> made = setupModal(&dlg, QStringLiteral("停用科室"), body, btns);
    dlg.connect(made.at(0), &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.connect(made.at(1), &QPushButton::clicked, &dlg, [&]() {
        if (m_doctors[row] > 0) {
            Toast::show(window(), QStringLiteral("该科室仍有在职医师，不能停用"), true);
        } else {
            m_rows[row].status = 0;
            Toast::show(window(), QStringLiteral("科室已停用（逻辑删除）"));
            rebuild();
        }
        dlg.accept();
    });
    dlg.exec();
}
