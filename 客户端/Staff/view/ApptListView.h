#ifndef APPTLISTVIEW_H
#define APPTLISTVIEW_H
// 医生端·当日预约患者列表（手册 3.4.2）：当日预约患者 + 舌苔结果 + 查看报告弹窗 + 翻页
#include <QWidget>
#include <QVector>
#include "util/StaffTypes.h"

class QTableWidget;
class QResizeEvent;

class ApptListView : public QWidget
{
    Q_OBJECT
public:
    explicit ApptListView(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* ev) override;   // 800 设计基准：随窗口比例调表格行高（同 Patient 记录页）

private:
    void loadDemoData();      // 演示数据（正式版：服务端按当日排班下发，脱敏字段同源）
    void fillTable();         // 表格装配：已问诊/已失约置灰，重症舌苔红字
    void openReport(int row); // 查看检测报告弹窗

    QTableWidget* m_table = nullptr;
    QVector<sdt::PatientRow> m_rows;       // 行数据（与表格行一一对应）
    QVector<sdt::TongueReport> m_reports;  // 各行对应的舌苔检测报告
};

#endif // APPTLISTVIEW_H
