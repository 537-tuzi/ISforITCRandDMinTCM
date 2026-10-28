#ifndef FILEUPLOADTASK_H
#define FILEUPLOADTASK_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include <QThread>
#include "Protocol.h"
#include "TcpClient.h"

class FileUploadTask : public QObject
{
    Q_OBJECT
public:
    explicit FileUploadTask(TcpClient *tcpClient, QObject *parent = nullptr);
    ~FileUploadTask();

    // 设置上传参数
    void setFilePath(const QString &filePath);
    void setServer(const QString &host, quint16 port);

    // 任务控制
    void start();
    void cancel();
    bool isRunning() const;

signals:
    void uploadStarted();
    void uploadProgress(qint64 sentBytes, qint64 totalBytes, int percent);
    void chunkSent(int currentChunk, int totalChunks);
    void uploadFinished(bool success, const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &error);
    void onTcpDataReceived(const QByteArray &data);

private:
    bool connectServer();
    void sendAllChunks();           // 连续发送所有分片
    bool sendSingleChunk(int index);
    bool buildChunkRequest(int chunkIndex, FILE_UPLOAD_REQ &req, int &actualSize);
    void cleanup();

    TcpClient *m_tcpClient;         // 网络传输工具（外部传入，不拥有所有权）
    bool m_ownTcpClient = false;    // 是否自己创建的TcpClient

    QString m_filePath;
    QString m_host;
    quint16 m_port = 0;

    QFile *m_file = nullptr;
    qint64 m_fileSize = 0;
    int m_totalChunks = 0;
    int m_currentChunk = 0;
    qint64 m_sentBytes = 0;

    bool m_running = false;
    bool m_connected = false;

    static constexpr int CHUNK_DATA_SIZE = 10240;
    // static constexpr int SEND_INTERVAL_MS = 10;  // 分片发送间隔，防止拥塞

    QByteArray m_recvBuffer;        // 接收缓冲区（处理服务器响应）
};

#endif // FILEUPLOADTASK_H