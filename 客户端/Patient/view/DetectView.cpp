#include "DetectView.h"
#include "InfoPanel.h"
#include "util/Toast.h"
#include "Dialogs.h"
#include "util/AppDefs.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QStyle>
#include <QFrame>
#include <QRandomGenerator>
#include <QEvent>

// 手册 3.3.4：五类舌象，灰黑苔>镜面舌>黄腻苔>白腻苔>薄白苔；灰黑苔/镜面舌重症触发自动预约
static QVector<pd::DetectResult> detLib()
{
    QVector<pd::DetectResult> lib;
    pd::DetectResult r;

    r.type = QStringLiteral("薄白苔"); r.state = QStringLiteral("健康");
    r.conf = QStringLiteral("96%"); r.sev = pd::Sev::Ok; r.autoBook = false;
    r.rep << QStringLiteral("舌淡红，苔薄白而润，舌头正常")
          << QStringLiteral("气血调和，脾胃功能良好")
          << QStringLiteral("建议每年复查 1-2 次");
    r.tips = QStringLiteral("饮食规律，可常吃山药、小米粥养胃。【怎么睡】晚上 11 点前睡觉。【怎么养】每天叩齿 36 下，配合八段锦「调理脾胃须单举」。");
    lib.append(r); r.rep.clear();

    r.type = QStringLiteral("白腻苔"); r.state = QStringLiteral("身体有恙");
    r.conf = QStringLiteral("93%"); r.sev = pd::Sev::Warn; r.autoBook = false;
    r.rep << QStringLiteral("舌淡胖，苔白腻，边有齿痕")
          << QStringLiteral("提示湿气重，困住脾胃")
          << QStringLiteral("建议到脾胃科调理");
    r.tips = QStringLiteral("少吃生冷、油腻、甜食；多吃薏米、赤小豆、陈皮煮水祛湿。【怎么动】别久坐，适度运动出点汗。【怎么养】可以艾灸肚脐和足三里，每次 15 分钟，每周 3 次。");
    lib.append(r); r.rep.clear();

    r.type = QStringLiteral("黄腻苔"); r.state = QStringLiteral("身体有恙");
    r.conf = QStringLiteral("91%"); r.sev = pd::Sev::Warn; r.autoBook = false;
    r.rep << QStringLiteral("舌红，苔又黄又厚腻")
          << QStringLiteral("提示湿热内积、脾胃有热")
          << QStringLiteral("建议尽快到脾胃科看病");
    r.tips = QStringLiteral("忌辛辣、油炸、酒；多吃绿豆、冬瓜、苦瓜清热。【怎么睡】别熬夜，减少内热。【注意】清热的药要在医师指导下吃，别自己买。");
    lib.append(r); r.rep.clear();

    r.type = QStringLiteral("镜面舌"); r.state = QStringLiteral("身体有恙");
    r.conf = QStringLiteral("89%"); r.sev = pd::Sev::Red; r.autoBook = true;
    r.rep << QStringLiteral("舌面光滑无苔，像镜子一样")
          << QStringLiteral("提示胃阴受伤、津液亏耗")
          << QStringLiteral("情况较重，系统已自动帮您挂号");
    r.tips = QStringLiteral("用西洋参、石斛、麦冬泡水滋养，忌辛辣燥热。【注意】请一定按预约时间去看病，配合医生系统治疗。");
    lib.append(r); r.rep.clear();

    r.type = QStringLiteral("灰黑苔"); r.state = QStringLiteral("身体有恙");
    r.conf = QStringLiteral("90%"); r.sev = pd::Sev::Red; r.autoBook = true;
    r.rep << QStringLiteral("苔色灰黑而干，病情较重")
          << QStringLiteral("提示阳虚寒重或里热炽盛")
          << QStringLiteral("情况较重，系统已自动帮您挂号");
    r.tips = QStringLiteral("【注意】灰黑苔是重病信号，请尽快就诊。系统已自动约好最近的科室和医生。");
    lib.append(r);
    return lib;
}

