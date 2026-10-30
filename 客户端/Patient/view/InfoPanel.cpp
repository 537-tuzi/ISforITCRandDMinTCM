#include "InfoPanel.h"
#include "util/UIFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

static QWidget* makeRow(const QString& key, QLabel** val)
{
    // 堆叠式：标签在上、值在下（值可换行），面板可以收窄；实现走 util 工厂
    return uif::makeStackedRow(key, QStringLiteral("prowWrap"),
                               QStringLiteral("prowKey"), QStringLiteral("prowVal"), val);
}

InfoPanel::InfoPanel(bool showPhone, bool showStatus, QWidget* parent) : QWidget(parent)
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

    QLabel* name = new QLabel(QStringLiteral("李建国"));
    name->setObjectName("pname");
    name->setAlignment(Qt::AlignCenter);
    c->addWidget(name);
    QLabel* sub = new QLabel(QStringLiteral("男 · 52 岁"));
    sub->setObjectName("psub");
    sub->setAlignment(Qt::AlignCenter);
    c->addWidget(sub);
    c->addSpacing(4);

    QLabel* yb = nullptr;
    c->addWidget(makeRow(QStringLiteral("医保账号"), &yb));
    if (yb) yb->setText(QStringLiteral("DYB****8823"));
    if (showPhone) {
        QLabel* ph = nullptr;
        c->addWidget(makeRow(QStringLiteral("手机号"), &ph));
        if (ph) ph->setText(QStringLiteral("139****6677"));
    }
    c->addWidget(makeRow(QStringLiteral("过敏史"), &m_gm));
    c->addWidget(makeRow(QStringLiteral("既往病史"), &m_jw));
    if (showStatus) {
        // 建档状态堆叠行；纯文字展示（2026-10-30 决定：去掉绿色胶囊特效）
        QWidget* w = new QWidget;
        w->setObjectName("prowWrap");
        QVBoxLayout* h = new QVBoxLayout(w);
        h->setContentsMargins(0, 4, 0, 4);
        h->setSpacing(3);
        QLabel* b = new QLabel(QStringLiteral("建档状态"));
        b->setObjectName("prowKey");
        m_jd = new QLabel(QStringLiteral("未建档"));
        m_jd->setObjectName("prowVal");
        h->addWidget(b);
        h->addWidget(m_jd);
        c->addWidget(w);
    }
    root->addWidget(card);

    QPushButton* out = new QPushButton(QStringLiteral("退出登录"));
    out->setObjectName("btnDangerLg");   // 红底大按钮（2026-10-30 决定）
    out->setCursor(Qt::PointingHandCursor);
    connect(out, &QPushButton::clicked, this, &InfoPanel::logoutClicked);
    root->addWidget(out);
    root->addStretch();
}

void InfoPanel::setProfile(const QString& gm, const QString& jw)
{
    if (m_gm)  m_gm->setText(gm.isEmpty() ? QStringLiteral("—") : gm);
    if (m_jw)  m_jw->setText(jw.isEmpty() ? QStringLiteral("—") : jw);
}

void InfoPanel::setJdDone(bool done)
{
    if (!m_jd) return;
    m_jd->setText(done ? QStringLiteral("已建档") : QStringLiteral("未建档"));  // 纯文字，无胶囊样式切换
}
