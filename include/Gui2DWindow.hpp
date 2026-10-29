#pragma once

#include "CartesianGrid.hpp"

#include <QMainWindow>
#include <QAction>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QPointF>

class QGraphicsScene;
class QGraphicsView;
class QGraphicsRectItem;
class QDoubleSpinBox;
class QPushButton;
class QGraphicsPathItem;
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;


class Gui2DWindow : public QMainWindow
{
    public:
        explicit Gui2DWindow(QWidget* parent = nullptr);

    private:
        geom::CartesianGrid grid_;

        // 2D canvas
        QGraphicsScene* scene_ = nullptr;
        QGraphicsView* view_ = nullptr;
        QGraphicsRectItem* boundingBoxItem_ = nullptr;
        QTreeWidget* workflowTree_ = nullptr;
        QTreeWidgetItem* boundingBoxNode_ = nullptr;

        // Bounding box inputs
        QDoubleSpinBox* widthInput_ = nullptr;
        QDoubleSpinBox* heightInput_ = nullptr;
        QDoubleSpinBox* minCellSizeInput_ = nullptr;

        // Controls
        QPushButton* applyButton_ = nullptr;
        QTextEdit* outputText_ = nullptr;

        // Grid items
        QGraphicsPathItem* gridItem_  = nullptr;

        // Grid view selection
        enum class SelectionMode
        {
            None,
            Cell,
            Face
        };
        SelectionMode selectionMode_ = SelectionMode::None;

        QAction* fitViewAction_ = nullptr;
        QAction* cellSelectAction_ = nullptr;
        QAction* faceSelectAction_ = nullptr;
        QGraphicsRectItem* selectedCellItem_ = nullptr;
        QGraphicsLineItem* selectedFaceItem_ = nullptr;
        geom::Cell* selectedCell_ = nullptr;
        geom::Face* selectedFace_ = nullptr;
        
        void fitMeshView();
        void handleMeshClick(const QPointF& scenePos);
        void clearSelectionHighlight();

        // Grid dimensions
        int nx_ = 0;
        int ny_ = 0;

        // Methods
        void createBoundingBox();
};