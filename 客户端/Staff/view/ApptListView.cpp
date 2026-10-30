#include "ApptListView.h"
#include "util/Toast.h"
#include "util/StaffTypes.h"
#include "util/UIFactory.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDate>
#include <QDialog>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QBrush>
#include <QResizeEvent>
#include <QtGlobal>

/* ============ 检测报告弹窗（手册 3.4.2"查看报告"） ============
   深底舌照占位卡（原型阶段无真实图片，图片规划走网络通道）+ 四项检测字段；
   重症患者追加红底"优先接诊"提示（灰黑苔/镜面舌=重症，触发系统自动预约）。 */
static void showReportDialog(const QString& name, const sdt::TongueReport& r, QWidget* win)
{
    QDialog dlg(win);
    dlg.setFixedWidth(560);
    dlg.setObjectName("modal");
    QVBoxLayout* mv = new QVBoxLayout(&dlg);
    mv->setContentsMargins(0, 0, 0, 0);
    mv->setSpacing(0);

    // ---- 标题栏：检测报告·姓名 ----
    QWidget* head = new QWidget;
    head->setObjectName("mHead");
    QHBoxLayout* hl = new QHBoxLayout(head);
    hl->setContentsMargins(18, 10, 18, 10);
    QLabel* title = new QLabel(QStringLiteral("检测报告·%1").arg(name));
    title->setObjectName("mTitle");
    hl->addWidget(title);
    hl->addStretch();
    mv->addWidget(head);

    // ---- 正文：左舌头照片占位（深底）+ 右检测情况 ----
    QWidget* body = new QWidget;
    body->setObjectName("mBody");
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(18, 12, 18, 12);
    bv->setSpacing(12);

    QHBoxLayout* cards = new QHBoxLayout;
    cards->setSpacing(14);

    QFrame* ph = new QFrame;
    ph->setObjectName("phCardDark");          // 舌头照片深底取景框占位
    ph->setMinimumHeight(150);
    QVBoxLayout* phv = new QVBoxLayout(ph);
    phv->setContentsMargins(12, 10, 12, 10);
    QLabel* phTitle = new QLabel(QStringLiteral("舌头照片"));
    phTitle->setObjectName("chSub");
    phv->addWidget(phTitle);
    QLabel* wm = new QLabel(QStringLiteral("👅"));   // 舌苔水印占位
    wm->setAlignment(Qt::AlignCenter);
    phv->addWidget(wm, 1);
    cards->addWidget(ph, 1);

    QFrame* dc = new QFrame;
    dc->setObjectName("phCard");
    dc->setMinimumHeight(150);
    QVBoxLayout* dcv = new QVBoxLayout(dc);
    dcv->setContentsMargins(14, 10, 14, 8);
    QLabel* dcTitle = new QLabel(QStringLiteral("检测情况"));
    dcTitle->setObjectName("chSub");
    dcv->addWidget(dcTitle);
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("舌苔类型"), r.type));
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("健康状态"), r.state));
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("置信度"), r.conf));
    dcv->addWidget(uif::makeFieldRow(QStringLiteral("检测时间"), r.when));
    cards->addWidget(dc, 1);
    bv->addLayout(cards);

    if (r.severe) {
        QLabel* warn = new QLabel(
            QStringLiteral("重症提示：该患者舌象为重症，请优先安排问诊。"));
        warn->setObjectName("noteRed");
        warn->setWordWrap(true);
        bv->addWidget(warn);
    }

    mv->addWidget(body, 1);

    // ---- 底部：关闭 ----
    QWidget* foot = new QWidget;
    foot->setObjectName("mFoot");
    QHBoxLayout* fl = new QHBoxLayout(foot);
    fl->setContentsMargins(18, 10, 18, 10);
    fl->addStretch();
    QPushButton* close = new QPushButton(QStringLiteral("关闭"));
    close->setObjectName("btnPri");
    close->setCursor(Qt::PointingHandCursor);
    QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
    fl->addWidget(close);
    mv->addWidget(foot);

    dlg.exec();
}