// 手册 3.3.4：手动预约四级联动数据来自系统排班
static pd::Sched sched()
{
    pd::Sched s;
    s[QStringLiteral("脾胃科")][QStringLiteral("陈志远（主任医师）")][QStringLiteral("10-31（周六）")] =
        QStringList{ QStringLiteral("上午 08:00-12:00") };
    s[QStringLiteral("脾胃科")][QStringLiteral("陈志远（主任医师）")][QStringLiteral("11-03（周二）")] =
        QStringList{ QStringLiteral("上午 08:00-12:00"), QStringLiteral("下午 14:00-17:00") };
    s[QStringLiteral("脾胃科")][QStringLiteral("林淑华（副主任医师）")][QStringLiteral("10-31（周六）")] =
        QStringList{ QStringLiteral("下午 14:00-17:00") };
    s[QStringLiteral("脾胃科")][QStringLiteral("林淑华（副主任医师）")][QStringLiteral("11-04（周三）")] =
        QStringList{ QStringLiteral("晚班 19:00-21:00") };
    s[QStringLiteral("针灸推拿科")][QStringLiteral("周红梅（主治医师）")][QStringLiteral("11-01（周日）")] =
        QStringList{ QStringLiteral("上午 08:00-12:00"), QStringLiteral("下午 14:00-17:00") };
    s[QStringLiteral("内科")][QStringLiteral("赵国安（主任医师）")][QStringLiteral("10-30（周五）")] =
        QStringList{ QStringLiteral("晚班 19:00-21:00") };
    s[QStringLiteral("内科")][QStringLiteral("赵国安（主任医师）")][QStringLiteral("11-02（周一）")] =
        QStringList{ QStringLiteral("上午 08:00-12:00") };
    return s;
}

/* 养生建议按【分类】拆条（如【怎么吃】xxx），渲染为"· 条目"逐行，样式与检测报告一致 */
static QStringList splitTips(const QString& tips)
{
    QStringList out;
    int start = tips.indexOf(QStringLiteral("【"));
    while (start >= 0) {
        const int next = tips.indexOf(QStringLiteral("【"), start + 1);
        out << tips.mid(start, ((next >= 0) ? next : tips.size()) - start);
        start = next;
    }
    return out;
}

