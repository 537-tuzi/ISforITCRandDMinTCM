#ifndef STAFFDIALOGS_H
#define STAFFDIALOGS_H
// 工作人员端弹窗集（对应手册 3.4/3.5 各详设）
#include <QDialog>
#include "util/StaffTypes.h"

// 修改密码（手册 3.4.1/3.5.1+表5/6：验证原密码；新密码 6-16 位含字母+数字）
class ChangePwdDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChangePwdDialog(QWidget* parent = nullptr);
};

#endif // STAFFDIALOGS_H
