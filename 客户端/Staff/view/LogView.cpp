#include "LogView.h"
#include "util/Toast.h"
#include "util/AppDefs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QBrush>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QResizeEvent>

LogView::LogView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageLogs");
    initDemoData();

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    // ===== 筛选卡：操作人 + 类型 + 结果 + 查询/重置 + 导出 =====
    QFrame* fc = new QFrame;
    fc->setObjectName("card");
    QHBoxLayout* fh = new QHBoxLayout(fc);
    fh->setContentsMargins(16, 12, 16, 12);
    fh->setSpacing(10);

    QLabel* l1 = new QLabel(QStringLiteral("操作人"));
    l1->setObjectName("fldLabel");
    m_oper = new QLineEdit;
    m_oper->setPlaceholderText(QStringLiteral("按操作人筛选"));
    m_oper->setClearButtonEnabled(true);
    m_oper->setMinimumWidth(150);

    QLabel* l2 = new QLabel(QStringLiteral("类型"));
    l2->setObjectName("fldLabel");
    m_type = new QComboBox;
    m_type->addItems(QStringList() << QStringLiteral("全部") << QStringLiteral("登录")
                                   << QStringLiteral("预约") << QStringLiteral("排班")
                                   << QStringLiteral("病例") << QStringLiteral("日志查询")
                                   << QStringLiteral("系统"));

    QLabel* l3 = new QLabel(QStringLiteral("结果"));
    l3->setObjectName("fldLabel");
    m_res = new QComboBox;
    m_res->addItems(QStringList() << QStringLiteral("全部") << QStringLiteral("成功")
                                  << QStringLiteral("失败"));

    QPushButton* query = new QPushButton(QStringLiteral("查询"));
    query->setObjectName("btnOut");
    query->setCursor(Qt::PointingHandCursor);
    QPushButton* reset = new QPushButton(QStringLiteral("重置"));
    reset->setObjectName("btnOut");
    reset->setCursor(Qt::PointingHandCursor);
    QPushButton* csv = new QPushButton(QStringLiteral("导出 Excel（CSV）"));
    csv->setObjectName("btnOut");
    csv->setCursor(Qt::PointingHandCursor);

    fh->addWidget(l1);
    fh->addWidget(m_oper, 1);
    fh->addWidget(l2);
    fh->addWidget(m_type);
    fh->addWidget(l3);
    fh->addWidget(m_res);
    fh->addWidget(query);
    fh->addWidget(reset);
    fh->addStretch();
    fh->addWidget(csv);
    root->addWidget(fc);

    // ===== 日志表格卡 =====
    QFrame* lc = new QFrame;
    lc->setObjectName("card");
    QVBoxLayout* lv = new QVBoxLayout(lc);
    lv->setContentsMargins(0, 0, 0, 0);
    QWidget* head = new QWidget;
    QHBoxLayout* hh = new QHBoxLayout(head);
    hh->setContentsMargins(16, 12, 16, 8);
    QLabel* title = new QLabel(QStringLiteral("操作日志"));
    title->setObjectName("chTitle");
    m_okPill = new QLabel;
    m_okPill->setObjectName("pillOk");
    m_ngPill = new QLabel;
    m_ngPill->setObjectName("pillSoon");
    hh->addWidget(title);
    hh->addStretch();
    hh->addWidget(m_okPill);
    hh->addWidget(m_ngPill);
    lv->addWidget(head);

    QWidget* body = new QWidget;
    QVBoxLayout* bv = new QVBoxLayout(body);
    bv->setContentsMargins(16, 2, 16, 10);
    bv->setSpacing(0);

    m_table = new QTableWidget(0, 6);
    m_table->setObjectName("tb");
    QStringList heads;
    heads << QStringLiteral("时间") << QStringLiteral("操作人") << QStringLiteral("类型")
          << QStringLiteral("操作内容") << QStringLiteral("结果") << QStringLiteral("来源IP");
    m_table->setHorizontalHeaderLabels(heads);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(false);
    // 前 5 列按内容收缩，给"操作内容"留弹性：内容列改 Stretch
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    bv->addWidget(m_table, 1);

    m_page = new QLabel;
    m_page->setObjectName("pagerTxt");
    m_page->setAlignment(Qt::AlignCenter);
    bv->addSpacing(8);
    bv->addWidget(m_page);
    lv->addWidget(body, 1);
    root->addWidget(lc, 1);

    connect(query, &QPushButton::clicked, this, &LogView::applyFilter);
    connect(m_oper, &QLineEdit::returnPressed, this, &LogView::applyFilter);
    connect(reset, &QPushButton::clicked, this, &LogView::resetFilter);
    connect(csv, &QPushButton::clicked, this, &LogView::exportCsv);

    fillTable();   // 首次进入全量
}

