#include "InfoPanel.h"
#include "util/UIFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

InfoPanel::InfoPanel(QWidget* parent) : QWidget(parent)
{
    setFixedWidth(170);
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    QWidget* card = new QWidget;
    card->setObjectName("pcard");
    QVBoxLayout* c = new QVBoxLayout(card);
    c->setContentsMargins(16, 18, 16, 16);
    c->setSpacing(0);

    // 四项统一堆叠式：标签在上（小灰）、值在下（加粗），逐项换行排列
    const struct { const char* key; const char* val; } rows[] = {
        {"姓名", "李建国"},
        {"年龄", "52岁"},
        {"医保账号", "DYB****8823"},
        {"手机号", "139****6677"},
    };
    for (int i = 0; i < 4; ++i) {
        QLabel* v = nullptr;
        c->addWidget(uif::makeStackedRow(QString::fromUtf8(rows[i].key), QStringLiteral("prowWrap"),
                                         QStringLiteral("prowKey"), QStringLiteral("prowVal"), &v));
        if (v) v->setText(QString::fromUtf8(rows[i].val));
    }
    root->addWidget(card);

    QPushButton* out = new QPushButton(QStringLiteral("退出登录"));
    out->setObjectName("btnDangerLg");   // 红底大按钮（2026-10-30 决定）
    out->setCursor(Qt::PointingHandCursor);
    connect(out, &QPushButton::clicked, this, &InfoPanel::logoutClicked);
    root->addWidget(out);
    root->addStretch();
}
