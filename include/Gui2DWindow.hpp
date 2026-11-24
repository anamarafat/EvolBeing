#pragma once

#include "CartesianGrid.hpp"
#include "Geometry2D.hpp"

#include <QMainWindow>
#include <QAction>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QPointF>
#include <QColor>
#include <optional>
#include <vector>
#include <functional>
#include <cstdint>

class QToolBar;
class QVBoxLayout;
class QHBoxLayout;
class QWidget;
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
        enum class ThemePreset { SlateDark = 0, DeepDark = 1, LightGrey = 2 };
        struct SimulationTheme {
            QColor background;
            QColor activeCell;
            QColor deadCell;
            QColor geometryCell;
            QColor gridLines;
            QColor wallGeometry;
            QColor selectedVertex;
            QColor boundingBox;
        };
        ThemePreset themePreset_ = ThemePreset::SlateDark;
        SimulationTheme theme_{};
        void applySimulationTheme();
        void setSimulationTheme(ThemePreset preset);
        void updateSimulationTheme();

        // GUI construction: keep the constructor limited to composition.
        void setupSimulationView(QWidget* leftPanel);
        void setupMenus();
        QToolBar* createMeshToolbar(QWidget* leftPanel);
        QToolBar* createGeometryToolbar(QWidget* leftPanel);
        QToolBar* createAnalyzeToolbar(QWidget* leftPanel);
        void configureToolModes();
        void setupOutputPanel(QWidget* leftPanel, QVBoxLayout* leftLayout);
        void setupWorkflowPanel(QWidget* central, QHBoxLayout* mainLayout);
        void setupWorkflowTree(QWidget* panel);
        void finalizeGeometryForWorkflow();
        bool geometryFinalized_ = false;
        QAction* finalizeGeometryAction_ = nullptr;
        QAction* connectNodesAction_ = nullptr;
        geom::CartesianGrid grid_;
        geom::Geometry2D geometry_;
        std::vector<geom::Geometry2D> undoHistory_, redoHistory_;
        void checkpoint();
        void undoGeometry();
        void redoGeometry();
        void saveGeometry();
        void loadGeometry();
        void scaleGeometry();
        void connectNodesClick(const QPointF& pos);
        std::optional<geom::GeometryVertexID> connectStart_;
        QString geometryFilePath_;


        enum class RegionType { Live, Dead, Obstacle };
        struct RegionSeed {
            std::uint32_t id;
            QString name;
            QPointF vertex;
            RegionType type;
        };
        std::vector<RegionSeed> regionSeeds_;
        std::uint32_t nextRegionId_ = 1;
        QTreeWidgetItem* computeMeshNode_ = nullptr;
        QTreeWidgetItem* regionsNode_ = nullptr;
        std::optional<std::uint32_t> highlightedRegionId_;
        QGraphicsEllipseItem* regionSeedHighlight_ = nullptr;
        std::function<void(const QPointF&)> regionPickHandler_;
        void createRegion();
        void editRegion(std::uint32_t id);
        void refreshRegionsTree();
        void toggleRegionHighlight(std::uint32_t id);
        void clearRegionHighlight();

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
            DrawLine, DrawPolyline, DrawEdge, DrawVertex, ConnectNodes, GeometryVertex, GeometryEdge, GeometryPolyline
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