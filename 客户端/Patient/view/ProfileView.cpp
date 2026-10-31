#include "ProfileView.h"
#include "util/Toast.h"
#include "util/AppDefs.h"
#include "util/UIFactory.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QScrollBar>

// AI 演示对话脚本（q=AI 回复 a=用户语句 fill=自动填充左侧栏目）
struct AiTurn { const char* q; const char* a; const char* gm; const char* jw; };
static const AiTurn kAiTurns[] = {
    {"好的，您的姓名、年龄等基本信息已经从医保账号带入了。请问您有药物或食物过敏吗？",
     "我对青霉素过敏。", "青霉素", nullptr},
    {"已记下「青霉素过敏」。请问您以前生过什么病、或长期吃什么药吗？",
     "我有高血压五年了，一直吃降压药。", nullptr, "高血压 5 年（服降压药）"},
    {"收到。您平时睡得好吗？抽烟喝酒吗？",
     "睡眠一般，不吸烟，偶尔喝一点酒。", nullptr, "高血压 5 年（服降压药），睡眠一般"},
    {"信息都录好了！左边「过敏史」「既往病史」可以自己再改。都确认好了就点「保存并上传」。",
     "", nullptr, nullptr},
};
static const int kAiTurnCount = int(sizeof(kAiTurns) / sizeof(kAiTurns[0]));

ProfileView::ProfileView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageJd");
    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    // ===== 左：健康档案卡 =====
    QFrame* card = new QFrame;
    card->setObjectName("card");
    card->setMinimumWidth(330);
    card->setMaximumWidth(400);
    QVBoxLayout* cv = new QVBoxLayout(card);
    cv->setContentsMargins(0, 0, 0, 0);

    QLabel* ch = new QLabel(QStringLiteral("健康档案"));
    ch->setObjectName("chTitle");
    ch->setContentsMargins(14, 12, 14, 12);
    cv->addWidget(ch);

    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(14, 4, 14, 14);
    bv->setSpacing(0);

    QGridLayout* grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(0);
    grid->addWidget(makeRo(QStringLiteral("姓名"),   QStringLiteral("李建国")), 0, 0);
    grid->addWidget(makeRo(QStringLiteral("性别"),   QStringLiteral("男")),     0, 1);
    grid->addWidget(makeRo(QStringLiteral("年龄"),   QStringLiteral("52 岁")),  1, 0);
    grid->addWidget(makeRo(QStringLiteral("医保账号"), QStringLiteral("DYB****8823")), 1, 1);
    grid->addWidget(makeRo(QStringLiteral("手机号"), QStringLiteral("139****6677")), 2, 0);
    grid->addWidget(makeRo(QStringLiteral("身份证号"), QStringLiteral("3301**********2215")), 2, 1);
    bv->addLayout(grid);
    bv->addSpacing(10);

    QLabel* l1 = new QLabel(QStringLiteral("过敏史"));
    l1->setObjectName("fldLabel");
    m_gm = new QPlainTextEdit;   // 多行：长档案文案可换行查看布局
    m_gm->setPlaceholderText(QStringLiteral("按住右侧按钮说出来，或直接填写"));
    m_gm->setFixedHeight(96);
    m_gm->setPlainText(QStringLiteral("青霉素过敏；海鲜（虾蟹）过敏；春季花粉过敏"));
    bv->addWidget(l1); bv->addWidget(m_gm); bv->addSpacing(8);

    QLabel* l2 = new QLabel(QStringLiteral("既往病史"));
    l2->setObjectName("fldLabel");
    m_jw = new QPlainTextEdit;
    m_jw->setPlaceholderText(QStringLiteral("按住右侧按钮说出来，或直接填写"));
    m_jw->setFixedHeight(120);
    m_jw->setPlainText(QStringLiteral(
        "高血压 5 年（服氨氯地平），血压控制尚可；2 型糖尿病 3 年（二甲双胍 0.5g 每日两次）；"
        "2019 年阑尾炎切除术；睡眠一般，偶有失眠。"));
    bv->addWidget(l2); bv->addWidget(m_jw); bv->addSpacing(10);

    QPushButton* save = new QPushButton(QStringLiteral("保存并上传"));
    save->setObjectName("btnPriLg");
    save->setCursor(Qt::PointingHandCursor);
    connect(save, &QPushButton::clicked, this, &ProfileView::onSave);
    bv->addWidget(save);

    m_okNote = new QLabel(QStringLiteral("档案已保存，与医保账号绑定"));
    m_okNote->setObjectName("okb");
    m_okNote->hide();
    bv->addWidget(m_okNote);
    bv->addStretch();
    cv->addWidget(body, 1);

    // ===== 右：AI 语音助手卡 =====
    QFrame* ai = new QFrame;
    ai->setObjectName("card");
    QVBoxLayout* av = new QVBoxLayout(ai);
    av->setContentsMargins(0, 0, 0, 0);

    QWidget* ah = new QWidget;
    ah->setObjectName("chBar");
    QHBoxLayout* ahl = new QHBoxLayout(ah);
    ahl->setContentsMargins(14, 12, 14, 12);
    QLabel* at = new QLabel(QStringLiteral("AI 语音助手"));
    at->setObjectName("chTitle");
    // 「在线」绿点闪烁（MD §5.2）：圆点单独成标签，定时器切换可见性
    QLabel* liveDot = new QLabel(QStringLiteral("●"));
    liveDot->setObjectName("liveDot");
    QLabel* live = new QLabel(QStringLiteral("在线"));
    live->setObjectName("live");
    ahl->addWidget(at);
    ahl->addStretch();
    ahl->addWidget(liveDot);
    ahl->addWidget(live);
    av->addWidget(ah);
    QTimer* blink = new QTimer(this);
    blink->start(700);
    connect(blink, &QTimer::timeout, liveDot, [liveDot] {
        liveDot->setVisible(!liveDot->isVisible());
    });

    m_scroll = new QScrollArea;
    m_scroll->setObjectName("aiScroll");
    m_scroll->setWidgetResizable(true);
    QWidget* chatHost = new QWidget;
    chatHost->setObjectName("aiBody");
    m_chat = new QVBoxLayout(chatHost);
    m_chat->setContentsMargins(14, 14, 14, 14);
    m_chat->setSpacing(10);
    m_chat->addStretch();
    m_intro = new QLabel(QStringLiteral(
        "您好！我是您的健康助手，帮您完成建档。您的姓名、年龄等基本信息已从医保账号自动带入，"
        "左侧带锁的项不用填。接下来我问您几个问题，帮您把「过敏史」「既往病史」录进去："
        "按住下面的按钮说话，说完松手就行。"));
    m_intro->setObjectName("msgAi");
    m_intro->setWordWrap(true);
    m_chat->insertWidget(m_chat->count() - 1, m_intro);
    m_scroll->setWidget(chatHost);
    av->addWidget(m_scroll, 1);

    QWidget* af = new QWidget;
    af->setObjectName("chBar");
    QVBoxLayout* afl = new QVBoxLayout(af);
    afl->setContentsMargins(14, 10, 14, 10);
    QPushButton* hold = new QPushButton(QStringLiteral("🎤 按住说话（说完松手）"));
    hold->setObjectName("hold");
    hold->setCursor(Qt::PointingHandCursor);
    afl->addWidget(hold, 0, Qt::AlignHCenter);
    av->addWidget(af);

    connect(hold, &QPushButton::released, this, &ProfileView::onHoldReleased);

    root->addWidget(card, 0);
    root->addWidget(ai, 1);
}

