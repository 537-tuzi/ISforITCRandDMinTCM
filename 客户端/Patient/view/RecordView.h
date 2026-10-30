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

    void setProfile(const QString& gm, const QString& jw);
    void setJdDone(bool done);       // 建档状态同步到左面板
    void setPanelWidth(int w);       // 窗口放大时左面板同步加宽
    void setRowHeight(int h);        // 窗口放大时表格行高同步加高

signals:
    void logoutClicked();

private slots:
    void openReport(int index);   // 查看报告弹窗（内容同"最近一次诊疗记录"信息，手册 3.3.5）
    void onPrev();
    void onNext();

private:
    void initTable();             // 填充表格行与操作按钮

    InfoPanel* m_panel = nullptr;
    QTableWidget* m_table = nullptr;
    QVector<pd::VisitRecord> m_visits;
};

#endif
