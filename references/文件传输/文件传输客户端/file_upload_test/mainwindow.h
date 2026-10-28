#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QPushButton;
class QProgressBar;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

class TcpClient;
class FileUploadTask;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectFile();
    void onUpload();
    void onCancel();
    void onProgress(qint64 sent, qint64 total, int percent);
    void onChunkSent(int current, int total);
    void onFinished(bool success, const QString &msg);
    void onError(const QString &error);

private:
    TcpClient *m_tcpClient;
    FileUploadTask *m_uploadTask;

    QLineEdit *m_fileEdit;
    QLineEdit *m_hostEdit;
    QLineEdit *m_portEdit;
    QPushButton *m_selectBtn;
    QPushButton *m_uploadBtn;
    QPushButton *m_cancelBtn;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_chunkLabel;
};

#endif // MAINWINDOW_H