DetectView::DetectView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageDetect");
    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(4, 10, 12, 12);   // 左侧贴边，宽度让给右侧内容区
    root->setSpacing(8);

    // ===== 左侧信息面板（三页统一齐全版：手机号+建档状态） =====
    m_panel = new InfoPanel(true, true);
    connect(m_panel, &InfoPanel::logoutClicked, this, &DetectView::logoutClicked);
    root->addWidget(m_panel);

    QWidget* main = new QWidget;
    QVBoxLayout* mv = new QVBoxLayout(main);
    mv->setContentsMargins(0, 0, 0, 0);
    mv->setSpacing(12);

    QHBoxLayout* cols = new QHBoxLayout;
    cols->setSpacing(10);

    // ===== 中：摄像头卡 =====
    QFrame* cam = new QFrame;
    cam->setObjectName("card");
    QVBoxLayout* cv = new QVBoxLayout(cam);
    cv->setContentsMargins(12, 12, 12, 12);
    cv->setSpacing(10);

    QWidget* steps = new QWidget;
    QHBoxLayout* sh = new QHBoxLayout(steps);
    sh->setContentsMargins(0, 0, 0, 0);
    sh->setSpacing(18);
    sh->addWidget(makeStep(1, QStringLiteral("伸出舌头")));
    sh->addWidget(makeStep(2, QStringLiteral("开始检测")));
    sh->addStretch();
    cv->addWidget(steps);

    m_screen = new QFrame;
    m_screen->setObjectName("screen");
    m_screen->setMinimumHeight(180);   // 800×640 下自适应，剩余空间全部给画面
    QVBoxLayout* sv = new QVBoxLayout(m_screen);
    sv->addStretch();
    QLabel* cap = new QLabel(QStringLiteral("摄像头实时画面"));
    cap->setObjectName("screenTxt");
    cap->setAlignment(Qt::AlignCenter);
    sv->addWidget(cap, 0, Qt::AlignHCenter);
    sv->addStretch();
    m_scan = new QFrame(m_screen);
    m_scan->setObjectName("scanline");
    m_scan->setFixedHeight(3);
    m_scan->hide();
    // 四角取景框（MD §6.2），随画面尺寸自适应摆位
    const char* cn[4] = { "ctl", "ctr", "cbl", "cbr" };
    for (int i = 0; i < 4; ++i) {
        QFrame* c = new QFrame(m_screen);
        c->setObjectName(cn[i]);
        c->setFixedSize(34, 34);
        m_corners.append(c);
    }
    m_screen->installEventFilter(this);
    placeCorners();
    cv->addWidget(m_screen, 1);

    m_start = new QPushButton(QStringLiteral("开始检测"));
    m_start->setObjectName("btnPriLg");
    m_start->setCursor(Qt::PointingHandCursor);
    connect(m_start, &QPushButton::clicked, this, &DetectView::onStart);
    cv->addWidget(m_start);

    QWidget* stats = new QWidget;
    QHBoxLayout* sth = new QHBoxLayout(stats);
    sth->setContentsMargins(0, 0, 0, 0);
    sth->setSpacing(8);
    sth->addWidget(makeStat(QStringLiteral("舌苔类型"), &m_type), 1);
    sth->addWidget(makeStat(QStringLiteral("健康状态"), &m_state), 1);
    sth->addWidget(makeStat(QStringLiteral("置信度"),   &m_conf),  1);
    cv->addWidget(stats);
    cols->addWidget(cam, 6);          // 中:右 ≈ 6:5 撑满可用宽度

    // ===== 右：报告列 =====
    QWidget* rcol = new QWidget;
    QVBoxLayout* rv = new QVBoxLayout(rcol);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(12);

    m_repCard = new QFrame;
    m_repCard->setObjectName("card");
    QVBoxLayout* repv = new QVBoxLayout(m_repCard);
    repv->setContentsMargins(0, 0, 0, 0);
    QWidget* rh = new QWidget;
    rh->setObjectName("chBar");
    QHBoxLayout* rhl = new QHBoxLayout(rh);
    rhl->setContentsMargins(14, 12, 14, 12);
    QLabel* rt = new QLabel(QStringLiteral("检测报告"));
    rt->setObjectName("chTitle");
    rhl->addWidget(rt);
    rhl->addStretch();
    repv->addWidget(rh);
    QWidget* rb = new QWidget;
    QVBoxLayout* rbv = new QVBoxLayout(rb);
    rbv->setContentsMargins(14, 10, 14, 12);
    m_repBody = new QLabel(QStringLiteral("还没有检测结果，请点击「开始检测」。"));
    m_repBody->setObjectName("repBody");
    m_repBody->setWordWrap(true);
    m_repBody->setAlignment(Qt::AlignLeft | Qt::AlignTop);   // 从上到下、从左到右
    rbv->addWidget(m_repBody, 1);
    m_repNote = new QLabel(QStringLiteral("结果较重，系统已自动挂号，请查看弹窗确认。"));
    m_repNote->setObjectName("noteRed");
    m_repNote->setWordWrap(true);
    m_repNote->hide();
    rbv->addWidget(m_repNote);
    repv->addWidget(rb, 1);
    rv->addWidget(m_repCard, 1);

    m_tipsCard = new QFrame;
    m_tipsCard->setObjectName("card");
    QVBoxLayout* tv = new QVBoxLayout(m_tipsCard);
    tv->setContentsMargins(0, 0, 0, 0);
    QWidget* th = new QWidget;
    th->setObjectName("chBar");
    QHBoxLayout* thl = new QHBoxLayout(th);
    thl->setContentsMargins(14, 12, 14, 12);
    QLabel* tt = new QLabel(QStringLiteral("养生建议"));
    tt->setObjectName("chTitle");
    thl->addWidget(tt);
    thl->addStretch();
    tv->addWidget(th);
    QWidget* tb = new QWidget;
    QVBoxLayout* tbv = new QVBoxLayout(tb);
    tbv->setContentsMargins(14, 10, 14, 12);
    m_tipsBody = new QLabel(QStringLiteral("检测完成后，这里会根据您的舌头情况给出饮食、作息、调理建议。"));
    m_tipsBody->setObjectName("repBody");
    m_tipsBody->setWordWrap(true);
    m_tipsBody->setAlignment(Qt::AlignLeft | Qt::AlignTop);   // 从上到下、从左到右
    tbv->addWidget(m_tipsBody, 1);
    tv->addWidget(tb, 1);
    rv->addWidget(m_tipsCard, 1);
    cols->addWidget(rcol, 5);

    mv->addLayout(cols, 1);

    // ===== 底部三按钮（手册 3.3.4：保存报告/再检测一次/预约医生） =====
    QWidget* acts = new QWidget;
    QHBoxLayout* ah = new QHBoxLayout(acts);
    ah->setContentsMargins(0, 0, 0, 0);
    ah->setSpacing(10);
    QPushButton* save = new QPushButton(QStringLiteral("保存报告"));
    save->setObjectName("btnOut");
    save->setCursor(Qt::PointingHandCursor);
    connect(save, &QPushButton::clicked, this, [this] {
        Toast::show(window(), QStringLiteral("报告已保存到服务器，与您的档案绑定"));
    });
    QPushButton* again = new QPushButton(QStringLiteral("再检测一次"));
    again->setObjectName("btnOut");
    again->setCursor(Qt::PointingHandCursor);
    connect(again, &QPushButton::clicked, this, &DetectView::onReset);
    QPushButton* book = new QPushButton(QStringLiteral("预约医生"));
    book->setObjectName("btnPri");
    book->setCursor(Qt::PointingHandCursor);
    connect(book, &QPushButton::clicked, this, &DetectView::onBook);
    ah->addWidget(save);
    ah->addWidget(again);
    ah->addWidget(book);
    ah->addStretch();
    mv->addWidget(acts);

    root->addWidget(main, 1);

    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, &QTimer::timeout, this, &DetectView::tickScan);
}

