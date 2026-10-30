#ifndef DIAGNOSEVIEW_H
#define DIAGNOSEVIEW_H
// 医生端·问诊开方+标准化病例生成（手册 3.4.3/3.4.4/3.4.5）：四象限布局
#include <QWidget>
#include <QVector>
#include "util/StaffTypes.h"

class QTextEdit;
class QLineEdit;
class QTableWidget;

class DiagnoseView : public QWidget
{
    Q_OBJECT
public:
    explicit DiagnoseView(QWidget* parent = nullptr);

private slots:
    void onAddRx();      // 添加药材（三输入非空校验）
    void onClearRx();    // 清空处方（确认弹窗后清空）
    void onSaveDraft();  // 保存草稿
    void onSubmit();     // 提交并生成病例（校验→确认→toast 病例号）

private:
    QTextEdit*   m_chief = nullptr;   // 主诉
    QLineEdit*  m_diag  = nullptr;    // 中医诊断
    QTableWidget* m_rx  = nullptr;    // 处方药材表
    QLineEdit*  m_inName = nullptr;   // 添加行：药材
    QLineEdit*  m_inDose = nullptr;   // 添加行：剂量
    QLineEdit*  m_inUse  = nullptr;   // 添加行：服用方法
    int m_rxSeq = 3;                  // 演示病例号序号
};

#endif // DIAGNOSEVIEW_H
