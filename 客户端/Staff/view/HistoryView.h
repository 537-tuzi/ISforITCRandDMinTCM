#ifndef HISTORYVIEW_H
#define HISTORYVIEW_H
// 医生端·历史诊疗病例查看（表5第5行，按姓名/时间筛选）
#include <QWidget>
#include <QString>
#include <QVector>
#include "util/StaffTypes.h"

class QLineEdit;
class QComboBox;
class QTableWidget;
class QLabel;

class HistoryView : public QWidget
{
    Q_OBJECT
public:
    explicit HistoryView(QWidget* parent = nullptr);

private slots:
    void onQuery();   // 查询：按姓名包含 + 时间范围过滤（演示口径）
    void onReset();   // 重置：清空条件，回到全量列表

private:
    // 演示病例行：契约结构 VisitCase + 首列病例号（VisitCase 无病例号字段，外包一层）
    struct CaseRow { QString no; sdt::VisitCase data; };

    void buildDemoData();         // 构造 6 条演示病例（含处方）
    void refreshTable();          // 按当前筛选条件重填表格与计数
    void openDetail(int index);   // 病例详情弹窗（index 为 m_all 下标）

    QVector<CaseRow> m_all;         // 全量演示病例
    QVector<int> m_rows;            // 当前筛选命中行（存 m_all 下标）
    QLineEdit* m_name = nullptr;    // 患者姓名筛选
    QComboBox* m_time = nullptr;    // 诊疗时间范围筛选
    QTableWidget* m_table = nullptr;
    QLabel* m_count = nullptr;      // 底部"共 N 条"计数
};

#endif // HISTORYVIEW_H
