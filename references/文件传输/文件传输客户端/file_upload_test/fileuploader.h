#ifndef FILEUPLOADER_H
#define FILEUPLOADER_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QTimer>
#include "Protocol.h"

class FileUploader : public QObject
{
    Q_OBJECT
public:
    explicit FileUploader(QObject *parent = nullptr);
    ~FileUploader();

    // 设置服务器地址
    void setServer(const QString &host, quint16 port);

    // 开始上传文件
    void uploadFile(const QString &filePath);

    // 取消上传
    void cancelUpload();

    // 是否正在上传
    bool isUploading() const { return m_uploading; }

signals:
    void connected();
    void disconnected();
    void uploadProgress(qint64 sentBytes, qint64 totalBytes, int percent);
    void chunkUploaded(int currentChunk, int totalChunks);
    void uploadFinished(bool success, const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onBytesWritten(qint64 bytes);
    void onRetryTimeout();

private:
    void sendNextChunk();
    void sendChunk(int index);
    bool buildUploadRequest(int chunkIndex, FILE_UPLOAD_REQ &req, int &actualSize);
    void resetState();
    void scheduleRetry();

    QTcpSocket *m_socket;
    QTimer *m_retryTimer;

    QString m_host;
    quint16 m_port;
    QString m_filePath;
    QFile *m_file;

    bool m_uploading = false;
    bool m_waitingForAck = false;
    int m_currentChunk = 0;
    int m_totalChunks = 0;
    qint64 m_fileSize = 0;
    qint64 m_sentBytes = 0;

    static constexpr int CHUNK_DATA_SIZE = 1024;
    static constexpr int MAX_RETRY_COUNT = 3;
    static constexpr int RETRY_INTERVAL_MS = 3000;

    int m_retryCount = 0;
    QByteArray m_pendingData; // 用于处理粘包
};

#endif // FILEUPLOADER_H
