#pragma once

#include "CartesianGrid.hpp"
#include "Geometry2D.hpp"

#include <QMainWindow>
#include <QAction>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QPointF>
#include <optional>
#include <vector>

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
        geom::Geometry2D geometry_;

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
            Face,
            Node,
            DrawLine, DrawPolyline, DrawEdge, DrawVertex, GeometryVertex, GeometryEdge, GeometryPolyline
        };

        SelectionMode selectionMode_ = SelectionMode::None;

        QAction* fitViewAction_ = nullptr;
        QAction* infoAction_ = nullptr;
        QAction* snapAction_ = nullptr;
        QAction* snapAllAction_ = nullptr;
        QAction* closeAction_ = nullptr;
        QAction* newEdgeAction_ = nullptr;
        QAction* drawPolylineAction_ = nullptr;
        QAction* drawEdgeAction_ = nullptr;
        QAction* drawVertexAction_ = nullptr;
        QAction* selectGeometryVertexAction_ = nullptr;
        QAction* selectGeometryEdgeAction_ = nullptr;
        QAction* selectGeometryPolylineAction_ = nullptr;
        std::optional<geom::GeometryVertexID> selectedGeometryVertex_;
        std::optional<geom::GeometryEdgeID> selectedGeometryEdge_;
        std::optional<geom::PolylineID> selectedGeometryPolyline_;
        QPointF nearestMeshNode(const QPointF& point) const;
        void applySnap();
        void closeActivePolyline();
        void printSelectedInfo();
        void handleGeometrySelection(const QPointF& point);
        void redrawGeometry();
        void deleteSelectedEntity();
        QAction* cellSelectAction_ = nullptr;
        QAction* faceSelectAction_ = nullptr;
        QAction* nodeSelectAction_ = nullptr;
        QAction* drawLineAction_ = nullptr;
        QGraphicsRectItem* selectedCellItem_ = nullptr;
        QGraphicsLineItem* selectedFaceItem_ = nullptr;
        QGraphicsEllipseItem* selectedNodeItem_ = nullptr;
        geom::Cell* selectedCell_ = nullptr;
        geom::Face* selectedFace_ = nullptr;
        geom::Node* selectedNode_ = nullptr;
        void fitMeshView();
        void handleMeshClick(const QPointF& scenePos);
        void clearSelectionHighlight();

        // ========================================
        // Geometry drawing
        // ========================================

        // Currently active polyline
        std::optional<geom::PolylineID> activePolyline_;

        // Qt items belonging to the unfinished polyline
        std::vector<QGraphicsItem*> activeDrawingItems_;
        std::vector<QGraphicsItem*> geometryItems_;
        std::vector<QGraphicsItem*> geometryHighlightItems_;
        std::vector<std::vector<QGraphicsItem*>> polylineGraphics_;

        // Temporary preview line
        QGraphicsLineItem* previewLineItem_ = nullptr;

        QTreeWidgetItem* geometryRootNode_ = nullptr;
        QTreeWidgetItem* unassignedNode_ = nullptr;

        void refreshGeometryTree();
        void showGeometryProperties();
        void validateGeometryFromMenu();
        void createGeometryThread();
        void assignSelectedPolyline();
        void renameSelectedGeometry();
        void deleteSelectedGeometry();
        
        // Mouse interaction
        void handleDrawingClick(const QPointF& scenePos);
        void handleDrawingMove(const QPointF& scenePos);

        // Drawing lifecycle
        void finishDrawing();
        void cancelDrawing();
        void clearPreviewLine();

        // Remove geometry graphics before resetting the scene
        void clearGeometryGraphics();
        void showGeometryContextMenu(const QPoint& pos);
        void highlightGeometryPart(geom::PolylineID id);
        void clearGeometryPartHighlight();

        // Grid dimensions
        int nx_ = 0;
        int ny_ = 0;

        // Methods
        void createBoundingBox();
};