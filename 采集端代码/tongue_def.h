#ifndef TONGUE_DEF_H
#define TONGUE_DEF_H

// ============================================================================
// 舌苔业务定义：类别映射、判定规则、检测结果结构
// 类别顺序必须与训练 data.yaml 一致：
//   0:灰黑苔  1:镜面舌  2:薄白苔  3:白腻苔  4:黄腻苔
// ============================================================================

#include <atomic>

enum TongueClass
{
    TONGUE_HUI_HEI   = 0, // 灰黑苔
    TONGUE_JING_MIAN = 1, // 镜面舌
    TONGUE_BO_BAI    = 2, // 薄白苔
    TONGUE_BAI_NI    = 3, // 白腻苔
    TONGUE_HUANG_NI  = 4  // 黄腻苔
};

static const char *TONGUE_NAMES[5] = {
    "灰黑苔", "镜面舌", "薄白苔", "白腻苔", "黄腻苔"};

// 有效置信度阈值（百分制，项目手册默认 >=80% 判定为有效结果）。
// D25：运行时可调——串口扩展指令 0x40XX 设定阈值 XX%（定义在 main.cc）。
// atomic：串口接收线程写 / 主循环读，跨线程访问消除数据竞争 UB
extern std::atomic<float> g_conf_threshold;

// 健康状态：0-健康 1-病体（薄白苔=健康，其余=病体）
inline int tongue_health(int cls)
{
    return (cls == TONGUE_BO_BAI) ? 0 : 1;
}

// 重症标记：灰黑苔/镜面舌=1（触发自动预约），其余=0
inline int tongue_severe(int cls)
{
    return (cls == TONGUE_HUI_HEI || cls == TONGUE_JING_MIAN) ? 1 : 0;
}

// 设备编号默认值（D1 终裁：结构体字段只多不少；串口 6 字节帧不携带，
// 供板端日志/存图命名/后续 TCP 上报使用，入库时由上位机会话绑定）
#define TONGUE_DEVICE_ID "RV1106-001"

// 单次舌苔检测结论（阶段三串口协议直接复用此结构）
typedef struct
{
    int   cls_id;          // 类别id 0~4；-1=检测失败
    float confidence;      // 置信度 0~100
    int   health;          // 0-健康 1-病体；失败时 -1
    int   severe;          // 0-否 1-是；失败时 -1
    int   valid;           // 1-有效 0-无效
    char  timestamp[24];   // 检测时间戳（板端本地时间 YYYY-MM-DD HH:MM:SS）
    char  device_id[24];   // 设备编号（TONGUE_DEVICE_ID）
} TongueResult;

#endif // TONGUE_DEF_H
