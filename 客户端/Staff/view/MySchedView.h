#ifndef MYSCHEDVIEW_H
#define MYSCHEDVIEW_H
// 医生端·个人排班周视图（手册 3.4.7 拓展）：行=三班次，列=周一..周日，只读
#include <QWidget>
#include <QDate>
#include <QVector>
#include "util/StaffTypes.h"

class QTableWidget;
class QLabel;

class MySchedView : public QWidget
{
    Q_OBJECT
public:
    explicit MySchedView(QWidget* parent = nullptr);

private slots:
    void onPrevWeek();   // 上一周：周起始日 -7 天后整表刷新
    void onNextWeek();   // 下一周：周起始日 +7 天后整表刷新

private:
    void rebuild();      // 按当前周起始日刷新周标签、表头日期与表格内容

    QDate m_weekStart;                   // 当前显示周的周一（演示基准 2026-10-27）
    QVector<sdt::SchedCell> m_cells;     // 演示排班（张主任，date 存基准周日期）
    QTableWidget* m_table = nullptr;     // 周视图表格（3 班次 × 7 天）
    QLabel* m_weekLabel = nullptr;       // 当前周标签（如 10-27 ~ 11-02）
};

#endif // MYSCHEDVIEW_H
