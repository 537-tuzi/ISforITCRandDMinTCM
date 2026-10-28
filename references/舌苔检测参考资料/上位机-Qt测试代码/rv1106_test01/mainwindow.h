#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SerialWorker;
class VideoReceiver;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // 辅助函数
    void refreshPorts();

private slots:
    // UI按钮槽
    void on_btn_serial_clicked();
    void on_btn_net_clicked();
    void on_btn_action_clicked();
    void on_btn_serial_refresh_clicked();

    // 串口工作线程回调
    void onSerialConnected(bool success);
    void onResultReceived(const QString &text);
    void onLogMessage(const QString &msg);
    // 【NEW】新增结构化推理结果槽声明
    void onInferenceResultReceived(quint16 echoCmd, quint8 tongueCode, quint8 confRaw, int confPercent, const QString &typeName);

    // 网络视频线程回调
    void onNetConnected(bool success);
    void onFrameReceived(const QImage &image);

    // 画面刷新定时器
    void updateVideoDisplay();

private:
    Ui::MainWindow *ui;

    // 串口线程资源
    QThread *m_serialThread;
    SerialWorker *m_serialWorker;

    // 视频网络线程资源
    QThread *m_netThread;
    VideoReceiver *m_videoReceiver;

    // 视频显示
    QTimer *m_displayTimer;
    QImage m_currentFrame;
    bool m_isVideoPlaying;
};

#endif // MAINWINDOW_H