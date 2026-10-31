#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <lockzhiner_vision_module/edit/edit.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <lockzhiner_vision_module/core/logger.h>
#include <lockzhiner_vision_module/periphery/usart/usart.h>
#include <chrono>
#include <ctime>
#include <atomic>
#include <thread>
#include <unistd.h>
#include "yolov8.h"
#include "tongue_def.h" // ===== 阶段一新增：舌苔业务定义 =====
#include "protocol.h"
// output img size
#define DISP_WIDTH 640
#define DISP_HEIGHT 480

// disp size
int width = DISP_WIDTH;
int height = DISP_HEIGHT;

// model size
int model_width = 640;
int model_height = 640;

// letterbox 边距（注意：SDK 侧 yolov8.cc 的 letterbox() extern 引用本变量，不可删除）
int leftPadding;
int topPadding;

// label size
extern int obj_class_num;
char *lable;

// ============================================================================
// ===== 阶段B新增：串口收发与运行模式（D20-D25） =====
// ============================================================================
using lockzhiner_vision_module::periphery::USART1;
static USART1 g_usart;                 // 串口对象（SDK 固定 /dev/ttyS1，D24 已确认）
std::atomic<int> g_mode{-1};           // D21：-1=从未收到指令→原连续检测逻辑；0停止/1仅播放/2单帧/3连续
std::atomic<float> g_conf_threshold{80.0f};  // 有效置信度阈值（D25 可调；rx 线程写/主循环读，atomic 消竞争）

// 串口写 6 字节帧（线程安全：fd 全双工，与接收线程的 read 并发安全）
static void send_frame(const unsigned char f[6])
{
    std::string s(reinterpret_cast<const char *>(f), 6);
    g_usart.Write(s);
}

// D20/D22：独立线程收下行帧（usleep 20ms 轮询非阻塞读），解析后切模式/应答
static void UsartRxThread()
{
    unsigned char half[4];
    int have = 0;
    while (true)
    {
        unsigned char buf[64];
        int len = g_usart.Read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (len <= 0)
        {
            usleep(20000);   // D22：无数据歇 20ms
            continue;
        }
        for (int i = 0; i < len; i++)
        {
            half[have++] = buf[i];
            if (have < 4)
                continue;
            const int cmd = ParseDownFrame(half);
            if (cmd < 0)
            {
                // 帧失步恢复：丢 1 字节左滑，保留后 3 字节继续找帧头 0x5A，
                // 避免丢 1 字节后永久错位（非法组也打日志）
                LOGGER_INFO("[串口] 非法帧 %02X %02X %02X %02X（滑窗重同步）",
                            half[0], half[1], half[2], half[3]);
                half[0] = half[1];
                half[1] = half[2];
                half[2] = half[3];
                have = 3;
                continue;
            }
            have = 0;
            unsigned char ack[6];
            switch (cmd)
            {
            case CMD_STOP_VIDEO:
            case CMD_PLAY_VIDEO:
            case CMD_SINGLE_INFER:
            case CMD_CONTINUE_INFER:
                g_mode = (cmd == CMD_STOP_VIDEO) ? 0
                       : (cmd == CMD_PLAY_VIDEO) ? 1
                       : (cmd == CMD_SINGLE_INFER) ? 2 : 3;
                printf("[串口] 指令 0x%04X → 模式%d\n", cmd, g_mode.load());
                break;
            case CMD_QUERY_STATUS:   // D25：状态查询应答
                BuildAckFrame(0x00, 0x20,
                              g_mode.load() < 0 ? 0xFF : (unsigned char)g_mode.load(), 0,
                              ack);
                send_frame(ack);
                printf("[串口] 状态查询应答：mode=%d\n", g_mode.load());
                break;
            case CMD_QUERY_THRES:    // D25：阈值查询应答
                BuildAckFrame(0x00, 0x30, 0x01, (unsigned char)(g_conf_threshold.load() + 0.5f), ack);
                send_frame(ack);
                printf("[串口] 阈值查询应答：%.0f%%\n", g_conf_threshold.load());
                break;
            default:
                if (IsSetThresholdCmd(cmd))   // D25：0x40XX 设定阈值并回显
                {
                    const int v = cmd & 0xFF;
                    if (v >= 1 && v <= 100)
                    {
                        g_conf_threshold = (float)v;
                        BuildAckFrame(0x40, (unsigned char)v, 0x01, (unsigned char)v, ack);
                        send_frame(ack);
                        printf("[串口] 阈值设定为 %d%%（0x40%02X 应答已发）\n", v, v);
                    }
                    else
                    {
                        LOGGER_INFO("[串口] 阈值越界 %d（1~100），忽略", v);
                    }
                }
                else
                {
                    LOGGER_INFO("[串口] 未知指令 0x%04X（忽略）", cmd);
                }
                break;
            }
            fflush(stdout);
        }
    }
}
// ============================ 阶段B结束 ================================

