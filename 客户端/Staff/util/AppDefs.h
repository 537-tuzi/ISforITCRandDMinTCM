#ifndef APPDEFS_H
#define APPDEFS_H
// 医生端+管理员端全局统一定义（与 Patient 工程 AppDefs 同构，常量按手册工作人员口径）
#include <QString>

namespace def {

// ---- 字体 ----
constexpr qreal FONT_COMP = 1.0;   // 视觉尺寸补偿基线（1.0=不补偿）

// ---- 布局基准 ----
constexpr int TOPBAR_H = 64;       // 顶部导航条高

// ---- 时序参数 ----
constexpr int TOAST_VISIBLE_MS = 3200;  // toast 停留时长
constexpr int TOAST_FADE_MS    = 500;   // toast 淡出时长

// ---- 账号规则（手册 3.2.4/3.4.1）----
constexpr int PWD_MAX_RETRY   = 3;    // 连续错误锁定次数
constexpr int STAFF_LOCK_HOURS = 24;  // 医生/管理员锁定时长（小时，手册 3.2.4）
inline const QString DEMO_SECRET = QStringLiteral("123456");  // 演示密码（默认密码同，3.5.3）

// ---- 演示账号（zysms_db.sql 测试数据同源）----
// 工号6位：100001 管理员；100002 张主任(主任医师/中医内科)；100003 李医生(主治医师/中医内科)；100004 王医生(副主任医师/针灸推拿科)
struct DemoAccount { const char* no; const char* name; int role; const char* dept; const char* title; };
inline const DemoAccount DEMO_ACCOUNTS[] = {
    {"100001", "系统管理员", 2, "", ""},
    {"100002", "张主任", 1, "中医内科", "主任医师"},
    {"100003", "李医生", 1, "中医内科", "主治医师"},
    {"100004", "王医生", 1, "针灸推拿科", "副主任医师"},
};
inline const int DEMO_ACCOUNT_COUNT = 4;

// ---- 班次（手册 3.2.4/schedule.shift_type）----
inline QString shiftName(int s)
{
    return s == 1 ? QStringLiteral("上午 08:00-12:00")
         : s == 2 ? QStringLiteral("下午 14:00-17:00")
                  : QStringLiteral("晚班 19:00-21:00");
}

} // namespace def
#endif // APPDEFS_H