/* ============ 页面 ============ */
ApptListView::ApptListView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageAppt");
    loadDemoData();

    // 根布局：800×640 设计基准
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    // ===== 单卡：标题栏（标题+当日日期角注）→ 患者表格 → 底部翻页 =====
    QFrame* card = new QFrame;
    card->setObjectName("card");
    QVBoxLayout* cv = new QVBoxLayout(card);
    cv->setContentsMargins(0, 0, 0, 0);
    cv->setSpacing(0);

    QWidget* bar = new QWidget;
    bar->setObjectName("chBar");
    QHBoxLayout* bh = new QHBoxLayout(bar);
    bh->setContentsMargins(14, 10, 14, 10);
    QLabel* title = new QLabel(QStringLiteral("当日预约患者"));
    title->setObjectName("chTitle");
    bh->addWidget(title);
    bh->addStretch();
    QLabel* dateNote = new QLabel(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
    dateNote->setObjectName("chSub");        // 右侧日期角注
    bh->addWidget(dateNote);
    cv->addWidget(bar);

    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(14, 2, 14, 10);
    bv->setSpacing(0);

    // ---- 患者表格（只读）----
    m_table = new QTableWidget(m_rows.size(), 10);
    m_table->setObjectName("tb");
    QStringList heads;
    heads << QStringLiteral("姓名") << QStringLiteral("性别") << QStringLiteral("年龄")
          << QStringLiteral("身份证号") << QStringLiteral("医保卡号") << QStringLiteral("手机号")
          << QStringLiteral("预约时段") << QStringLiteral("舌苔结果") << QStringLiteral("状态")
          << QStringLiteral("操作");
    m_table->setHorizontalHeaderLabels(heads);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 内容列全部按内容收缩（列宽短、收缩更彻底），腾出的宽度全部给最右"操作"列
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Stretch);
    m_table->setShowGrid(false);
    fillTable();
    bv->addWidget(m_table, 1);

    // ---- 底部翻页（演示数据仅 1 页，到头提示）----
    QWidget* pager = new QWidget;
    QHBoxLayout* pg = new QHBoxLayout(pager);
    pg->setContentsMargins(0, 8, 0, 0);
    QPushButton* prev = new QPushButton(QStringLiteral("上一页"));
    prev->setObjectName("btnOutSm");
    prev->setCursor(Qt::PointingHandCursor);
    QLabel* page = new QLabel(QStringLiteral("共 %1 条 · 第 1/1 页").arg(m_rows.size()));
    page->setObjectName("chSub");
    page->setAlignment(Qt::AlignCenter);
    QPushButton* next = new QPushButton(QStringLiteral("下一页"));
    next->setObjectName("btnOutSm");
    next->setCursor(Qt::PointingHandCursor);
    pg->addWidget(prev);
    pg->addStretch();
    pg->addWidget(page);
    pg->addStretch();
    pg->addWidget(next);
    bv->addWidget(pager);
    connect(prev, &QPushButton::clicked, this, [this] {
        Toast::show(window(), QStringLiteral("已经是第一页了"));
    });
    connect(next, &QPushButton::clicked, this, [this] {
        Toast::show(window(), QStringLiteral("已经是最后一页了"));
    });

    cv->addWidget(body, 1);
    root->addWidget(card, 1);
}

/* 演示数据 5 行（手册 3.4.2）：脱敏身份证/手机号由服务端下发；
   王五灰黑苔=重症（红字+优先接诊），赵六已问诊、孙七已失约（置灰）。 */