QWidget* ProfileView::makeRo(const QString& key, const QString& val)
{
    QLabel* v = nullptr;
    QWidget* w = uif::makeStackedRow(key, QStringLiteral("ro"),
                                     QStringLiteral("roKey"), QStringLiteral("roVal"), &v);
    if (v) v->setText(val);
    return w;
}

void ProfileView::appendAi(const QString& text)
{
    QLabel* l = new QLabel(text);
    l->setObjectName("msgAi");
    l->setWordWrap(true);
    m_chat->insertWidget(m_chat->count() - 1, l);
    scrollChatBottom();
}

void ProfileView::appendMe(const QString& text)
{
    QLabel* l = new QLabel(text);
    l->setObjectName("msgMe");
    l->setWordWrap(true);
    m_chat->insertWidget(m_chat->count() - 1, l, 0, Qt::AlignRight);
    scrollChatBottom();
}

/* 手册 3.3.2：松开按钮=语音完成，可多次；AI 回复显示在聊天窗，识别内容填入左侧 */
void ProfileView::onHoldReleased()
{
    if (m_aiIdx >= kAiTurnCount) return;
    const AiTurn& t = kAiTurns[m_aiIdx];
    if (*t.a) appendMe(QStringLiteral("（您说）") + QString::fromUtf8(t.a));
    const AiTurn cur = t;
    QTimer::singleShot(def::AI_REPLY_MS, this, [this, cur] {
        appendAi(QString::fromUtf8(cur.q));
        if (cur.gm) m_gm->setPlainText(QString::fromUtf8(cur.gm));
        if (cur.jw) m_jw->setPlainText(QString::fromUtf8(cur.jw));
    });
    ++m_aiIdx;
}

/* 保存：过敏史/既往病史必填（手册演示规则），成功后全局切换"已建档" */
void ProfileView::onSave()
{
    if (m_gm->toPlainText().trimmed().isEmpty()) {
        Toast::show(window(), QStringLiteral("「过敏史」还没有填，没有就填「无」"), true);
        return;
    }
    if (m_jw->toPlainText().trimmed().isEmpty()) {
        Toast::show(window(), QStringLiteral("「既往病史」还没有填，没有就填「无」"), true);
        return;
    }
    // 建档完成后首语替换（仅当未进行过语音对话）
    if (m_aiIdx == 0)
        m_intro->setText(QStringLiteral(
            "您的健康档案已建立。左侧「过敏史」「既往病史」可随时修改，"
            "或按住下方按钮语音更新，改完点「保存并上传」。"));
    m_okNote->show();
    Toast::show(window(), QStringLiteral("档案已保存上传，正在返回首页…"));
    emit profileSaved(m_gm->toPlainText(), m_jw->toPlainText());
}

void ProfileView::scrollChatBottom()
{
    QTimer::singleShot(60, this, [this] {
        QScrollBar* sb = m_scroll->verticalScrollBar();
        if (sb) sb->setValue(sb->maximum());
    });
}
