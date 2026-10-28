#include "yolov8.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <set>
#include <vector>

int OBJ_CLASS_NUM = 1;        // 默认值（通过 init_post_process 修改）
static char** labels = nullptr; // 动态分配的标签数组
int obj_class_num = 0;


inline static int clamp(float val, int min, int max) { return val > min ? (val < max ? val : max) : min; }
extern char *lable;

static char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        if (ch == '\r') continue; // 过滤 Windows 的 \r 换行符，防止 cv::putText 卡死
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = (char *)tmp;

        buffer[i] = (char)ch;
        i++;
    }
    buffer[i] = '\0';

    *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s = NULL;
    int i = 0;
    int n = 0;

    if (file == NULL)
    {
        printf("Open %s fail!\n", fileName);
        return -1;
    }

    while ((s = readLine(file, s, &n)) != NULL)
    {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    fclose(file);
    return i;
}

static int loadLabelName(const char *locationFilename, char *label[])
{
    printf("load lable %s\n", locationFilename);
    readLines(locationFilename, label, OBJ_CLASS_NUM);
    return 0;
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
                              float ymax1)
{
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
               int filterId, float threshold)
{
    for (int i = 0; i < validCount; ++i)
    {
        int n = order[i];
        if (n == -1 || classIds[n] != filterId)
        {
            continue;
        }
        for (int j = i + 1; j < validCount; ++j)
        {
            int m = order[j];
            if (m == -1 || classIds[m] != filterId)
            {
                continue;
            }
            float xmin0 = outputLocations[n * 4 + 0];
            float ymin0 = outputLocations[n * 4 + 1];
            float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
            float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

            float xmin1 = outputLocations[m * 4 + 0];
            float ymin1 = outputLocations[m * 4 + 1];
            float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
            float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold)
            {
                order[j] = -1;
            }
        }
    }
    return 0;
}

static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
{
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right)
    {
        key_index = indices[left];
        key = input[left];
        while (low < high)
        {
            while (low < high && input[high] <= key)
            {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key)
            {
                low++;
            }
            input[high] = input[low];
            indices[high] = indices[low];
        }
        input[low] = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
}

static float sigmoid(float x) { return 1.0 / (1.0 + expf(-x)); }

static float unsigmoid(float y) { return -1.0 * logf((1.0 / y) - 1.0); }

inline static int32_t __clip(float val, float min, float max)
{
    float f = val <= min ? min : (val >= max ? max : val);
    return f;
}

static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    int8_t res = (int8_t)__clip(dst_val, -128, 127);
    return res;
}

static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }

static void compute_dfl(float* tensor, int dfl_len, float* box){
    for (int b=0; b<4; b++){
        float exp_t[dfl_len];
        float exp_sum=0;
        float acc_sum=0;
        for (int i=0; i< dfl_len; i++){
            exp_t[i] = exp(tensor[i+b*dfl_len]);
            exp_sum += exp_t[i];
        }

        for (int i=0; i< dfl_len; i++){
            acc_sum += exp_t[i]/exp_sum *i;
        }
        box[b] = acc_sum;
    }
}

