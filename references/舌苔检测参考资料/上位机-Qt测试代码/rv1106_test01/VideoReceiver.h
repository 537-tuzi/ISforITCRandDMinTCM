#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QObject>
#include <QTcpSocket>
#include <QImage>
#include <QByteArray>
#include <QTimer>
class VideoReceiver : public QObject
{
    Q_OBJECT
public:
    explicit VideoReceiver(const QString &host, quint16 port, QObject *parent = nullptr);
    ~VideoReceiver();

public slots:
    void start();
    void stop();

signals:
    void connected(bool success);
    void frameReceived(const QImage &image);
    void logMessage(const QString &msg);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    bool readAllBytes(QByteArray &buffer, qint64 size);
    void parseFrame();

    QTcpSocket *m_socket;
    QString m_host;
    quint16 m_port;
    bool m_running;
    QByteArray m_rxBuffer;
    enum State { WaitingMagic, WaitingLen, WaitingRows, WaitingCols, WaitingData } m_state;
    quint32 m_dataLen, m_rows, m_cols;
};

#endif // VIDEORECEIVER_H