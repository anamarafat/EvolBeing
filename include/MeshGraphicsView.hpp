#pragma once

#include <QGraphicsView>
#include <QPoint>
#include <functional>
#include <QPointF>

class MeshGraphicsView : public QGraphicsView
{
public:
    explicit MeshGraphicsView(QWidget* parent = nullptr);
    using ClickHandler = std::function<void(const QPointF&)>;
    void setClickHandler(ClickHandler handler);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool panning_ = false;
    QPoint lastMousePos_;
    ClickHandler clickHandler_;
};