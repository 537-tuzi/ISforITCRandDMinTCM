#pragma once
enum SERVICE_TYPE
{
    FILE_UPLOAD,
};
#pragma pack(push, 1)
// 通信协议
typedef struct
{
    SERVICE_TYPE type;  // 业务类型
    int len;  // 数据包长度
}HEAD;

typedef struct {
    char filename[256];
    int chunkTotal;  // 总分片数
    int chunkIndex; // 当前分片序号
    int chunkSize;  // 分片大小
    char chunkData[10240];  // 分片数据

}FILE_UPLOAD_REQ;
#pragma pack(pop)
//typedef struct {
//    uint32_t chunkIndex;   // 确认的分片序号
//    uint32_t status;       // 状态码：0成功/1失败/2重传请求
//    char msg[64];          // 提示信息（可选）
//}FILE_UPLOAD_RESP;
// typedef struct{
//     char filename[128];
//     int chunkTotal;  // 总分片数
//     int chuunkIndex; // 当前分片序号
//     int chunkSize;  // 分片大小
//     char chunkData[1024];  // 分片数据

// }FILE_UPLOAD_RESP

