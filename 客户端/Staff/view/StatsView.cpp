#include "StatsView.h"
#include "util/Toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPainter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStyle>
#include <algorithm>

// ==================== 自绘条形图 ====================

// 医生统计行顺序对应的科室名（表格与 CSV 导出共用，防两处漂移）
static const QStringList kDeptNames = { QStringLiteral("中医内科"),
                                        QStringLiteral("中医内科"),
                                        QStringLiteral("针灸推拿科") };
void BarChart::setData(const QVector<int>& vals, const QStringList& names)
{
    m_vals = vals;
    m_names = names;
    update();
}

void BarChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int n = m_vals.size();
    if (n == 0) return;
    const int maxV = *std::max_element(m_vals.begin(), m_vals.end());
    const int padT = 34, padB = 30, padX = 30;
    const double slot = double(width() - padX * 2) / n;
    const double barW = slot * 0.45;
    const int hZone = height() - padT - padB;

    QFont old = p.font();
    for (int i = 0; i < n; ++i) {
        const double h = maxV > 0 ? double(m_vals[i]) / maxV * hZone : 0;
        const double x = padX + slot * i + (slot - barW) / 2;
        const double y = padT + (hZone - h);
        p.setBrush(QColor("#2e5e50"));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(x, y, barW, h), 6, 6);
        // 顶部数值 / 底部医生名
        p.setPen(QColor("#262620"));
        p.setFont(old);
        QFont f = old; f.setBold(true);
        p.setFont(f);
        p.drawText(QRectF(padX + slot * i, y - 28, slot, 24), Qt::AlignCenter,
                   QString::number(m_vals[i]));
        p.setFont(old);
        p.drawText(QRectF(padX + slot * i, height() - padB + 4, slot, 24), Qt::AlignCenter,
                   m_names.value(i));
    }
}

// ==================== 页面 ====================
StatsView::StatsView(QWidget* parent) : QWidget(parent)
{
    setObjectName("pageStats");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(10);

    // ===== 工具栏：周期切换 =====
    QWidget* bar = new QWidget;
    QHBoxLayout* bh = new QHBoxLayout(bar);
    bh->setContentsMargins(16, 12, 16, 4);
    QLabel* title = new QLabel(QStringLiteral("诊疗工作量统计"));
    title->setObjectName("chTitle");
    m_wk = new QPushButton(QStringLiteral("本周"));
    m_wk->setObjectName("btnPri");
    m_wk->setCursor(Qt::PointingHandCursor);
    m_mo = new QPushButton(QStringLiteral("本月"));
    m_mo->setObjectName("btnOut");
    m_mo->setCursor(Qt::PointingHandCursor);
    bh->addWidget(title);
    bh->addStretch();
    bh->addWidget(m_wk);
    bh->addWidget(m_mo);
    root->addWidget(bar);

    // ===== 汇总三格 =====
    QWidget* stats = new QWidget;
    QHBoxLayout* sh = new QHBoxLayout(stats);
    sh->setContentsMargins(0, 6, 0, 6);
    sh->setSpacing(10);
    const char* caps[3] = { "问诊总人次", "接诊率", "平均处方药材数" };
    for (int i = 0; i < 3; ++i) {
        QFrame* f = new QFrame;
        f->setObjectName("stat");
        QVBoxLayout* fv = new QVBoxLayout(f);
        fv->setContentsMargins(10, 10, 10, 10);
        QLabel* c = new QLabel(QString::fromUtf8(caps[i]));
        c->setObjectName("statTitle");
        c->setAlignment(Qt::AlignCenter);
        m_sum[i] = new QLabel;
        m_sum[i]->setObjectName("statVal");
        m_sum[i]->setAlignment(Qt::AlignCenter);
        fv->addWidget(c);
        fv->addWidget(m_sum[i]);
        sh->addWidget(f, 1);
    }
    root->addWidget(stats);

    // ===== 表格 + 图表 =====
    QHBoxLayout* cols = new QHBoxLayout;
    cols->setSpacing(10);

    QFrame* tc = new QFrame;
    tc->setObjectName("card");
    QVBoxLayout* tv = new QVBoxLayout(tc);
    tv->setContentsMargins(0, 0, 0, 0);
    QLabel* tt = new QLabel(QStringLiteral("医生统计"));
    tt->setObjectName("chTitle");
    tt->setContentsMargins(14, 12, 14, 8);
    tv->addWidget(tt);
    m_table = new QTableWidget(0, 5);
    m_table->setObjectName("tb");
    m_table->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("医生") << QStringLiteral("科室") << QStringLiteral("问诊数")
        << QStringLiteral("已完成") << QStringLiteral("接诊率"));
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tv->addWidget(m_table, 1);
    cols->addWidget(tc, 3);

    QFrame* cc = new QFrame;
    cc->setObjectName("card");
    QVBoxLayout* cv = new QVBoxLayout(cc);
    cv->setContentsMargins(0, 0, 0, 0);
    QLabel* ct = new QLabel(QStringLiteral("问诊量对比"));
    ct->setObjectName("chTitle");
    ct->setContentsMargins(14, 12, 14, 8);
    cv->addWidget(ct);
    m_chart = new BarChart;
    m_chart->setMinimumHeight(200);
    cv->addWidget(m_chart, 1);
    cols->addWidget(cc, 2);
    root->addLayout(cols, 1);

    // ===== 底部导出 =====
    QWidget* acts = new QWidget;
    QHBoxLayout* ah = new QHBoxLayout(acts);
    ah->setContentsMargins(0, 0, 0, 0);
    QPushButton* csv = new QPushButton(QStringLiteral("导出 Excel（CSV）"));
    csv->setObjectName("btnOut");
    csv->setCursor(Qt::PointingHandCursor);
    ah->addStretch();
    ah->addWidget(csv);
    root->addWidget(acts);

    connect(m_wk, &QPushButton::clicked, this, &StatsView::onWeek);
    connect(m_mo, &QPushButton::clicked, this, &StatsView::onMonth);
    connect(csv, &QPushButton::clicked, this, &StatsView::exportCsv);

    refresh();
}

