#ifndef STATSVIEW_H
#define STATSVIEW_H
// 管理员端·诊疗工作量统计（手册 3.5.5：周/月统计+条形图+CSV 导出，统计延迟≤10ms）
#include <QWidget>
#include <QVector>
#include "util/StaffTypes.h"

class QTableWidget;
class QLabel;
class QPushButton;

// 自绘条形图（禁止引入图表库）：三根墨绿条+数值+医生名
class BarChart : public QWidget
{
    Q_OBJECT
public:
    void setData(const QVector<int>& vals, const QStringList& names);
protected:
    void paintEvent(QPaintEvent* ev) override;
private:
    QVector<int> m_vals;
    QStringList m_names;
};

class StatsView : public QWidget
{
    Q_OBJECT
public:
    explicit StatsView(QWidget* parent = nullptr);

private slots:
    void onWeek();     // 本周
    void onMonth();    // 本月
    void exportCsv();  // 导出 CSV（UTF-8 BOM）

private:
    void refresh();    // 按当前周期填充汇总/表格/图表
    void repolish();   // 周期按钮 objectName 互换后重算样式

    bool m_month = false;
    QPushButton* m_wk = nullptr;
    QPushButton* m_mo = nullptr;
    QLabel* m_sum[3] = { nullptr, nullptr, nullptr };   // 问诊总人次/接诊率/平均药材数
    QTableWidget* m_table = nullptr;
    BarChart* m_chart = nullptr;
    QVector<sdt::StatRow> m_rows;
};

#endif // STATSVIEW_H
