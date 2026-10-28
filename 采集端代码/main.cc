#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <lockzhiner_vision_module/edit/edit.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <lockzhiner_vision_module/core/logger.h>
#include <chrono>
#include "yolov8.h"
#include "tongue_def.h" // ===== 阶段一新增：舌苔业务定义 =====

// output img size
#define DISP_WIDTH 640
#define DISP_HEIGHT 480

// disp size
int width = DISP_WIDTH;
int height = DISP_HEIGHT;

// model size
int model_width = 640;
int model_height = 640;

int leftPadding;
int topPadding;

// label size
extern int obj_class_num;
char *lable;

// ============================================================================
// ===== 阶段一新增：舌苔结果解析与判定 =====
// ============================================================================

// T1.2：从所有检测框中取置信度最高的一个，并完成 T1.3 有效性判定 / T1.4 健康判定
// 返回 true = 得到有效结论；false = 无效（置信度不足或无目标）
static bool judge_tongue_result(const object_detect_result_list &od_results,
                                int class_num, TongueResult &out)
{
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

	// T1.3：有效性判定（置信度 >= 80% 才有效）
	if (conf < TONGUE_CONF_THRESHOLD)
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
			printf("[舌苔检测] 类别=%s 置信度=%.1f%% 结论=无效(置信度不足%d%%)\n",
			       TONGUE_NAMES[r.cls_id], r.confidence, (int)TONGUE_CONF_THRESHOLD);
		}
		else
		{
			printf("[舌苔检测] 检测失败（未捕获到舌苔）\n");
		}
		return;
	}
	printf("[舌苔检测] 类别=%s 置信度=%.1f%% 健康=%s 重症=%s 结论=有效\n",
	       TONGUE_NAMES[r.cls_id], r.confidence,
	       r.health == 0 ? "是" : "否",
	       r.severe == 1 ? "是" : "否");
}
// ============================ 阶段一新增结束 ================================

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		LOGGER_INFO("Usage: %s ./model_path class_count ./label_txt", argv[0]);
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
	       TONGUE_CONF_THRESHOLD);
	// ================================================

	// 加载标签文件（在确定最终类别数之后）
	if (init_post_process() != 0)
	{
		LOGGER_ERROR("init_post_process failed");
		release_yolov8_model(&rknn_app_ctx);
		return -1;
	}

	// 打开摄像头
	lockzhiner_vision_module::edit::Edit edit;
	if (!edit.StartAndAcceptConnection())
	{
		std::cerr << "Error: Failed to start and accept connection." << std::endl;
		return EXIT_FAILURE;
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
			continue;
		}
		cv::resize(frame, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
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

		draw_detections(od_results.count,
		                od_results.results,
		                frame,
		                mapCoordinates);

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
		fflush(stdout);
		// ============================ 改造结束 ==============================
	}
	release_yolov8_model(&rknn_app_ctx);
	deinit_post_process();
	cap.release();
	return 0;
}
