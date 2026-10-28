#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <iostream>
// 凌智视觉显示模块头文件（屏幕输出、画面渲染）
#include <lockzhiner_vision_module/edit/edit.h>
// OpenCV基础图像处理头文件
#include <opencv2/core/core.hpp>
// OpenCV窗口、视频、相机采集头文件
#include <opencv2/highgui/highgui.hpp>
// OpenCV图像缩放、变换等处理函数
#include <opencv2/imgproc/imgproc.hpp>
// 凌智日志打印工具
#include <lockzhiner_vision_module/core/logger.h>
// C++高精度计时库，用于计算推理耗时、FPS
#include <chrono>
// YOLOv8 RKNN模型推理、初始化、释放接口头文件
#include "yolov8.h"
// 凌智硬件串口外设驱动（USART1）
#include <lockzhiner_vision_module/periphery/usart/usart.h>


// ===================== 宏定义区 =====================
// 屏幕输出画面分辨率
#define DISP_WIDTH 640
#define DISP_HEIGHT 480

// 【NEW】串口通信协议宏定义
#define FRAME_HEAD_DOWN  0x5A    // PC下行帧头
#define FRAME_HEAD_UP    0x5B    // RV1106上行帧头
#define FRAME_TAIL       0xA5    // 上下行共用帧尾
#define DOWN_FRAME_LEN   4       // 下行控制帧固定4字节
#define UP_FRAME_LEN     6       // 上行响应帧固定6字节
#define CONF_MAX_HEX     0x64    // 置信度最大值对应100%

// ===================== 全局变量区 =====================
// 屏幕显示宽高，使用宏赋值
int width = DISP_WIDTH;
int height = DISP_HEIGHT;

// RKNN模型输入尺寸（YOLOv8默认640*640）
int model_width = 640;
int model_height = 640;

// letterbox填充左右、上下留白像素（坐标映射用）
int leftPadding;
int topPadding;

// 目标检测分类总数，外部定义（从命令行传入）
extern int obj_class_num;
// 标签文本文件路径（外部使用）
char *lable;

// ===================== 运行模式枚举 =====================
/**
 * MODE_STOP_VIDEO：停止读取摄像头，画面冻结，仅监听串口指令
 * MODE_PLAY_VIDEO：只播放摄像头画面，不执行AI推理
 * MODE_SINGLE_INFER：单帧拍照推理，识别到物体后自动停止画面
 * MODE_CONTINUE_INFER：持续实时推理，边预览边检测目标
 */
enum RunMode
{
    MODE_STOP_VIDEO,
    MODE_PLAY_VIDEO,
    MODE_SINGLE_INFER,
    MODE_CONTINUE_INFER
};
// 全局运行模式，默认上电只播放视频不推理
RunMode g_run_mode = MODE_PLAY_VIDEO;

// ===================== 串口通信全局缓冲区 =====================
// 【MODIFY】原固定下标接收改为状态机解析，解决乱码半包问题
uint8_t recv_buf[DOWN_FRAME_LEN];
// 接收状态机枚举
enum RecvState {
    WAIT_HEAD,  // 等待下行帧头0x5A
    RECV_CMD_H, // 接收指令高8位
    RECV_CMD_L, // 接收指令低8位
    RECV_TAIL   // 接收帧尾0xA5
};
RecvState g_recv_state = WAIT_HEAD;
uint16_t g_recv_cmd = 0; // 缓存当前解析到的完整16位指令

// 【NEW】舌苔分类编码映射函数，推理类别ID转协议规定HEX编码
uint8_t GetTongueClassCode(int cls_id)
{
    switch(cls_id)
    {
        case 0: return 0x00; // 灰黑苔
        case 1: return 0x01; // 镜面舌
        case 2: return 0x02; // 薄白苔
        case 3: return 0x03; // 白腻苔
        case 4: return 0x04; // 黄腻苔
        default: return 0xFF;// 未识别/异常
    }
}

/**
 * @brief 【NEW】向上位机发送6字节上行响应帧
 * @param cmd 下行收到的原始16位指令码（原样回显）
 * @param cls_code 舌苔分类编码 0x00~0x04 / 0xFF
 * @param conf 模型原始置信度 0.0f ~ 1.0f
 * 上行帧结构：[0x5B][CMD_H][CMD_L][分类编码][置信度(0x00~0x64)][0xA5]
 */
