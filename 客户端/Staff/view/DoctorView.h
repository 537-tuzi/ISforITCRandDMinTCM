#ifndef DOCTORVIEW_H
#define DOCTORVIEW_H
// 管理员端·医师档案管理（手册 3.5.3 拓展：增删改查+重置密码，删除为逻辑删）
#include <QWidget>
#include <QString>
#include <QVector>
#include "util/StaffTypes.h"

class QTableWidget;
class QLineEdit;

class DoctorView : public QWidget
{
    Q_OBJECT
public:
    explicit DoctorView(QWidget* parent = nullptr);

private slots:
    void onSearch();            // 搜索（演示：姓名/工号包含匹配）
    void onAdd();               // 新增医师（工号自动分配 6 位，初始密码 123456）
    void onEdit(int idx);       // 编辑医师信息（工号只读）
    void onResetPwd(int idx);   // 重置密码为默认 123456
    void onRemove(int idx);     // 删除医师（逻辑删，历史诊疗数据保留）

private:
    void renderTable();         // 按当前搜索词重建表格

    QVector<sdt::DoctorRow> m_rows;   // 演示数据（内存态；接服务端后换查询结果）
    QString m_key;                    // 当前搜索词（空=显示全部）
    QTableWidget* m_tb = nullptr;     // 医师列表表格
    QLineEdit* m_edSearch = nullptr;  // 搜索输入框
};

#endif // DOCTORVIEW_H