/* 演示 10 条（字段与 zysms_db.system_log 对应，覆盖各类型，含 2 条失败） */
void LogView::initDemoData()
{
    struct Row { const char* t; const char* o; const char* ty; const char* d; const char* r; const char* ip; };
    const Row rows[] = {
        {"2026-10-30 09:01:12", "张三",   "登录",     "医保账号登录",                       "成功", "192.168.25.5"},
        {"2026-10-30 09:03:40", "张三",   "预约",     "挂号：脾胃科·张主任 上午班",         "成功", "192.168.25.5"},
        {"2026-10-30 09:05:11", "李四",   "登录",     "医保登录失败（密码错误×3，锁定15分钟）", "失败", "192.168.25.6"},
        {"2026-10-30 09:10:02", "张主任", "登录",     "工号登录（100002）",                 "成功", "192.168.25.7"},
        {"2026-10-30 09:15:30", "张主任", "病例",     "生成病例 BL20261030001（张三）",     "成功", "192.168.25.7"},
        {"2026-10-30 09:20:18", "张主任", "病例",     "开具处方（5 味药材）",               "成功", "192.168.25.7"},
        {"2026-10-30 09:40:55", "系统管理员", "排班", "新增排班：李医生 10-31 晚班",        "成功", "192.168.25.8"},
        {"2026-10-30 10:00:00", "系统",   "系统",     "数据库定时备份（保留30天）",         "成功", "127.0.0.1"},
        {"2026-10-30 10:12:47", "系统管理员", "日志查询", "筛选：类型=登录 结果=失败",      "成功", "192.168.25.8"},
        {"2026-10-30 10:30:03", "王五",   "预约",     "自动挂号失败（号源已满）",           "失败", "192.168.25.9"},
    };
    for (int i = 0; i < 10; ++i) {
        sdt::LogRow r;
        r.time = QString::fromUtf8(rows[i].t);
        r.oper = QString::fromUtf8(rows[i].o);
        r.type = QString::fromUtf8(rows[i].ty);
        r.desc = QString::fromUtf8(rows[i].d);
        r.result = QString::fromUtf8(rows[i].r);
        r.ip = QString::fromUtf8(rows[i].ip);
        m_all.append(r);
    }
}

/* 查询：操作人包含 + 类型/结果相等（"全部"跳过） */
void LogView::applyFilter()
{
    m_view.clear();
    const QString key = m_oper->text().trimmed();
    const QString ty = m_type->currentText();
    const QString rs = m_res->currentText();
    for (const sdt::LogRow& r : m_all) {
        if (!key.isEmpty() && !r.oper.contains(key)) continue;
        if (ty != QStringLiteral("全部") && r.type != ty) continue;
        if (rs != QStringLiteral("全部") && r.result != rs) continue;
        m_view.append(r);
    }
    if (m_view.isEmpty())
        Toast::show(window(), QStringLiteral("未找到符合条件的日志"));
    fillTable();
}

void LogView::resetFilter()
{
    m_oper->clear();
    m_type->setCurrentIndex(0);
    m_res->setCurrentIndex(0);
    m_view = m_all;
    fillTable();
}

void LogView::fillTable()
{
    m_table->setRowCount(m_view.size());   // 空筛选结果保持空表（与"未找到"toast 一致）
    int ok = 0;
    for (int i = 0; i < m_view.size(); ++i) {
        const sdt::LogRow& r = m_view[i];
        if (r.result == QStringLiteral("成功")) ++ok;
        m_table->setRowHeight(i, 46);
        const QString cols[6] = { r.time, r.oper, r.type, r.desc, r.result, r.ip };
        for (int c = 0; c < 6; ++c) {
            QTableWidgetItem* it = new QTableWidgetItem(cols[c]);
            it->setTextAlignment(Qt::AlignCenter);
            if (c == 4)   // 结果列：成功墨绿 / 失败朱砂
                it->setForeground(QBrush(r.result == QStringLiteral("成功")
                                             ? QColor("#234b40") : QColor("#b23b2e")));
            else if (c == 3)
                it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_table->setItem(i, c, it);
        }
    }
    m_okPill->setText(QStringLiteral("成功 %1").arg(ok));
    m_ngPill->setText(QStringLiteral("失败 %1").arg(m_view.size() - ok));
    m_page->setText(QStringLiteral("共 %1 条 · 第 1/1 页").arg(m_view.size()));
}

/* 导出当前筛选结果（UTF-8 带 BOM，Excel 双击可直接识别中文） */
void LogView::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出日志"), QStringLiteral("系统日志.csv"),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Toast::show(window(), QStringLiteral("文件创建失败，请检查路径"), true);
        return;
    }
    QTextStream ts(&f);                  // Qt6 默认 UTF-8
    ts.setGenerateByteOrderMark(true);   // BOM
    ts << QStringLiteral("时间,操作人,类型,操作内容,结果,来源IP\n");
    for (const sdt::LogRow& r : m_view)
        ts << r.time << ',' << r.oper << ',' << r.type << ','
           << QString(r.desc).remove(',') << ',' << r.result << ',' << r.ip << '\n';
    f.close();
    Toast::show(window(), QStringLiteral("已导出：%1").arg(path));
}

/* 行高随窗口缩放（46 为 800 宽基准） */
void LogView::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    if (!m_table) return;
    const qreal s = def::FONT_COMP * qMax<qreal>(1.0, width() / 800.0);
    const int h = qRound(46 * s);
    for (int i = 0; i < m_table->rowCount(); ++i)
        m_table->setRowHeight(i, h);
}