// ============================================
// 修复后的 process_i8_rv1106 函数
// ============================================
// YOLOv8 输出布局（每个分支3个tensor）：
//   index+0: box tensor,   dims=[1, H, W, 64]  (4 * dfl_len)
//   index+1: score tensor,  dims=[1, H, W, C]  (C = 类别数)
//   index+2: score_sum/aux, dims=[1, H, W, 1]  (用于快速过滤)
// ============================================
static int process_i8_rv1106(int8_t *box_tensor, int32_t box_zp, float box_scale,
                             int8_t *score_tensor, int32_t score_zp, float score_scale,
                             int8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                             int grid_h, int grid_w, int stride, int dfl_len, int score_channels,
                             std::vector<float> &boxes,
                             std::vector<float> &objProbs,
                             std::vector<int> &classId,
                             float threshold) {
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    int8_t score_thres_i8 = qnt_f32_to_affine(threshold, score_zp, score_scale);
    int8_t score_sum_thres_i8 = qnt_f32_to_affine(threshold, score_sum_zp, score_sum_scale);

    for (int i = 0; i < grid_h; i++) {
        for (int j = 0; j < grid_w; j++) {
            int offset = i * grid_w + j;
            int max_class_id = -1;

            // 通过 score sum 起到快速过滤的作用
            if (score_sum_tensor != nullptr) {
                if (score_sum_tensor[offset] < score_sum_thres_i8) {
                    continue;
                }
            }

            int8_t max_score = -128;
            // score_channels 是模型实际的类别数（从 score tensor 的 dims[3] 获取）
            int class_count = OBJ_CLASS_NUM < score_channels ? OBJ_CLASS_NUM : score_channels;

            // score_tensor 布局: [1, H, W, C]
            // 对于每个 (i,j)，类别 c 的分数在 offset*C + c 位置
            int score_offset = offset * score_channels;

            for (int c = 0; c < class_count; c++) {
                int8_t score = score_tensor[score_offset + c];
                if ((score > score_thres_i8) && (score > max_score)) {
                    max_score = score;
                    max_class_id = c;
                }
            }

            // compute box
            if (max_class_id >= 0 && max_score > score_thres_i8) {
                // box_tensor 布局: [1, H, W, 64]
                // 对于每个 (i,j)，4*dfl_len 个值在 offset*4*dfl_len 位置
                offset = (i * grid_w + j) * 4 * dfl_len;
                float box[4];
                float before_dfl[dfl_len*4];
                for (int k=0; k< dfl_len*4; k++){
                    before_dfl[k] = deqnt_affine_to_f32(box_tensor[offset + k], box_zp, box_scale);
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * stride;
                y1 = (-box[1] + i + 0.5) * stride;
                x2 = (box[2] + j + 0.5) * stride;
                y2 = (box[3] + i + 0.5) * stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount ++;
            }
        }
    }
    return validCount;
}

// ============================================
// 修复后的 post_process 函数
// ============================================
// int post_process(rknn_app_context_t *app_ctx, void *outputs, float conf_threshold, float nms_threshold, object_detect_result_list *od_results)
// {
//     rknn_tensor_mem **_outputs = (rknn_tensor_mem **)outputs;
//     std::vector<float> filterBoxes;
//     std::vector<float> objProbs;
//     std::vector<int> classId;
//     int validCount = 0;
//     int stride = 0;
//     int grid_h = 0;
//     int grid_w = 0;
//     int model_in_w = app_ctx->model_width;
//     int model_in_h = app_ctx->model_height;

//     memset(od_results, 0, sizeof(object_detect_result_list));

//     // default 3 branch
//     int output_per_branch = app_ctx->io_num.n_output / 3;

//     for (int i = 0; i < 3; i++)
//     {
//         // ============================================
//         // 关键修复：正确的 tensor 索引映射
//         // ============================================
//         // 每个分支3个输出：
//         //   branch_start + 0: box tensor    (64 channels)
//         //   branch_start + 1: score tensor  (C channels, C=类别数)
//         //   branch_start + 2: score_sum/aux (1 channel, 用于快速过滤)
//         // ============================================
//         int branch_start = i * output_per_branch;
//         int box_idx = branch_start + 0;     // box
//         int score_idx = branch_start + 1;   // score (各类别分数)
//         int score_sum_idx = branch_start + 2; // score_sum (快速过滤用)

//         int dfl_len = app_ctx->output_attrs[box_idx].dims[3] / 4;

//         void *score_sum = nullptr;
//         int32_t score_sum_zp = 0;
//         float score_sum_scale = 1.0;

//         if (output_per_branch >= 3) {
//             score_sum = _outputs[score_sum_idx]->virt_addr;
//             score_sum_zp = app_ctx->output_attrs[score_sum_idx].zp;
//             score_sum_scale = app_ctx->output_attrs[score_sum_idx].scale;
//         }

//         grid_h = app_ctx->output_attrs[box_idx].dims[1];
//         grid_w = app_ctx->output_attrs[box_idx].dims[2];
//         stride = model_in_h / grid_h;

//         // score tensor 的 channels 数 = 类别数
//         int score_channels = app_ctx->output_attrs[score_idx].dims[3];

//         if (app_ctx->is_quant) {
//             validCount += process_i8_rv1106(
//                 (int8_t *)_outputs[box_idx]->virt_addr, 
//                 app_ctx->output_attrs[box_idx].zp, 
//                 app_ctx->output_attrs[box_idx].scale,
//                 (int8_t *)_outputs[score_idx]->virt_addr, 
//                 app_ctx->output_attrs[score_idx].zp,
//                 app_ctx->output_attrs[score_idx].scale, 
//                 (int8_t *)score_sum, 
//                 score_sum_zp, 
//                 score_sum_scale,
//                 grid_h, grid_w, stride, dfl_len, 
//                 score_channels,  // 关键：传入正确的类别数
//                 filterBoxes, objProbs, classId, conf_threshold);
//         }
//         else
//         {
//             printf("RV1106 only support quantization mode\n");
//             return -1;
//         }
//     }

//     // no object detect
//     if (validCount <= 0)
//     {
//         return 0;
//     }
//     std::vector<int> indexArray;
//     for (int i = 0; i < validCount; ++i)
//     {
//         indexArray.push_back(i);
//     }
//     quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

//     std::set<int> class_set(std::begin(classId), std::end(classId));

//     for (auto c : class_set)
//     {
//         nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
//     }

//     int last_count = 0;
//     od_results->count = 0;

//     /* box valid detect target */
//     for (int i = 0; i < validCount; ++i)
//     {
//         if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
//         {
//             continue;
//         }
//         int n = indexArray[i];

//         float x1 = filterBoxes[n * 4 + 0];
//         float y1 = filterBoxes[n * 4 + 1];
//         float x2 = x1 + filterBoxes[n * 4 + 2];
//         float y2 = y1 + filterBoxes[n * 4 + 3];
//         int id = classId[n];
//         float obj_conf = objProbs[n];

//         od_results->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w));
//         od_results->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h));
//         od_results->results[last_count].box.right = (int)(clamp(x2, 0, model_in_w));
//         od_results->results[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h));
//         od_results->results[last_count].prop = obj_conf;
//         od_results->results[last_count].cls_id = id;
//         last_count++;
//     }
//     od_results->count = last_count;
//     return 0;
// }

int post_process(rknn_app_context_t *app_ctx, void *outputs, float conf_threshold, float nms_threshold, object_detect_result_list *od_results)
{
    rknn_tensor_mem **_outputs = (rknn_tensor_mem **)outputs;
    std::vector<float> filterBoxes;
    std::vector<float> objProbs;
    std::vector<int> classId;
    int validCount = 0;
    int stride = 0;
    int grid_h = 0;
    int grid_w = 0;
    int model_in_w = app_ctx->model_width;
    int model_in_h = app_ctx->model_height;

    memset(od_results, 0, sizeof(object_detect_result_list));

    // default 3 branch
    int output_per_branch = app_ctx->io_num.n_output / 3;

    for (int i = 0; i < 3; i++)
    {
        // ============================================
        // YOLOv8 输出布局（每个分支3个tensor）：
        //   branch_start + 0: box tensor    [1, H, W, 64]  (4 * dfl_len)
        //   branch_start + 1: score tensor  [1, H, W, C]   (C = 类别数)
        //   branch_start + 2: score_sum     [1, H, W, 1]   (快速过滤)
        // ============================================
        int branch_start = i * output_per_branch;
        int box_idx = branch_start + 0;
        int score_idx = branch_start + 1;
        int score_sum_idx = branch_start + 2;

        int dfl_len = app_ctx->output_attrs[box_idx].dims[3] / 4;

        void *score_sum = nullptr;
        int32_t score_sum_zp = 0;
        float score_sum_scale = 1.0;

        if (output_per_branch >= 3) {
            score_sum = _outputs[score_sum_idx]->virt_addr;
            score_sum_zp = app_ctx->output_attrs[score_sum_idx].zp;
            score_sum_scale = app_ctx->output_attrs[score_sum_idx].scale;
        }

        grid_h = app_ctx->output_attrs[box_idx].dims[1];
        grid_w = app_ctx->output_attrs[box_idx].dims[2];
        stride = model_in_h / grid_h;

        // score tensor 的 channels 数 = 类别数
        int score_channels = app_ctx->output_attrs[score_idx].dims[3];

        if (app_ctx->is_quant) {
            validCount += process_i8_rv1106(
                (int8_t *)_outputs[box_idx]->virt_addr, 
                app_ctx->output_attrs[box_idx].zp, 
                app_ctx->output_attrs[box_idx].scale,
                (int8_t *)_outputs[score_idx]->virt_addr, 
                app_ctx->output_attrs[score_idx].zp,
                app_ctx->output_attrs[score_idx].scale, 
                (int8_t *)score_sum, 
                score_sum_zp, 
                score_sum_scale,
                grid_h, grid_w, stride, dfl_len, 
                score_channels,
                filterBoxes, objProbs, classId, conf_threshold);
        }
        else
        {
            printf("RV1106 only support quantization mode\n");
            return -1;
        }
    }

    // no object detect
    if (validCount <= 0) return 0;

    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i)
        indexArray.push_back(i);
    
    quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));
    for (auto c : class_set)
        nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);

    int last_count = 0;
    for (int i = 0; i < validCount; ++i)
    {
        if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
            continue;
        
        int n = indexArray[i];
        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        float obj_conf = objProbs[n];

        od_results->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w));
        od_results->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h));
        od_results->results[last_count].box.right = (int)(clamp(x2, 0, model_in_w));
        od_results->results[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h));
        od_results->results[last_count].prop = obj_conf;
        od_results->results[last_count].cls_id = id;
        last_count++;
    }
    od_results->count = last_count;
    return 0;
}



int init_post_process()
{
    OBJ_CLASS_NUM = obj_class_num;

    // 释放旧内存（防止内存泄漏）
    if (labels != nullptr) {
        deinit_post_process();
    }

    // 分配新内存并清零（防止野指针）
    labels = new char*[OBJ_CLASS_NUM];
    memset(labels, 0, sizeof(char*) * OBJ_CLASS_NUM);

    int ret = 0;
    ret = loadLabelName(lable, labels);
    if (ret < 0)
    {
        printf("Load %s failed!\n", lable);
        return -1;
    }
    return 0;
}

char *coco_cls_to_name(int cls_id)
{

    static char null_str[] = "null";

    if (cls_id >= OBJ_CLASS_NUM)
    {
        return null_str;
    }

    if (labels[cls_id])
    {
        return labels[cls_id];
    }

    return null_str;
}

void deinit_post_process()
{
    if (labels == nullptr)
    {
        return;
    }
    for (int i = 0; i < OBJ_CLASS_NUM; i++)
    {
        if (labels[i] != nullptr)
        {
            free(labels[i]);
            labels[i] = nullptr;
        }
    }
    delete[] labels;
    labels = nullptr;
}