void ApptListView::loadDemoData()
{
    const QString today = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const QString slot = def::shiftName(1);   // 上午 08:00-12:00

    sdt::PatientRow p;
    sdt::TongueReport r;

    p.name = QStringLiteral("张三");   p.gender = QStringLiteral("男");
    p.age = QStringLiteral("45");
    p.idCardMasked = QStringLiteral("3301**********2215");
    p.cardNo = QStringLiteral("MP20261024001");  p.phone = QStringLiteral("139****0001");
    p.slot = slot;  p.status = QStringLiteral("已预约");
    r.type = QStringLiteral("白腻苔");  r.conf = QStringLiteral("93%");
    r.state = QStringLiteral("身体有恙（建议就诊）");  r.when = today + QStringLiteral(" 08:12");
    m_rows.append(p);  m_reports.append(r);

    p = sdt::PatientRow();
    p.name = QStringLiteral("李四");   p.gender = QStringLiteral("女");
    p.age = QStringLiteral("32");
    p.idCardMasked = QStringLiteral("3302**********3306");
    p.cardNo = QStringLiteral("MP20261024002");  p.phone = QStringLiteral("138****0002");
    p.slot = slot;  p.status = QStringLiteral("已预约");
    r = sdt::TongueReport();
    r.type = QStringLiteral("薄白苔");  r.conf = QStringLiteral("95%");
    r.state = QStringLiteral("大致正常");  r.when = today + QStringLiteral(" 08:05");
    m_rows.append(p);  m_reports.append(r);

    p = sdt::PatientRow();
    p.name = QStringLiteral("王五");   p.gender = QStringLiteral("男");
    p.age = QStringLiteral("58");
    p.idCardMasked = QStringLiteral("3303**********7719");
    p.cardNo = QStringLiteral("MP20261024003");  p.phone = QStringLiteral("137****0003");
    p.slot = slot;  p.status = QStringLiteral("已预约");  p.severe = true;
    r = sdt::TongueReport();
    r.type = QStringLiteral("灰黑苔");  r.conf = QStringLiteral("91%");
    r.state = QStringLiteral("重症（须尽快就诊）");  r.when = today + QStringLiteral(" 08:20");
    r.severe = true;
    m_rows.append(p);  m_reports.append(r);

    p = sdt::PatientRow();
    p.name = QStringLiteral("赵六");   p.gender = QStringLiteral("女");
    p.age = QStringLiteral("41");
    p.idCardMasked = QStringLiteral("3304**********6652");
    p.cardNo = QStringLiteral("MP20261024004");  p.phone = QStringLiteral("136****0004");
    p.slot = slot;  p.status = QStringLiteral("已问诊");
    r = sdt::TongueReport();
    r.type = QStringLiteral("黄腻苔");  r.conf = QStringLiteral("88%");
    r.state = QStringLiteral("身体有恙（建议就诊）");  r.when = today + QStringLiteral(" 08:26");
    m_rows.append(p);  m_reports.append(r);

    p = sdt::PatientRow();
    p.name = QStringLiteral("孙七");   p.gender = QStringLiteral("男");
    p.age = QStringLiteral("67");
    p.idCardMasked = QStringLiteral("3305**********1293");
    p.cardNo = QStringLiteral("MP20261024005");  p.phone = QStringLiteral("135****0005");
    p.slot = slot;  p.status = QStringLiteral("已失约");
    r = sdt::TongueReport();
    r.type = QStringLiteral("白腻苔");  r.conf = QStringLiteral("90%");
    r.state = QStringLiteral("身体有恙（建议就诊）");  r.when = today + QStringLiteral(" 08:32");
    m_rows.append(p);  m_reports.append(r);
}

/* 表格装配：前 9 列文本居中；重症舌苔红字（#b23b2e），已问诊/已失约整行置灰 */
void ApptListView::fillTable()
{
    const QColor active(QStringLiteral("#234b40"));   // 已预约（待接诊）
    const QColor done(QStringLiteral("#5f5f56"));     // 已问诊/已失约
    const QColor severe(QStringLiteral("#b23b2e"));   // 重症舌苔

    for (int i = 0; i < m_rows.size(); ++i) {
        m_table->setRowHeight(i, 52);
        const sdt::PatientRow& p = m_rows.at(i);
        const sdt::TongueReport& r = m_reports.at(i);
        const QString tongue = r.type + (r.severe ? QStringLiteral("·重症") : QString());
        const bool finished = p.status != QStringLiteral("已预约");

        QStringList cells;
        cells << p.name << p.gender << p.age << p.idCardMasked
              << p.cardNo << p.phone << p.slot << tongue << p.status;
        for (int c = 0; c < 9; ++c) {
            QTableWidgetItem* it = new QTableWidgetItem(cells.at(c));
            it->setTextAlignment(Qt::AlignCenter);
            if (c == 7 && r.severe)
                it->setForeground(QBrush(severe));            // 重症舌苔红字
            else
                it->setForeground(QBrush(finished ? done : active));
            it->setToolTip(cells.at(c));                      // 脱敏字段完整内容以悬浮提示兜底
            m_table->setItem(i, c, it);
        }

        // 操作列：【查看报告】
        QWidget* btnHost = new QWidget;
        QHBoxLayout* bh = new QHBoxLayout(btnHost);
        bh->setContentsMargins(2, 2, 2, 2);
        QPushButton* b = new QPushButton(QStringLiteral("查看报告"));
        b->setObjectName("btnOutSm");
        b->setCursor(Qt::PointingHandCursor);
        bh->addWidget(b);
        m_table->setCellWidget(i, 9, btnHost);
        connect(b, &QPushButton::clicked, this, [this, i] { openReport(i); });
    }
}

/* 查看报告：按行打开检测报告弹窗 */
void ApptListView::openReport(int row)
{
    if (row < 0 || row >= m_rows.size()) return;
    showReportDialog(m_rows.at(row).name, m_reports.at(row), window());
}

/* 窗口比例变化：行高按同一因子等比调整（基准 52px / 800 宽） */
void ApptListView::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    if (!m_table) return;
    const qreal s = def::FONT_COMP * qMax<qreal>(1.0, width() / 800.0);
    const int h = qRound(52 * s);
    for (int i = 0; i < m_table->rowCount(); ++i)
        m_table->setRowHeight(i, h);
}