// ============================================================================
// ===== 阶段一新增：舌苔结果解析与判定 =====
// ============================================================================

// T1.2：从所有检测框中取置信度最高的一个，并完成 T1.3 有效性判定 / T1.4 健康判定
// 返回 true = 得到有效结论；false = 无效（置信度不足或无目标）
static bool judge_tongue_result(const object_detect_result_list &od_results,
                                int class_num, TongueResult &out)
{
	// D1 终裁：结构体带时间戳/设备编号（所有路径统一填充）
	memset(&out, 0, sizeof(out));
	strncpy(out.device_id, TONGUE_DEVICE_ID, sizeof(out.device_id) - 1);
	{
		time_t now = time(nullptr);
		struct tm tmv;
		localtime_r(&now, &tmv);
		strftime(out.timestamp, sizeof(out.timestamp), "%Y-%m-%d %H:%M:%S", &tmv);
	}

	// 无任何检测框
	if (od_results.count <= 0)
	{
		out.valid = 0;
		out.cls_id = -1;
		out.confidence = 0;
		out.health = -1;
		out.severe = -1;
		return false;
	}

	// 遍历所有框，只保留置信度最高的一个
	int best = -1;
	float best_prop = 0.0f;
	for (int i = 0; i < od_results.count; i++)
	{
		const object_detect_result *det = &od_results.results[i];
		if (det->prop > best_prop)
		{
			best_prop = det->prop;
			best = i;
		}
	}

	const object_detect_result *det = &od_results.results[best];
	float conf = det->prop * 100.0f;

	// 防御：类别数不是5或id越界，映射无意义，判无效
	if (class_num != 5 || det->cls_id < 0 || det->cls_id >= 5)
	{
		out.valid = 0;
		out.cls_id = -1;
		out.confidence = conf;
		out.health = -1;
		out.severe = -1;
		return false;
	}

	// T1.3：有效性判定（置信度 >= 阈值才有效；阈值 0x40XX 可调，D25）
	if (conf < g_conf_threshold)
	{
		out.valid = 0;
		out.cls_id = det->cls_id;
		out.confidence = conf;
		out.health = -1;
		out.severe = -1;
		return false;
	}

	// T1.4：健康/重症判定
	out.valid = 1;
	out.cls_id = det->cls_id;
	out.confidence = conf;
	out.health = tongue_health(det->cls_id);
	out.severe = tongue_severe(det->cls_id);
	return true;
}

// 打印结构化结论（每次检测一行）
static void print_tongue_result(const TongueResult &r)
{
	if (!r.valid)
	{
		if (r.cls_id >= 0)
		{
			// 有目标但置信度不足
			printf("[舌苔检测] 类别=%s 置信度=%.1f%% 结论=无效(置信度不足%.0f%%)\n",
			       TONGUE_NAMES[r.cls_id], r.confidence, g_conf_threshold.load());
		}
		else
		{
			printf("[舌苔检测] 检测失败（未捕获到舌苔）\n");
		}
		return;
	}
	printf("[舌苔检测] 类别=%s 置信度=%.1f%% 健康=%s 重症=%s 结论=有效 时间=%s 设备=%s\n",
	       TONGUE_NAMES[r.cls_id], r.confidence,
	       r.health == 0 ? "是" : "否",
	       r.severe == 1 ? "是" : "否",
	       r.timestamp, r.device_id);
}
// ============================ 阶段一新增结束 ================================