QWidget* DetectView::makeStep(int n, const QString& text)
{
    QWidget* w = new QWidget;
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(10);
    QLabel* num = new QLabel(QString::number(n));
    num->setObjectName("stepN");
    num->setFixedSize(34, 34);   // 实心绿圆白字编号
    num->setAlignment(Qt::AlignCenter);
    QLabel* t = new QLabel(text);
    t->setObjectName("stepTxt");
    h->addWidget(num);
    h->addWidget(t);
    return w;
}

QFrame* DetectView::makeStat(const QString& title, QLabel** val)
{
    QFrame* f = new QFrame;
    f->setObjectName("stat");
    QVBoxLayout* v = new QVBoxLayout(f);
    v->setContentsMargins(8, 8, 8, 8);
    QLabel* t = new QLabel(title);
    t->setObjectName("statTitle");
    t->setAlignment(Qt::AlignCenter);
    QLabel* b = new QLabel(QStringLiteral("—"));
    b->setObjectName("statVal");
    b->setAlignment(Qt::AlignCenter);
    v->addWidget(t);
    v->addWidget(b);
    *val = b;
    return f;
}

void DetectView::setProfile(const QString& gm, const QString& jw)
{ m_panel->setProfile(gm, jw); }   // MD §6.1：左面板过敏史/既往病史同步

void DetectView::setJdDone(bool done)
{ m_panel->setJdDone(done); }      // 左面板建档状态同步（三页一致）

void DetectView::setPanelWidth(int w)
{ m_panel->setFixedWidth(w); }

/* 开始检测：演示 1.8s 返回（正式版：串口等待，超时 10s 可重发，见表 3） */
void DetectView::onStart()
{
    m_scan->show();
    m_scanY = 20;
    m_scanTimer->start(30);
    m_start->setEnabled(false);
    m_start->setText(QStringLiteral("正在检测，请保持不动…"));
    m_type->setText(QStringLiteral("识别中"));
    m_state->setText(QStringLiteral("…"));
    m_conf->setText(QStringLiteral("…"));

    const QVector<pd::DetectResult> lib = detLib();
    const pd::DetectResult r = lib[m_detIdx];
    QTimer::singleShot(def::DETECT_ANIM_MS, this, [this, r] {
        m_scanTimer->stop();
        m_scan->hide();
        m_start->setEnabled(true);
        m_start->setText(QStringLiteral("开始检测"));

        m_type->setText(r.type);
        m_conf->setText(r.conf);
        m_state->setText(r.state);
        m_state->setStyleSheet(r.sev == pd::Sev::Ok
            ? QStringLiteral("color:%1;font-weight:700;").arg(def::CLR_PRESS)
            : r.sev == pd::Sev::Red
            ? QStringLiteral("color:%1;font-weight:700;").arg(def::CLR_WARN)
            : QStringLiteral("color:%1;font-weight:700;").arg(def::CLR_TEXT));

        QString rep;
        foreach (const QString& x, r.rep) rep += QStringLiteral("· ") + x + QStringLiteral("<br>");
        rep.chop(4);
        m_repBody->setText(rep);

        // 养生建议与检测报告同款条目式："· 【分类】内容"逐行
        QString tips;
        foreach (const QString& seg, splitTips(r.tips))
            tips += QStringLiteral("· ") + seg + QStringLiteral("<br>");
        tips.chop(4);
        m_tipsBody->setText(tips);
        m_repNote->setVisible(r.autoBook);
        m_repCard->setObjectName(r.sev == pd::Sev::Red ? "cardRed" : "card");
        m_tipsBody->setText(r.tips);
        m_tipsCard->setObjectName(r.sev == pd::Sev::Red ? "cardRed" : "card");
        m_repCard->style()->unpolish(m_repCard);
        m_repCard->style()->polish(m_repCard);
        m_tipsCard->style()->unpolish(m_tipsCard);
        m_tipsCard->style()->polish(m_tipsCard);

        if (r.autoBook) {
            // 手册 3.3.4：重症自动预约，不能手动修改；确认后报 toast #18（MD §8.2/§9）
            AutoApptDialog d(r.type, window());
            if (d.exec() == QDialog::Accepted) {
                pd::Appointment a;
                a.date = def::DEMO_DATE; a.week = def::DEMO_WEEK;
                a.doc  = def::DEMO_DOC;
                a.slot = def::DEMO_SLOT;
                a.booked = true; a.soon = true;
                emit appointmentAdded(a, true);
                Toast::show(window(), QStringLiteral("挂号成功！单号 %1：脾胃科 赵国安 今晚 19:00，请按时就诊。")
                            .arg(def::ghNo()));
            }
        } else {
            Toast::show(window(), QStringLiteral("检测完成：%1 · %2 · 置信度 %3")
                        .arg(r.type, r.state, r.conf), false);
        }
    });
}

