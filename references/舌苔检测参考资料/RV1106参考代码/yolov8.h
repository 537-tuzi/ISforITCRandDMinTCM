#ifndef _RKNN_DEMO_YOLOV8_H_
#define _RKNN_DEMO_YOLOV8_H_

#include "rknn_api.h"

typedef struct {
    char *dma_buf_virt_addr;
    int dma_buf_fd;
    int size;
} rknn_dma_buf;

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;

    rknn_tensor_mem* input_mems[1];
    rknn_tensor_mem* output_mems[9];
    rknn_dma_buf img_dma_buf;

    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;
} rknn_app_context_t;

#include "postprocess.h"


int init_yolov8_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_yolov8_model(rknn_app_context_t* app_ctx);

int inference_yolov8_model(rknn_app_context_t* app_ctx, object_detect_result_list* od_results);

#endif //_RKNN_DEMO_YOLOV8_H_

cv::Mat letterbox(cv::Mat input);
void mapCoordinates(int *x, int *y);

void draw_detections(int count,
                     object_detect_result *results,
                     cv::Mat &frame,
                     void (*mapCoord)(int *, int *));