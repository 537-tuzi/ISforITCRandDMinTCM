#include "Dialogs.h"
#include "util/Toast.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>

/* 通用弹窗骨架（与 Patient 工程同款：标题+正文+底部按钮，返回按钮列表由调用方接线） */
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

static QPushButton* findBtn(const QList<QPushButton*>& list, const char* name)
{
    foreach (QPushButton* b, list)
        if (b->objectName() == QLatin1String(name)) return b;
    return nullptr;
}

/* 修改密码：原密码/新密码/确认新密码 三框（演示比对 123456；正式版走服务端） */
ChangePwdDialog::ChangePwdDialog(QWidget* parent) : QDialog(parent)
{
    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(0, 0, 0, 0);
    bv->setSpacing(10);

    struct Fld { const char* label; QLineEdit::EchoMode mode; };
    QLineEdit* eds[3] = {nullptr, nullptr, nullptr};
    const char* labels[3] = {"原密码", "新密码（6-16 位，须包含字母和数字）", "确认新密码"};
    for (int i = 0; i < 3; ++i) {
        QLabel* l = new QLabel(QString::fromUtf8(labels[i]));
        l->setObjectName("fldLabel");
        QLineEdit* e = new QLineEdit;
        e->setEchoMode(QLineEdit::Password);
        bv->addWidget(l);
        bv->addWidget(e);
        eds[i] = e;
    }

    QList<QPair<QString, QString>> btns;
    btns << qMakePair(QStringLiteral("取消"), QStringLiteral("btnOut"))
         << qMakePair(QStringLiteral("确认修改"), QStringLiteral("btnPri"));
    const QList<QPushButton*> made = setupModal(this, QStringLiteral("修改密码"), body, btns);
    connect(findBtn(made, "btnOut"), &QPushButton::clicked, this, &QDialog::reject);
    connect(findBtn(made, "btnPri"), &QPushButton::clicked, this, [this, eds] {
        if (eds[0]->text() != def::DEMO_SECRET) {
            Toast::show(this, QStringLiteral("原密码不正确"), true);
            return;
        }
        const QString np = eds[1]->text();
        static const QRegularExpression re(
            QStringLiteral("^(?=.*[A-Za-z])(?=.*\\d).{6,16}$"));
        if (!re.match(np).hasMatch()) {
            Toast::show(this, QStringLiteral("新密码须为 6-16 位且同时包含字母和数字"), true);
            return;
        }
        if (np != eds[2]->text()) {
            Toast::show(this, QStringLiteral("两次输入的新密码不一致"), true);
            return;
        }
        accept();
    });
}