void DetectView::tickScan()
{
    m_scanY += 6;
    if (m_scanY > m_screen->height() - 24) m_scanY = 20;
    m_scan->setGeometry(m_screen->width() / 10, m_scanY,
                        m_screen->width() * 8 / 10, 3);
}

/* 再检测一次（MD §6.4）：清空结果并循环到下一类演示结果 */
void DetectView::onReset()
{
    static const QVector<pd::DetectResult> lib = detLib();
    m_detIdx = (m_detIdx + 1) % lib.size();
    clearResult();
    Toast::show(window(), QStringLiteral("已重置，本次将模拟「%1」结果")
                .arg(lib[m_detIdx].type));
}

/* 结果区恢复默认态（三格/报告/建议/重症红边全部复位） */
void DetectView::clearResult()
{
    m_type->setText(QStringLiteral("—"));
    m_state->setText(QStringLiteral("—"));
    m_state->setStyleSheet(QString());
    m_conf->setText(QStringLiteral("—"));
    m_repBody->setText(QStringLiteral("还没有检测结果，请点击「开始检测」。"));
    m_repNote->hide();
    m_tipsBody->setText(QStringLiteral("检测完成后，这里会根据您的舌头情况给出饮食、作息、调理建议。"));
    m_repCard->setObjectName(QStringLiteral("card"));
    m_tipsCard->setObjectName(QStringLiteral("card"));
    m_repCard->style()->unpolish(m_repCard);
    m_repCard->style()->polish(m_repCard);
    m_tipsCard->style()->unpolish(m_tipsCard);
    m_tipsCard->style()->polish(m_tipsCard);
}

/* 四角取景框跟随画面尺寸摆位 */
void DetectView::placeCorners()
{
    if (m_corners.size() != 4) return;
    const int S = 34, M = 12;
    const int W = m_screen->width(), H = m_screen->height();
    m_corners[0]->setGeometry(M, M, S, S);
    m_corners[1]->setGeometry(W - M - S, M, S, S);
    m_corners[2]->setGeometry(M, H - M - S, S, S);
    m_corners[3]->setGeometry(W - M - S, H - M - S, S, S);
    foreach (QFrame* c, m_corners) c->raise();
}

bool DetectView::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_screen && ev->type() == QEvent::Resize)
        placeCorners();
    return QWidget::eventFilter(obj, ev);
}

/* 手册 3.3.4：手动预约四级联动，数据来自系统排班 */
void DetectView::onBook()
{
    ApptDialog d(sched(), window());
    connect(&d, &ApptDialog::incomplete, this, [this, &d] {
        Toast::show(&d, QStringLiteral("请按顺序选完：科室、医生、日期、时段"), true);
    });
    if (d.exec() == QDialog::Accepted) {
        const QStringList r = d.result();
        // 手册：预约记录提交后，首页【待办事项】同步显示
        const QString dayFull = r.at(2);   // "10-31（周六）"
        pd::Appointment a;
        a.date = dayFull.section(QStringLiteral("（"), 0, 0);
        a.week = dayFull.contains(QStringLiteral("（"))
                     ? dayFull.section(QStringLiteral("（"), 1).chopped(1) : QString();
        a.doc  = r.at(0) + QStringLiteral(" · ") +
                 r.at(1).section(QStringLiteral("（"), 0, 0);          // "脾胃科 · 陈志远"
        a.slot = r.at(3);
        a.booked = true; a.soon = false;
        emit appointmentAdded(a, false);
        Toast::show(window(), QStringLiteral("挂号成功！单号 %1：%2 · %3 · %4 · %5。"
                                            "首页「待办事项」可查看。")
                    .arg(def::ghNo(), r.at(0), r.at(1), r.at(2), r.at(3)), false);
    }
}
