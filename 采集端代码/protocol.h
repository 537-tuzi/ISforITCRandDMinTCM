#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "tongue_def.h"   // 用里面的 TongueResult

// ================== 串口协议常量（课程协议） ==================
#define FRAME_HEAD_DOWN 0x5A   // 下行帧头（PC→板）
#define FRAME_HEAD_UP   0x5B   // 上行帧头（板→PC）
#define FRAME_TAIL      0xA5   // 帧尾（上下行共用）

// 下行指令码
enum DownCmd {
    CMD_STOP_VIDEO     = 0x0000,  // 停止视频输出
    CMD_PLAY_VIDEO     = 0x0001,  // 仅播放，不推理
    CMD_SINGLE_INFER   = 0x0010,  // 单帧抓拍推理
    CMD_CONTINUE_INFER = 0x0100,  // 连续采集推理
};

// 解析下行4字节帧：合法返回指令码，非法返回 -1
static int ParseDownFrame(const unsigned char buf[4])
{
    if (buf[0] == FRAME_HEAD_DOWN && buf[3] == FRAME_TAIL)
        return ((int)buf[1] << 8) | (int)buf[2];
    return -1;
}

// 组上行6字节帧：0x5B + CMD回显 + 类别码 + 置信度(0~100) + 0xA5
// 无效结果（valid=0 或类别越界）→ 类别码填 0xFF（协议的"未识别/异常"）
static void BuildUpFrame(unsigned char cmd_h, unsigned char cmd_l,
                         const TongueResult &r, unsigned char out[6])
{
    out[0] = FRAME_HEAD_UP;
    out[1] = cmd_h;
    out[2] = cmd_l;
    out[3] = (r.valid && r.cls_id >= 0 && r.cls_id <= 4)
                 ? (unsigned char)r.cls_id : 0xFF;
    out[4] = (unsigned char)(r.confidence > 100.0f ? 100 :
             (r.confidence < 0.0f ? 0 : (int)(r.confidence + 0.5f)));
    out[5] = FRAME_TAIL;
}

#endif
