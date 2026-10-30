#ifndef APPDEFS_H
#define APPDEFS_H
// 全局统一定义：配色、演示常量、时序参数、通用小函数。
// 用 constexpr 常量与 inline 函数代替传统宏：同样集中一处修改，但有类型检查、无展开副作用。
#include <QString>
#include <QRandomGenerator>

namespace def {

// ---- 配色（与 style.qss 的 4 色制一致，供代码内联样式使用）----
inline const char* const CLR_MAIN  = "#2e5e50"; // 墨绿（主色）
inline const char* const CLR_PRESS = "#234b40"; // 墨绿按压/健康
inline const char* const CLR_WARN  = "#b23b2e"; // 朱砂（警示/重症）
inline const char* const CLR_TEXT  = "#262620"; // 墨黑（正文）

// ---- 演示数据常量（手册演示脚本）----
inline const QString DEMO_SECRET  = QStringLiteral("123456");          // 演示密码/验证码
inline const QString DEMO_DATE    = QStringLiteral("10-30");           // 自动预约日期
inline const QString DEMO_WEEK    = QStringLiteral("周五 · 今天");      // 自动预约星期
inline const QString DEMO_DOC     = QStringLiteral("脾胃科 · 赵国安");  // 自动预约科室·医生
inline const QString DEMO_SLOT    = QStringLiteral("晚班 19:00-21:00"); // 自动预约时段

// ---- 字体 ----
// 视觉尺寸补偿基线：换字体时若字形偏小/偏大可整体调整（1.0 = 不补偿）
constexpr qreal FONT_COMP = 1.0;

// ---- 布局基准（800×640 设计稿的代码侧固定尺寸；随 UiScale 因子等比放大）----
constexpr int TOPBAR_H  = 64;    // 顶部导航条高
constexpr int PANEL_W   = 170;   // 左侧信息面板宽
constexpr int TBL_ROW_H = 52;    // 记录页表格行高
constexpr int MODAL_W   = 560;   // 弹窗宽

// ---- 时序参数 ----
constexpr int AI_REPLY_MS      = 500;   // 建档 AI 回复延迟
constexpr int DETECT_ANIM_MS   = 1800;  // 舌苔检测演示时长（正式版：串口超时 10s）
constexpr int PROFILE_BACK_MS  = 1200;  // 建档保存成功后回首页延迟
constexpr int TOAST_VISIBLE_MS = 3200;  // toast 停留时长
constexpr int TOAST_FADE_MS    = 500;   // toast 淡出时长
constexpr int SMS_RESEND_S     = 60;    // 验证码重发倒计时（秒）
constexpr int PWD_MAX_RETRY    = 3;     // 医保密码最大错误次数

// ---- 通用小函数 ----
// 挂号单号：GH202610XXX（XXX=100~999 随机）
inline QString ghNo()
{
    return QStringLiteral("GH202610") +
           QString::number(QRandomGenerator::global()->bounded(100, 1000));
}

// "晚班 19:00-21:00" -> "19:00-21:00"（剥掉时段前缀词，同类展示统一）
inline QString slotTime(const QString& slot)
{
    const int sp = slot.indexOf(QLatin1Char(' '));
    return sp >= 0 ? slot.mid(sp + 1) : slot;
}

} // namespace def
#endif // APPDEFS_H
