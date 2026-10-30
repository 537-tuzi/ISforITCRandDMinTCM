#ifndef PROFILEVIEW_H
#define PROFILEVIEW_H
// 初次建档页（建档后=我的档案）：左档案卡 + 右 AI 面板（自包含演示逻辑，对应 MD §5）
#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QScrollArea;

class ProfileView : public QWidget
{
    Q_OBJECT
public:
    explicit ProfileView(QWidget* parent = nullptr);

    void setProfile(const QString& gm, const QString& jw);   // 保存后同步面板

signals:
    void profileSaved(const QString& gm, const QString& jw); // WinManager 做全局状态切换

private slots:
    void onSave();
    void onHoldReleased();   // 松开按钮：推进一轮 AI 演示对话

private:
    QWidget* makeRo(const QString& key, const QString& val);
    void appendAi(const QString& text);
    void appendMe(const QString& text);
    void scrollChatBottom();

    QPlainTextEdit* m_gm  = nullptr;
    QPlainTextEdit* m_jw  = nullptr;
    QLabel* m_okNote = nullptr;
    QLabel* m_intro  = nullptr;
    QVBoxLayout* m_chat = nullptr;
    QScrollArea* m_scroll = nullptr;
    int m_aiIdx = 0;         // 演示对话进度（4 轮脚本）
};

#endif
