#include "MeshGraphicsView.hpp"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QKeyEvent>
#include <utility>

MeshGraphicsView::MeshGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing, false);

    // Receive mouse movement without pressing a button.
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    // Receive Enter and Escape.
    setFocusPolicy(Qt::StrongFocus);
}

void MeshGraphicsView::setClickHandler(ClickHandler handler)
{
    clickHandler_ = std::move(handler);
}

void MeshGraphicsView::setMoveHandler(MoveHandler handler)
{
    moveHandler_ = std::move(handler);
}

void MeshGraphicsView::setFinishHandler(CommandHandler handler)
{
    finishHandler_ = std::move(handler);
}

void MeshGraphicsView::setCancelHandler(CommandHandler handler)
{
    cancelHandler_ = std::move(handler);
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
    // Middle button: start panning
    if (event->button() == Qt::MiddleButton)
    {
        panning_ = true;
        lastMousePos_ = event->pos();

        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    // Right button: finish drawing when enabled
    if (event->button() == Qt::RightButton &&
        finishHandler_)
    {
        finishHandler_();
        event->accept();
        return;
    }

    // Left button: selection or drawing
    if (event->button() == Qt::LeftButton &&
        clickHandler_)
    {
        setFocus();

        const QPointF scenePos =
            mapToScene(event->pos());

        clickHandler_(scenePos);

        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}


void MeshGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    // Preserve existing middle-mouse panning
    if (panning_)
    {
        const QPoint delta =
            event->pos() - lastMousePos_;

        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() - delta.x());

        verticalScrollBar()->setValue(
            verticalScrollBar()->value() - delta.y());

        lastMousePos_ = event->pos();

        event->accept();
        return;
    }

    // Send mouse position for geometry preview
    if (moveHandler_)
    {
        const QPointF scenePos =
            mapToScene(event->pos());

        moveHandler_(scenePos);
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

void MeshGraphicsView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter)
    {
        if (finishHandler_)
        {
            finishHandler_();
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_Escape)
    {
        if (cancelHandler_)
        {
            cancelHandler_();
            event->accept();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}