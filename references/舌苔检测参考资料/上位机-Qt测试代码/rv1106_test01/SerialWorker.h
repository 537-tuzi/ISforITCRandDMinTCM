#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(const QString &portName, qint32 baudRate, QObject *parent = nullptr);
    ~SerialWorker();

    void start();
    void stop();
    void sendCommand(quint16 cmd);

signals:
    // 串口连接状态
    void connected(bool ok);
    // 日志打印信号
    void logMessage(const QString &msg);
    // 兼容旧版文本字符串结果
    void resultReceived(const QString &text);
    // 【NEW】结构化推理结果信号，完整携带协议字段
    void inferenceResultReceived(quint16 echoCmd, quint8 tongueCode, quint8 confRaw, int confPercent, const QString &typeName);

private slots:
    void handleReadyRead();

private:
    void processBuffer();
    QByteArray buildFrame(quint16 cmd);

    QSerialPort *m_serial;
    QString m_portName;
    qint32 m_baudRate;
    bool m_running;
    QByteArray m_rxBuffer; // 二进制接收缓存
};

#endif // SERIALWORKER_H