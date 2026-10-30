#ifndef SCHEDMGRVIEW_H
#define SCHEDMGRVIEW_H
// 管理员端·排班调度设置（手册 3.5.4：按科室周排班+班次+接诊上限+冲突检测）
#include <QWidget>
#include <QVector>
#include <QStringList>
#include "util/StaffTypes.h"

class QComboBox;
class QTableWidget;
class QLabel;

class SchedMgrView : public QWidget
{
    Q_OBJECT
public:
    explicit SchedMgrView(QWidget* parent = nullptr);

private slots:
    void onDeptChanged();    // 科室切换：联动医生下拉与表格
    void onPrevWeek();
    void onNextWeek();
    void onAddSched();       // 添加排班弹窗（含冲突检测）

private:
    void rebuild();          // 按当前科室重建表格
    struct Doc { QString name; QString dept; };
    QVector<Doc> doctors() const;      // 当前科室的医生
    QStringList cells() const;         // 当前科室的排班格原文列表

    QComboBox* m_dept = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_weekLabel = nullptr;
    int m_weekOff = 0;                    // 相对演示周的偏移（±7 天）
    QStringList m_allCells;               // 演示排班原文：医生|星期几|班次名|已约/上限
};

#endif // SCHEDMGRVIEW_H
