#include "MeshGraphicsView.hpp"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <utility>

MeshGraphicsView::MeshGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing, false);
}

void MeshGraphicsView::setClickHandler(ClickHandler handler)
{
    clickHandler_ = std::move(handler);
}

void MeshGraphicsView::wheelEvent(QWheelEvent* event)
{
    constexpr double zoomFactor = 1.15;

    if (event->angleDelta().y() > 0)
        scale(zoomFactor, zoomFactor);
    else if (event->angleDelta().y() < 0)
        scale(1.0 / zoomFactor, 1.0 / zoomFactor);

    event->accept();
}

void MeshGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        panning_ = true;
        lastMousePos_ = event->pos();

        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && clickHandler_)
    {
        QPointF scenePos = mapToScene(event->pos());

        clickHandler_(scenePos);

        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void MeshGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_)
    {
        const QPoint delta = event->pos() - lastMousePos_;

        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() - delta.x());

        verticalScrollBar()->setValue(
            verticalScrollBar()->value() - delta.y());

        lastMousePos_ = event->pos();

        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void MeshGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        panning_ = false;
        setCursor(Qt::ArrowCursor);

        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}