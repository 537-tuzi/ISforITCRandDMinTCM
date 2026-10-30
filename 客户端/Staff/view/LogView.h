#ifndef LOGVIEW_H
#define LOGVIEW_H
// 管理员端·系统日志管理（手册 3.5.6 拓展：按操作人/类型/结果筛选 + 导出 CSV）
#include <QWidget>
#include <QVector>
#include "util/StaffTypes.h"

class QLineEdit;
class QComboBox;
class QTableWidget;
class QLabel;
class QResizeEvent;

class LogView : public QWidget
{
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = nullptr);

protected:
    // 窗口宽度变化：行高按同一缩放因子跟随（46 为 800 宽基准，同 Patient 记录页模式）
    void resizeEvent(QResizeEvent* ev) override;

private:
    void initDemoData();  // 装载演示 10 条（字段与 zysms_db.system_log 对应）
    void applyFilter();   // 查询：按 操作人/类型/结果 三条件过滤刷新
    void resetFilter();   // 重置：清空条件，恢复全量
    void fillTable();     // 以 m_view 重建表格行与底部计数
    void exportCsv();     // 导出当前筛选结果（UTF-8 带 BOM 的 CSV）

    QLineEdit*    m_oper   = nullptr;  // 操作人关键字输入
    QComboBox*    m_type   = nullptr;  // 操作类型下拉
    QComboBox*    m_res    = nullptr;  // 操作结果下拉
    QTableWidget* m_table  = nullptr;  // 日志表格
    QLabel*       m_page   = nullptr;  // "共 N 条 · 第 1/1 页"
    QLabel*       m_okPill = nullptr;  // 成功条数胶囊
    QLabel*       m_ngPill = nullptr;  // 失败条数胶囊

    QVector<sdt::LogRow> m_all;    // 全量演示数据
    QVector<sdt::LogRow> m_view;   // 当前筛选结果（导出亦按此导）
};

#endif // LOGVIEW_H
