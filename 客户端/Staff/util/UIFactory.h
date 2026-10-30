#ifndef UIFACTORY_H
#define UIFACTORY_H
// 可复用 UI 组件工厂（util）：行构造与可点击容器统一从这里出。
// 各容器内的动态组件由调用方把返回的值指针存为类成员（如 m_type/m_gm），
// 之后实现里直接调成员；一次性静态组件（只读行、步骤行）无需保留引用。
#include <QFrame>
#include <QString>

class QLabel;
class QMouseEvent;

namespace uif {

// 可点击容器（四宫格入口）：按下即发 clicked
class ClickFrame : public QFrame
{
    Q_OBJECT
public:
    explicit ClickFrame(QWidget* parent = nullptr);
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent*) override;
};

// "标签: 值"横排行（弹窗明细/记录卡）；rowObj 空=不加行样式，valObj 空=liVal
QWidget* makeFieldRow(const QString& key, const QString& val,
                      const QString& rowObj = QString(), const QString& valObj = QString());

// 标签在上、值在下堆叠行（信息面板/只读档案）；值经 val 输出给调用方作成员
QWidget* makeStackedRow(const QString& key, const QString& rowObj,
                        const QString& keyObj, const QString& valObj, QLabel** val);

} // namespace uif
#endif // UIFACTORY_H
