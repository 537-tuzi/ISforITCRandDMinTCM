#ifndef DEPTVIEW_H
#define DEPTVIEW_H
// 管理员端·科室档案管理（手册 3.5.2 拓展：增改删，删除为带约束逻辑删）
#include <QWidget>
#include <QVector>
#include "util/StaffTypes.h"

class QTableWidget;

class DeptView : public QWidget
{
    Q_OBJECT
public:
    explicit DeptView(QWidget* parent = nullptr);

private:
    void rebuild();              // 依 m_rows 整表重建（新增/编辑/停用后刷新）
    void openEditor(int row);    // 弹新增/编辑窗：row<0 新增，>=0 编辑该行
    void tryDisable(int row);    // 停用流程：确认弹窗 → 前置约束校验 → 逻辑删

    QTableWidget* m_table = nullptr;  // 科室列表（objectName=tb）
    QVector<sdt::DeptRow> m_rows;     // 科室档案（演示数据；status 0=停用 1=正常）
    QVector<int> m_doctors;           // 在职医师数列（演示数据，下标与 m_rows 对齐）
};

#endif // DEPTVIEW_H
