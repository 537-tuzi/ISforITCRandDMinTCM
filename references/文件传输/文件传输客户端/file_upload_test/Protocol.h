#ifndef PROTOCOL_H
#define PROTOCOL_H
// 业务类型枚举
enum SERVICE_TYPE
{
    FILE_UPLOAD = 0,
};

// 通信协议头
#pragma pack(push, 1)
typedef struct
{
    SERVICE_TYPE type;  // 业务类型
    int len;            // 数据包长度（HEAD + 后续数据的总长度）
} HEAD;

// 文件上传请求体
typedef struct {
    char filename[256];     // 文件名
    int chunkTotal;         // 总分片数
    int chunkIndex;         // 当前分片序号（从0开始）
    int chunkSize;          // 分片实际大小
    char chunkData[10240];   // 分片数据
} FILE_UPLOAD_REQ;
#pragma pack(pop)
#endif // PROTOCOL_H