void StatsView::onWeek()
{
    m_month = false;
    m_wk->setObjectName("btnPri");
    m_mo->setObjectName("btnOut");
    repolish();
    refresh();
}

void StatsView::onMonth()
{
    m_month = true;
    m_wk->setObjectName("btnOut");
    m_mo->setObjectName("btnPri");
    repolish();
    refresh();
}

/* objectName 互换后必须 unpolish/polish 才会重算 QSS 配色 */
void StatsView::repolish()
{
    foreach (QPushButton* b, QList<QPushButton*>() << m_wk << m_mo) {
        b->style()->unpolish(b);
        b->style()->polish(b);
    }
}

void StatsView::refresh()
{
    // 演示数据（本月=本周×3.3 上下取整）
    const int k = m_month ? 3 : 1;
    m_rows.clear();
    sdt::StatRow r;
    r.doctor = QStringLiteral("张主任"); r.total = 52 * k; r.done = 48 * k; m_rows.append(r);
    r.doctor = QStringLiteral("李医生"); r.total = 41 * k; r.done = 36 * k; m_rows.append(r);
    r.doctor = QStringLiteral("王医生"); r.total = 35 * k; r.done = 33 * k; m_rows.append(r);

    int sumT = 0, sumD = 0;
    const QStringList& depts = kDeptNames;
    m_table->setRowCount(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        const sdt::StatRow& s = m_rows[i];
        sumT += s.total; sumD += s.done;
        m_table->setRowHeight(i, 46);
        const int rate = s.total ? s.done * 100 / s.total : 0;
        const QString cols[5] = { s.doctor, depts.value(i), QString::number(s.total),
                                  QString::number(s.done), QStringLiteral("%1%").arg(rate) };
        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem* it = new QTableWidgetItem(cols[c]);
            it->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(i, c, it);
        }
    }
    m_sum[0]->setText(QString::number(sumT));
    m_sum[1]->setText(QStringLiteral("%1%").arg(sumT ? sumD * 100 / sumT : 0));
    m_sum[2]->setText(QStringLiteral("4.2"));

    QVector<int> vals;
    QStringList names;
    foreach (const sdt::StatRow& s, m_rows) { vals << s.total; names << s.doctor; }
    m_chart->setData(vals, names);
}

void StatsView::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出统计"), QStringLiteral("工作量统计.csv"),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Toast::show(window(), QStringLiteral("文件创建失败，请检查路径"), true);
        return;
    }
    QTextStream ts(&f);                  // Qt6 默认 UTF-8
    ts.setGenerateByteOrderMark(true);
    ts << QStringLiteral("医生,科室,问诊数,已完成,接诊率\n");
    const QStringList& depts = kDeptNames;
    for (int i = 0; i < m_rows.size(); ++i) {
        const sdt::StatRow& s = m_rows[i];
        const int rate = s.total ? s.done * 100 / s.total : 0;
        ts << s.doctor << ',' << depts.value(i) << ',' << s.total << ',' << s.done
           << ',' << rate << "%\n";
    }
    f.close();
    Toast::show(window(), QStringLiteral("已导出：%1").arg(path));
}