void SendUpResponseFrame(lockzhiner_vision_module::periphery::USART1& usart, uint16_t cmd, uint8_t cls_code, float conf)
{
    uint8_t send_buf[UP_FRAME_LEN] = {0};
    // 帧头
    send_buf[0] = FRAME_HEAD_UP;
    // 指令高低字节回显（大端）
    send_buf[1] = (cmd >> 8) & 0xFF;
    send_buf[2] = cmd & 0xFF;
    // 舌苔分类编码
    send_buf[3] = cls_code;
    // 置信度换算：0.0~1.0 → 0x00 ~ 0x64
    uint8_t conf_hex = static_cast<uint8_t>(conf * CONF_MAX_HEX);
    send_buf[4] = conf_hex;
    // 帧尾
    send_buf[5] = FRAME_TAIL;

    // 二进制帧发送
    // 强制将uint8_t* 转为 const char*，兼容SDK接口
    usart.Write(reinterpret_cast<const char*>(send_buf), UP_FRAME_LEN);
    std::cout << "[UP FRAME] Send hex: ";
    for(int i=0; i<UP_FRAME_LEN; i++) printf("%02X ", send_buf[i]);
    std::cout << std::endl;
}

/**
 * @brief 串口单字节解析函数，【MODIFY】重构为状态机协议解析，兼容乱字节、半包
 * @param ch 串口读到的单个字节
 * @note 下行协议：[0x5A][CMD_H][CMD_L][0xA5]
 *       CMD为16bit大端模式，高字节在前，低字节在后
 */
void ParseByte(lockzhiner_vision_module::periphery::USART1& usart, uint8_t ch)
{
    switch(g_recv_state)
    {
        case WAIT_HEAD:
            if(ch == FRAME_HEAD_DOWN)
            {
                // 收到合法帧头，切换到接收指令高字节
                g_recv_state = RECV_CMD_H;
            }
            break;
        case RECV_CMD_H:
            g_recv_cmd = (static_cast<uint16_t>(ch) << 8); // 暂存高8位
            g_recv_state = RECV_CMD_L;
            break;
        case RECV_CMD_L:
            g_recv_cmd |= ch; // 拼接低8位，得到完整16位CMD
            g_recv_state = RECV_TAIL;
            break;
        case RECV_TAIL:
            if(ch == FRAME_TAIL)
            {
                // 完整合法下行帧接收完成，处理指令
                uint16_t cmd = g_recv_cmd;
                std::cout << "[DOWN FRAME] Recv CMD: 0x" << std::hex << cmd << std::dec << std::endl;
                switch (cmd)
                {
                case 0x0000:
                    g_run_mode = MODE_STOP_VIDEO;
                    std::cout << "CMD: 0x0000 停止视频" << std::endl;
                    break;
                case 0x0001:
                    g_run_mode = MODE_PLAY_VIDEO;
                    std::cout << "CMD: 0x0001 仅播放视频" << std::endl;
                    break;
                case 0x0010:
                    g_run_mode = MODE_SINGLE_INFER;
                    std::cout << "CMD: 0x0010 进入单帧推理模式" << std::endl;
                    break;
                case 0x0100:
                    g_run_mode = MODE_CONTINUE_INFER;
                    std::cout << "CMD: 0x0100 开启实时连续推理" << std::endl;
                    break;
                default:
                    // 未定义指令，忽略
                    std::cout << "Unknown CMD: 0x" << std::hex << cmd << std::dec << std::endl;
                    break;
                }
            }
            // 无论帧尾是否合法，重置状态机等待下一帧头
            g_recv_state = WAIT_HEAD;
            g_recv_cmd = 0;
            break;
        default:
            g_recv_state = WAIT_HEAD;
            break;
    }
}

/**
 * @brief 程序入口主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * 启动传参格式：./app ./model.rknn 类别总数 ./labels.txt
 * argv[1] = rknn模型路径
 * argv[2] = 目标分类数量
 * argv[3] = 标签文本文件路径
 */
