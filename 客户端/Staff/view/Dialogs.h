#ifndef STAFFDIALOGS_H
#define STAFFDIALOGS_H
// 工作人员端弹窗集（对应手册 3.4/3.5 各详设）
#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>
#include "util/StaffTypes.h"

class QPushButton;
class QWidget;

// ---- 通用弹窗骨架（全工程单点维护；各页弹窗统一走这里） ----
// 返回底部按钮列表（文本,objectName），点击行为由调用方显式接线
QList<QPushButton*> setupModal(QDialog* dlg, const QString& title, QWidget* body,
                               const QList<QPair<QString, QString>>& buttons);

// 确认弹窗：标题+正文+取消/确认，返回用户是否点了确认
bool confirmModal(QWidget* parent, const QString& title, const QString& text);

// 修改密码（手册 3.4.1/3.5.1+表5/6：验证原密码；新密码 6-16 位含字母+数字）
class ChangePwdDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChangePwdDialog(QWidget* parent = nullptr);
};

#endif // STAFFDIALOGS_H
