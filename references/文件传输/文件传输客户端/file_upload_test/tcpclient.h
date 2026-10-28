#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    // 连接管理
    bool connectToHost(const QString &host, quint16 port, int timeoutMs = 5000);
    void disconnectFromHost();
    bool isConnected() const;

    // 数据发送（通用接口，不感知业务）
    qint64 sendData(const QByteArray &data);
    qint64 sendData(const char *data, int len);

    // 接收数据（非阻塞读取当前可用数据）
    QByteArray readAll();
    bool waitForReadyRead(int msecs = 3000);

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_socket;
};

#endif // TCPCLIENT_H