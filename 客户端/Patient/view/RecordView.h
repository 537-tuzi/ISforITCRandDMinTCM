#ifndef RECORDVIEW_H
#define RECORDVIEW_H
// 就诊记录页：左信息面板 + 最近一次 + 全部记录表 + 报告弹窗（自包含演示数据，对应 MD §7）
#include <QWidget>
#include <QVector>
#include "util/PatientTypes.h"

class InfoPanel;
class QTableWidget;

class RecordView : public QWidget
{
    Q_OBJECT
public:
    explicit RecordView(QWidget* parent = nullptr);

signals:
    void logoutClicked();

protected:
    void resizeEvent(QResizeEvent* ev) override;   // 随窗口比例调面板宽/表格行高

private slots:
    void openReport(int index);   // 查看报告弹窗（内容同"最近一次诊疗记录"信息，手册 3.3.5）
    void onPrev();
    void onNext();

private:
    void initTable();             // 填充表格行与操作按钮
    void applyRowHeight(int h);   // 表格行高统一设置

    InfoPanel* m_panel = nullptr;
    QTableWidget* m_table = nullptr;
    QVector<pd::VisitRecord> m_visits;
};

#endif