int main(int argc, char *argv[])
{
	// 第 5 参可传 "headless"：跳过显示服务（Edit 的 accept 会阻塞等待
	// “凌智助手”客户端连接 6868 端口，无人连接时程序挂起——联调/自检场景
	// 用 headless 直接进检测循环，只走串口输出）
	bool headless = false;
	if (argc == 5 && strcmp(argv[4], "headless") == 0)
		headless = true;
	if (argc != 4 && !headless)
	{
		LOGGER_INFO("Usage: %s ./model_path class_count ./label_txt [headless]", argv[0]);
		return -1;
	}
	obj_class_num = atoi(argv[2]);
	lable = argv[3];
	if (obj_class_num <= 0)
	{
		LOGGER_ERROR("Invalid class count: %d", obj_class_num);
		return -1;
	}

	// rknn上下文结构体
	rknn_app_context_t rknn_app_ctx;
	object_detect_result_list od_results;
	int ret;
	const char *model_path = argv[1];
	memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

	// Step 1: Load RKNN model
	if (init_yolov8_model(model_path, &rknn_app_ctx) != 0)
	{
		printf("❌ Failed to load RKNN model!\n");
		return -1;
	}
	printf("✅ RKNN model loaded successfully.\n");

	// 从模型输出中自动检测实际类别数
	// YOLOv8 输出布局: 每个分支3个tensor [box, score, score_sum]
	// score tensor 在索引 1, 4, 7 位置，其 dims[3] 为类别数
	int output_per_branch = rknn_app_ctx.io_num.n_output / 3;
	int model_class_num = 0;

	if (output_per_branch >= 2)
	{
		// score tensor 在每个分支的索引 +1 位置
		// 取第一个分支的 score tensor (index=1) 来获取类别数
		model_class_num = rknn_app_ctx.output_attrs[1].dims[3];

		// 验证三个分支的类别数是否一致
		for (int i = 1; i < 3; i++)
		{
			int branch_score_idx = i * output_per_branch + 1;
			int branch_classes = rknn_app_ctx.output_attrs[branch_score_idx].dims[3];
			if (branch_classes != model_class_num)
			{
				LOGGER_INFO("Branch %d class count %d mismatch with branch 0 count %d",
				            i, branch_classes, model_class_num);
			}
		}

		if (model_class_num > 0 && obj_class_num != model_class_num)
		{
			LOGGER_INFO("Class count mismatch, use model class count %d instead of arg %d",
			            model_class_num, obj_class_num);
			obj_class_num = model_class_num;
		}
	}
	else
	{
		LOGGER_ERROR("Invalid output_per_branch: %d", output_per_branch);
		release_yolov8_model(&rknn_app_ctx);
		return -1;
	}

	// ===== 阶段一新增：舌苔业务要求类别数必须为5 =====
	if (obj_class_num != 5)
	{
		LOGGER_ERROR("[舌苔检测] 模型类别数为%d，不是5，舌苔业务映射不可用！", obj_class_num);
		release_yolov8_model(&rknn_app_ctx);
		return -1;
	}
	printf("✅ 舌苔检测业务启动（5类别：灰黑苔/镜面舌/薄白苔/白腻苔/黄腻苔，有效阈值%.0f%%）\n",
	       g_conf_threshold.load());
	// ================================================
	// ===== 阶段A：协议帧自测 =====
	{
		TongueResult demo;
		demo.valid = 1;
		demo.cls_id = 2;            // 薄白苔
		demo.confidence = 92.0f;    // 92% → 0x5C
		unsigned char f[6];
		BuildUpFrame(0x00, 0x10, demo, f);   // 假装回显 0x0010 指令
		printf("[协议自测] 期望: 5B 00 10 02 5C A5 | 实际: ");
		for (int i = 0; i < 6; i++)
			printf("%02X ", f[i]);
		printf("\n");
		fflush(stdout);
	}
	// 加载标签文件（在确定最终类别数之后）
	if (init_post_process() != 0)
	{
		LOGGER_ERROR("init_post_process failed");
		release_yolov8_model(&rknn_app_ctx);
		return -1;
	}

	// ===== 阶段B：打开串口并启动接收线程（D23：失败仅日志、不影响单机推理） =====
	if (g_usart.Open(115200))
	{
		std::thread(UsartRxThread).detach();   // D20：独立线程 + detach 常驻
		printf("✅ 串口已打开 /dev/ttyS1@115200（指令线程已启动）\n");
	}
	else
	{
		LOGGER_ERROR("串口打开失败（/dev/ttyS1），仅单机模式运行");
	}

	// 打开摄像头与显示服务（headless 模式跳过显示，避免 accept 阻塞）
	lockzhiner_vision_module::edit::Edit edit;
	if (!headless)
	{
		if (!edit.StartAndAcceptConnection())
		{
			std::cerr << "Error: Failed to start and accept connection." << std::endl;
			return EXIT_FAILURE;
		}
	}
	else
	{
		printf("ℹ️ headless 模式：跳过显示服务（纯检测+串口输出）\n");
	}

	cv::VideoCapture cap;
	cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
	cap.open(0);

	if (!cap.isOpened())
	{
		std::cerr << "Error: Could not open camera." << std::endl;
		return 1;
	}
	cv::Mat frame;
	std::chrono::steady_clock::time_point start, end;

	// ===== 阶段一新增：失败重试状态标记 =====
	// 上一帧结果无效时置位：下一帧相当于"重试1次"，
	// 重试仍无效才输出"检测失败"
	bool retry_pending = false;
	// ================================================

	while (true)
	{
		start = std::chrono::steady_clock::now();
		cap >> frame;
		if (frame.empty())
		{
			LOGGER_INFO("❌ Failed to read frame from camera.\n");
			usleep(100000);   // 断流防刷屏：100ms 后再取
			continue;
		}
		// 摄像头已配置 640×480：同尺寸时跳过恒等重采样（省 1~2ms/帧）
		if (frame.cols != width || frame.rows != height)
			cv::resize(frame, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);

		// ===== 阶段B：按指令模式分流（-1 未收指令=原连续检测逻辑）=====
		const int mode = g_mode.load();
		if (mode == 0)
		{
			usleep(50000);   // 停止视频输出：不推理、不显示
			continue;
		}
		if (mode == 1)
		{
			if (!headless)
				edit.Print(frame);   // 仅播放：显示画面，不推理、不发帧
			usleep(30000);
			continue;
		}

		cv::Mat letterboxImage = letterbox(frame);

		if (letterboxImage.empty() || letterboxImage.total() * letterboxImage.elemSize() != model_width * model_height * 3)
		{
			LOGGER_ERROR("❌ Input image format or size mismatch!\n");
			release_yolov8_model(&rknn_app_ctx);
			return -1;
		}

		if (rknn_app_ctx.input_mems == nullptr || rknn_app_ctx.input_mems[0] == nullptr)
		{
			LOGGER_ERROR("❌ RKNN input memory not allocated!\n");
			release_yolov8_model(&rknn_app_ctx);
			return -1;
		}

		memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width * model_height * 3);

		if (inference_yolov8_model(&rknn_app_ctx, &od_results) != 0)
		{
			LOGGER_ERROR("inference_yolov8_model failed");
			release_yolov8_model(&rknn_app_ctx);
			return -1;
		}

		// headless：无显示输出，跳过画框（省 ~0.3ms/帧）；逐框打印与判定/上行帧不受影响
		if (!headless)
		{
			draw_detections(od_results.count,
			                od_results.results,
			                frame,
			                mapCoordinates);
		}

		// D6（用户 2026-10-30 确认）：保留逐框打印
		for (int i = 0; i < od_results.count; i++)
		{
			const object_detect_result *det = &od_results.results[i];
			printf("[框%d] 类别=%s(%d) 置信度=%.1f%% 区域=(%d,%d)-(%d,%d)\n",
			       i + 1, coco_cls_to_name(det->cls_id), det->cls_id,
			       det->prop * 100.0f,
			       det->box.left, det->box.top, det->box.right, det->box.bottom);
		}

		if (!headless)
			edit.Print(frame);

		end = std::chrono::steady_clock::now();
		double elapsed_time = std::chrono::duration<double>(end - start).count();
		printf("Frame processed in %.4f seconds, detected %d objects\n", elapsed_time, od_results.count);

		// ====================================================================
		// ===== 阶段一改造：结果解析与判定（替代原来逐框打印的循环）=====
		// ====================================================================
		TongueResult tres;
		bool ok = judge_tongue_result(od_results, obj_class_num, tres);
		if (!ok && !retry_pending)
		{
			// 第一次无效：不立即判失败，下一帧重新推理（重试1次）
			retry_pending = true;
		}
		else
		{
			retry_pending = false;
			print_tongue_result(tres);
		}

		// ===== 阶段B：推理模式下组上行结果帧发送（回显触发指令码）=====
		if (mode == 2 || mode == 3)
		{
			unsigned char up[6];
			BuildUpFrame(mode == 2 ? 0x00 : 0x01, mode == 2 ? 0x10 : 0x00, tres, up);
			send_frame(up);
			printf("[串口] 上行 %02X %02X %02X %02X %02X %02X\n",
			       up[0], up[1], up[2], up[3], up[4], up[5]);
			if (mode == 2)
				g_mode = 0;   // 单帧语义：一帧（含重试）完成后回到停止态
		}
		fflush(stdout);
		// ============================ 改造结束 ==============================
	}
	release_yolov8_model(&rknn_app_ctx);
	deinit_post_process();
	cap.release();
	return 0;
}