int main(int argc, char *argv[])
{
    // 校验启动参数数量，必须传入3个参数 + 程序名，共4个
    if (argc != 4)
    {
        // 日志打印使用方法
        LOGGER_INFO("Usage: %s ./model_path class_count ./label_txt", argv[0]);
        return -1;
    }

    // 读取命令行传入的分类数量
    obj_class_num = atoi(argv[2]);
    // 标签文件路径赋值给全局指针
    lable = argv[3];

    // 校验分类数量合法性，不能小于等于0
    if (obj_class_num <= 0)
    {
        LOGGER_ERROR("Invalid class count: %d", obj_class_num);
        return -1;
    }

    // ===================== 初始化串口USART1 =====================
    lockzhiner_vision_module::periphery::USART1 usart;
    // 波特率115200打开串口，失败直接退出
    if (!usart.Open(115200))
    {
        std::cout << "Failed to open usart." << std::endl;
        return 1;
    }
    std::cout << "Serial opened, waiting down frame head 0x5A......" << std::endl;

    // ===================== RKNN模型推理上下文、检测结果容器 =====================
    rknn_app_context_t rknn_app_ctx; // RKNN推理句柄，存放输入输出、模型参数
    object_detect_result_list od_results; // 存放一帧推理后所有目标框、类别、置信度
    // 清空推理上下文内存，防止脏数据
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    // 读取模型文件路径
    const char *model_path = argv[1];

    // 初始化加载RKNN YOLOv8模型，加载失败退出程序
    if (init_yolov8_model(model_path, &rknn_app_ctx) != 0)
    {
        printf("❌ Failed to load RKNN model!\n");
        return -1;
    }
    printf("✅ RKNN model loaded successfully.\n");

    // ===================== 校验模型输出分支、分类数量 =====================
    // YOLOv8有3个检测输出分支，每个分支包含score/box两组输出
    // output_per_branch：单个分支的输出tensor数量
    int output_per_branch = rknn_app_ctx.io_num.n_output / 3;
    int model_class_num = 0;

    // 每个分支至少包含坐标、置信度两层输出才合法
    if (output_per_branch >= 2)
    {
        // 取第一个分支置信度层通道数 = 模型内置分类数
        model_class_num = rknn_app_ctx.output_attrs[1].dims[3];

        // 遍历3个检测分支，校验所有分支分类数量统一
        for (int i = 1; i < 3; i++)
        {
            // 计算当前分支置信度tensor下标
            int branch_score_idx = i * output_per_branch + 1;
            // 获取该分支分类通道数
            int branch_classes = rknn_app_ctx.output_attrs[branch_score_idx].dims[3];
            // 多分支类别数不一致打印警告日志
            if (branch_classes != model_class_num)
            {
                LOGGER_INFO("Branch %d class count mismatch", i);
            }
        }

        // 如果模型内置分类有效，覆盖命令行传入的分类数，以模型为准
        if (model_class_num > 0 && obj_class_num != model_class_num)
        {
            obj_class_num = model_class_num;
        }
    }
    else
    {
        // 分支输出数量非法，释放模型资源后退出
        LOGGER_ERROR("Invalid output_per_branch");
        release_yolov8_model(&rknn_app_ctx);
        return -1;
    }

    // 初始化后处理NMS、坐标还原、绘制接口
    if (init_post_process() != 0)
    {
        LOGGER_ERROR("init_post_process failed");
        release_yolov8_model(&rknn_app_ctx);
        return -1;
    }

    // ===================== 初始化凌智屏幕渲染模块 =====================
    lockzhiner_vision_module::edit::Edit edit;
    // 启动屏幕服务、创建渲染通道，失败退出
    if (!edit.StartAndAcceptConnection())
    {
        std::cerr << "Failed to start edit" << std::endl;
        return EXIT_FAILURE;
    }

    // ===================== OpenCV摄像头采集初始化 =====================
    cv::VideoCapture cap;
    // 设置摄像头采集分辨率640*480
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    // 打开板载摄像头设备0
    cap.open(0);
    // 摄像头打开失败直接退出
    if (!cap.isOpened())
    {
        std::cerr << "Can not open camera" << std::endl;
        return 1;
    }

    cv::Mat frame; // 存储单帧摄像头原图
    // 高精度时间戳，用于计算单帧耗时、FPS
    std::chrono::steady_clock::time_point start, end;

    // ===================== 主循环：持续采集画面+串口指令处理 =====================
    while (true)
    {
        // ---------------------- 1. 串口异步接收指令【MODIFY】传usart对象给解析函数 ----------------------
        std::string str_buf;
        // 阻塞读取1字节串口数据存入字符串缓存
        usart.Read(str_buf, 1);
        // 遍历读到的所有字节，送入协议解析函数
        for (auto ch : str_buf)
        {
            ParseByte(usart, static_cast<uint8_t>(ch));
        }

        // ---------------------- 2. 停止模式判断 ----------------------
        // 模式：停止视频，跳过摄像头读取、推理、画面刷新
        if (g_run_mode == MODE_STOP_VIDEO)
        {
            continue;
        }

        // 记录当前帧开始时间，用于计算推理耗时
        start = std::chrono::steady_clock::now();
        // 读取一帧摄像头图像
        cap >> frame;
        // 读取图像为空，跳过本轮循环
        if (frame.empty())
        {
            LOGGER_INFO("Read frame empty");
            continue;
        }
        // 缩放原图到屏幕输出尺寸640*480
        cv::resize(frame, frame, cv::Size(width, height));

        // 推理使能标记
        bool need_infer = false;

        // 连续推理 / 单帧推理 两种模式需要执行AI检测
        if (g_run_mode == MODE_CONTINUE_INFER || g_run_mode == MODE_SINGLE_INFER)
        {
            need_infer = true;
        }

        // ---------------------- 3. 执行RKNN YOLOv8推理 ----------------------
        if (need_infer)
        {
            // 对原图做letterbox等比例缩放填充，适配模型640输入尺寸
            cv::Mat letterboxImage = letterbox(frame);
            // 将处理好的图像数据拷贝到RKNN模型输入内存
            memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width * model_height * 3);

            // 执行模型推理，推理成功进入后处理
            if (inference_yolov8_model(&rknn_app_ctx, &od_results) == 0)
            {
                // 绘制检测框、类别、置信度到原图
                // mapCoordinates：将模型letterbox坐标映射回原图坐标
                draw_detections(od_results.count, od_results.results, frame, mapCoordinates);

                // ---------------------- 单帧推理模式业务逻辑【MODIFY】删除文本发送，改用协议二进制上行帧 ----------------------
                if (g_run_mode == MODE_SINGLE_INFER)
                {
                    // 当前帧检测到目标物体
                    if (od_results.count > 0)
                    {
                        // 取第一个目标（需求单帧拍照返回一次结果）
                        object_detect_result *det = &od_results.results[0];
                        // 转换舌苔协议编码
                        uint8_t tongue_code = GetTongueClassCode(det->cls_id);
                        // 发送标准6字节上行响应帧，带回本次触发的原始指令g_recv_cmd
                        SendUpResponseFrame(usart, g_recv_cmd, tongue_code, det->prop);

                        // 识别成功，自动切换到停止模式，冻结画面
                        g_run_mode = MODE_STOP_VIDEO;
                        std::cout << "Single shot detect success, stop camera." << std::endl;
                    }
                    // else
                    // {
                    //     // 无目标，发送异常编码0xFF上行帧（可选，根据业务需求）
                    //     SendUpResponseFrame(usart, g_recv_cmd, 0xFF, 0.0f);
                    //     std::cout << "Single shot no target, send unrecognize frame, continue..." << std::endl;
                    // }
                }

                // ---------------------- 连续实时推理打印FPS ----------------------
                if (g_run_mode == MODE_CONTINUE_INFER)
                {
                    // 获取帧结束时间
                    end = std::chrono::steady_clock::now();
                    // 计算本帧总耗时（秒）
                    double t = std::chrono::duration<double>(end - start).count();
                    // 打印单帧耗时、当前画面目标数量
                    printf("FPS frame %.4fs, obj:%d\n", t, od_results.count);
                }
            }
            // else
            // {
            //     // 推理失败，发送异常上行帧
            //     if(g_run_mode == MODE_SINGLE_INFER)
            //     {
            //         SendUpResponseFrame(usart, g_recv_cmd, 0xFF, 0.0f);
            //     }
            // }
        }

        // ---------------------- 4. 屏幕刷新输出画面 ----------------------
        // 仅播放视频 / 实时推理 两种模式持续刷新屏幕画面
        edit.Print(frame);
    }

    // ===================== 程序正常退出资源释放（死循环理论不会走到这里） =====================
    release_yolov8_model(&rknn_app_ctx);  // 释放RKNN模型内存
    deinit_post_process();               // 释放后处理资源
    cap.release();                       // 关闭摄像头
    usart.Close();                       // 【NEW】关闭串口资源
    return 0;
}