#include "Gui2DWindow.hpp"
#include "MeshGraphicsView.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainter>

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QTextEdit>
#include <QToolBar>
#include <QActionGroup>
#include <QToolButton>
#include <QWidgetAction>
#include <QCheckBox>
#include <QSet>
#include <QLineF>

#include <QInputDialog>
#include <QMessageBox>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <limits>

#include <QMenu>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>

Gui2DWindow::Gui2DWindow(QWidget* parent)
    : QMainWindow(parent)
{
    qDebug() << "Gui2DWindow constructor START";
    setWindowTitle("EvolBeing - GUI2D");
    resize(1100, 750);

    // ========================================
    // Main container and layout
    // ========================================

    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);

    // ========================================
    // Left: 2D viewport
    // ========================================

    scene_ = new QGraphicsScene(this);

    // Light gray background
    scene_->setBackgroundBrush(Qt::lightGray);

    // Initial empty scene
    scene_->setSceneRect(0, 0, 800, 600);


    // Left side container
    auto* leftPanel = new QWidget(central);

    // Vertical layout: viewport above, output below
    auto* leftLayout = new QVBoxLayout(leftPanel);

    // Remove unnecessary margins
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);

    // Graphics viewport
    view_ = new MeshGraphicsView(leftPanel);
    view_->setScene(scene_);

    auto* meshView = static_cast<MeshGraphicsView*>(view_);
    meshView->setClickHandler(
        [this](const QPointF& scenePos)
        {
            handleMeshClick(scenePos);
        });

    meshView->setMoveHandler(
        [this](const QPointF& scenePos)
        {
            handleDrawingMove(scenePos);
        });

    meshView->setFinishHandler(
        [this]()
        {
            finishDrawing();
        });

    meshView->setCancelHandler(
        [this]()
        {
            cancelDrawing();
        });

    // Toolbar for mesh viewing and selection
    QToolBar* meshToolbar = new QToolBar(leftPanel);
    meshToolbar->setMovable(false);

    // Fit view button
    fitViewAction_ = meshToolbar->addAction("Fit View");
    infoAction_ = meshToolbar->addAction("Info");
    meshToolbar->addSeparator();

    // Selection buttons
    cellSelectAction_ = meshToolbar->addAction("Cell");
    faceSelectAction_ = meshToolbar->addAction("Face");
    nodeSelectAction_ = meshToolbar->addAction("Node");

    // Geometry drawing button
    meshToolbar->addSeparator();
    drawLineAction_ = meshToolbar->addAction("Draw Line");
    drawPolylineAction_ = meshToolbar->addAction("Polyline");
    drawEdgeAction_ = meshToolbar->addAction("Draw Edge");
    drawVertexAction_ = meshToolbar->addAction("Draw Vertex");
    meshToolbar->addSeparator();
    selectGeometryVertexAction_ = meshToolbar->addAction("Select Vertex");
    selectGeometryEdgeAction_ = meshToolbar->addAction("Select Edge");
    selectGeometryPolylineAction_ = meshToolbar->addAction("Select Polyline");
    meshToolbar->addSeparator();
    auto* snapButton = new QToolButton(meshToolbar);
    snapButton->setText("Snap");
    snapButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto* snapMenu = new QMenu(snapButton);
    snapAllAction_ = snapMenu->addAction("Snap All geometry vertices");
    snapAllAction_->setCheckable(true);
    snapButton->setMenu(snapMenu);
    meshToolbar->addWidget(snapButton);
    connect(snapButton, &QToolButton::clicked, this, &Gui2DWindow::applySnap);
    auto* closeButton = new QToolButton(meshToolbar);
    closeButton->setText("Close Polyline");
    closeButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto* closeMenu = new QMenu(closeButton);
    newEdgeAction_ = closeMenu->addAction("New Edge");
    newEdgeAction_->setCheckable(true);
    newEdgeAction_->setChecked(true);
    closeButton->setMenu(closeMenu);
    meshToolbar->addWidget(closeButton);
    connect(closeButton, &QToolButton::clicked, this, &Gui2DWindow::closeActivePolyline);
    auto* deleteEntityAction = meshToolbar->addAction("Delete Entity");
    connect(deleteEntityAction, &QAction::triggered, this, &Gui2DWindow::deleteSelectedEntity);
    connect(infoAction_, &QAction::triggered, this, &Gui2DWindow::printSelectedInfo);

    // Allow buttons to remain pressed/highlighted
    cellSelectAction_->setCheckable(true);
    faceSelectAction_->setCheckable(true);
    nodeSelectAction_->setCheckable(true);
    drawLineAction_->setCheckable(true);
    for (auto* action : {drawPolylineAction_, drawEdgeAction_, drawVertexAction_,
                         selectGeometryVertexAction_, selectGeometryEdgeAction_, selectGeometryPolylineAction_})
        action->setCheckable(true);

    // Group the selection buttons
    QActionGroup* selectionGroup = new QActionGroup(this);
    selectionGroup->setExclusionPolicy(
        QActionGroup::ExclusionPolicy::ExclusiveOptional);

    selectionGroup->addAction(cellSelectAction_);
    selectionGroup->addAction(faceSelectAction_);
    selectionGroup->addAction(nodeSelectAction_);
    selectionGroup->addAction(drawLineAction_);
    for (auto* action : {drawPolylineAction_, drawEdgeAction_, drawVertexAction_,
                         selectGeometryVertexAction_, selectGeometryEdgeAction_, selectGeometryPolylineAction_})
        selectionGroup->addAction(action);

    // Fit View
    connect(fitViewAction_, &QAction::triggered,
            this, &Gui2DWindow::fitMeshView);

    // ========================================
    // Selection and drawing tool switching
    // ========================================

    auto connectTool =
        [this](QAction* action, SelectionMode mode)
    {
        connect(action, &QAction::toggled,
                this, [this, mode](bool checked)
        {
            if (checked)
            {
                // Finish an unfinished polyline when
                // switching away from drawing mode.
                if ((selectionMode_ == SelectionMode::DrawLine || selectionMode_ == SelectionMode::DrawPolyline || selectionMode_ == SelectionMode::DrawEdge) &&
                    mode != selectionMode_)
                {
                    finishDrawing();
                }

                selectionMode_ = mode;
                clearSelectionHighlight();
            }
            else if (selectionMode_ == mode)
            {
                if (mode == SelectionMode::DrawLine || mode == SelectionMode::DrawPolyline || mode == SelectionMode::DrawEdge)
                    finishDrawing();

                selectionMode_ = SelectionMode::None;
                clearSelectionHighlight();
            }
        });
    };

    connectTool(cellSelectAction_, SelectionMode::Cell);
    connectTool(faceSelectAction_, SelectionMode::Face);
    connectTool(nodeSelectAction_, SelectionMode::Node);
    connectTool(drawLineAction_, SelectionMode::DrawLine);
    connectTool(drawPolylineAction_, SelectionMode::DrawPolyline);
    connectTool(drawEdgeAction_, SelectionMode::DrawEdge);
    connectTool(drawVertexAction_, SelectionMode::DrawVertex);
    connectTool(selectGeometryVertexAction_, SelectionMode::GeometryVertex);
    connectTool(selectGeometryEdgeAction_, SelectionMode::GeometryEdge);
    connectTool(selectGeometryPolylineAction_, SelectionMode::GeometryPolyline);

    // Viewport takes all remaining vertical space
    leftLayout->addWidget(meshToolbar);
    leftLayout->addWidget(view_, 1);

    // Create output panel
    outputText_ = new QTextEdit(leftPanel);
    outputText_->setReadOnly(true);
    outputText_->setFixedHeight(120);
    outputText_->setPlaceholderText("Output messages will appear here...");


    // Output stays below viewport
    leftLayout->addWidget(outputText_, 0);

    // Add entire left panel to main horizontal layout
    mainLayout->addWidget(leftPanel, 1);

    // ========================================
    // Right: control panel
    // ========================================

    auto* panel = new QWidget(central);
    panel->setFixedWidth(240);

    auto* panelLayout = new QVBoxLayout(panel);

   // Workflow heading
    auto* workflowLabel = new QLabel("Workflow", panel);
    panelLayout->addWidget(workflowLabel);

    // Create workflow tree
    workflowTree_ = new QTreeWidget(panel);
    workflowTree_->setColumnCount(1);
    workflowTree_->setHeaderHidden(true);
    workflowTree_->setRootIsDecorated(true);
    workflowTree_->setIndentation(14);
    workflowTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(workflowTree_,
            &QTreeWidget::customContextMenuRequested,
            this,
            &Gui2DWindow::showGeometryContextMenu);

    connect(
        workflowTree_,
        &QTreeWidget::currentItemChanged,
        this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem*)
        {
            clearGeometryPartHighlight();
            selectedGeometryPolyline_.reset();

            if (!current)
                return;

            if (current->data(0, Qt::UserRole).toString()
                != "polyline")
                return;

            const auto id =
                current->data(0, Qt::UserRole + 1)
                    .value<geom::PolylineID>();

            selectedGeometryPolyline_ = id;
            highlightGeometryPart(id);
        }
    );


    // Add Bounding Box workflow node
    boundingBoxNode_ = new QTreeWidgetItem(workflowTree_);
    boundingBoxNode_->setText(0, "Bounding Box");

    // Container for bounding box parameters
    auto* boundingBoxSettings = new QWidget(workflowTree_);
    auto* settingsLayout = new QFormLayout(boundingBoxSettings);

    // Width
    widthInput_ = new QDoubleSpinBox(boundingBoxSettings);
    widthInput_->setRange(0.01, 100000.0);
    widthInput_->setDecimals(2);
    widthInput_->setValue(800.0);

    // Height
    heightInput_ = new QDoubleSpinBox(boundingBoxSettings);
    heightInput_->setRange(0.01, 100000.0);
    heightInput_->setDecimals(2);
    heightInput_->setValue(600.0);

    // Minimum cell size
    minCellSizeInput_ = new QDoubleSpinBox(boundingBoxSettings);
    minCellSizeInput_->setRange(0.01, 100000.0);
    minCellSizeInput_->setDecimals(2);
    minCellSizeInput_->setValue(1.0);

    // Add inputs to form
    settingsLayout->addRow("Width:", widthInput_);
    settingsLayout->addRow("Height:", heightInput_);
    settingsLayout->addRow("Min. Cell:", minCellSizeInput_);

    // Create child item to hold the settings widget
    auto* settingsNode = new QTreeWidgetItem(boundingBoxNode_);

    // Embed the settings widget in the tree
    workflowTree_->setItemWidget(
        settingsNode,
        0,
        boundingBoxSettings
    );

    // Give the settings row enough height
    settingsNode->setSizeHint(
        0,
        boundingBoxSettings->sizeHint()
    );

    // Initially expanded
    boundingBoxNode_->setExpanded(true);

    /* Geometry Tree */
    geometryRootNode_ =
        new QTreeWidgetItem(workflowTree_);
    geometryRootNode_->setText(0, "Geometry Parts");
    unassignedNode_ =
        new QTreeWidgetItem(geometryRootNode_);
    unassignedNode_->setText(0, "Unassigned");
    geometryRootNode_->setExpanded(true);
    unassignedNode_->setExpanded(true);


    // Add workflow tree to panel
    panelLayout->addWidget(workflowTree_, 1);

    // Apply button at the bottom
    applyButton_ = new QPushButton("Apply", panel);
    panelLayout->addWidget(applyButton_);

    // Add right panel to main horizontal layout
    mainLayout->addWidget(panel);

    // Connect Apply to existing function
    connect(applyButton_,
            &QPushButton::clicked,
            this,
            &Gui2DWindow::createBoundingBox);

    setCentralWidget(central);
    
    qDebug() << "Gui2DWindow constructor END";
}


// ============================================
// Create or update the bounding box
// ============================================

void Gui2DWindow::createBoundingBox()
{
    const double width = widthInput_->value();
    const double height = heightInput_->value();
    const double cellSize = minCellSizeInput_->value();

    // Calculate number of cells
    const double nxReal = std::ceil(width / cellSize);
    const double nyReal = std::ceil(height / cellSize);

    // Prevent integer overflow and unreasonable grid sizes
    constexpr double maxCells = 100'000'000.0;

    if (nxReal > std::numeric_limits<int>::max() ||
        nyReal > std::numeric_limits<int>::max() ||
        nxReal * nyReal > maxCells)
    {
        outputText_->append("ERROR: Grid resolution too large. Increase cell size.");
        return;
    }

    nx_ = static_cast<int>(nxReal);
    ny_ = static_cast<int>(nyReal);

    // Remove selection highlights before mesh regeneration
    clearSelectionHighlight();
    finishDrawing();
    clearGeometryGraphics();

    grid_.generate(
        static_cast<std::uint32_t>(nx_),
        static_cast<std::uint32_t>(ny_),
        cellSize
    );

    // Remove previous bounding box
    if (boundingBoxItem_)
    {
        scene_->removeItem(boundingBoxItem_);
        delete boundingBoxItem_;
        boundingBoxItem_ = nullptr;
    }

    // White bounding box outline
    QPen pen(Qt::white);
    pen.setWidth(0);

    boundingBoxItem_ = scene_->addRect(
        0.0,
        0.0,
        width,
        height,
        pen,
        Qt::NoBrush
    );

        // ============================================
    // 6. DRAW GRID LINES HERE
    // ============================================

    // Remove old grid
    if (gridItem_)
    {
        scene_->removeItem(gridItem_);
        delete gridItem_;
        gridItem_ = nullptr;
    }

    QPainterPath gridPath;

    // Vertical lines
    for (int i = 1; i < nx_; ++i)
    {
        double x = i * cellSize;

        if (x >= width)
            break;

        gridPath.moveTo(x, 0.0);
        gridPath.lineTo(x, height);
    }

    // Horizontal lines
    for (int j = 1; j < ny_; ++j)
    {
        double y = j * cellSize;

        if (y >= height)
            break;

        gridPath.moveTo(0.0, y);
        gridPath.lineTo(width, y);
    }

    QPen gridPen(QColor(65, 65, 65));
    gridPen.setWidth(0);

    gridItem_ = scene_->addPath(
        gridPath,
        gridPen,
        Qt::NoBrush
    );

    gridItem_->setZValue(-1);
    redrawGeometry();

    // Margin around bounding box
    const double margin =
        0.05 * std::max(width, height);

    scene_->setSceneRect(
        -margin,
        -margin,
        width + 2.0 * margin,
        height + 2.0 * margin
    );

    // Fit domain inside viewport
    view_->fitInView(
        scene_->sceneRect(),
        Qt::KeepAspectRatio
    );

    // Display grid information
    const long long totalCells =
        static_cast<long long>(nx_) * ny_;

    outputText_->append(QString("Bounding box created: %1 x %2\n"
                                "Cell size: %3\n"
                                "Grid: %4 x %5\n"
                                "Total cells: %6\n")
                                .arg(width)
                                .arg(height)
                                .arg(cellSize)
                                .arg(nx_)
                                .arg(ny_)
                                .arg(totalCells)
                        );
    
    outputText_->append("===== Cartesian Mesh Statistics =====");
    outputText_->append(QString("Grid Dimensions : %1 x %2").arg(nx_).arg(ny_));
    outputText_->append(
        QString("Cell Size       : %1").arg(cellSize)
    );
    outputText_->append(
        QString("Total Cells     : %1").arg(grid_.cellCount())
    );
    outputText_->append(
        QString("Total Faces     : %1").arg(grid_.faceCount())
    );
    outputText_->append(
        QString("Total Nodes     : %1").arg(grid_.nodeCount())
    );
    outputText_->append(
        QString("Cell Threads    : %1").arg(grid_.cellThreadCount())
    );
    outputText_->append(
        QString("Face Threads    : %1").arg(grid_.faceThreadCount())
    );
    outputText_->append(
        QString("Node Threads    : %1").arg(grid_.nodeThreadCount())
    );
    outputText_->append("====================================");

}

void Gui2DWindow::handleMeshClick(const QPointF& scenePos)
{
    // Route clicks to geometry drawing
    if (selectionMode_ == SelectionMode::DrawLine || selectionMode_ == SelectionMode::DrawPolyline ||
        selectionMode_ == SelectionMode::DrawEdge || selectionMode_ == SelectionMode::DrawVertex)
    {
        handleDrawingClick(scenePos);
        return;
    }

    if (selectionMode_ == SelectionMode::GeometryVertex || selectionMode_ == SelectionMode::GeometryEdge ||
        selectionMode_ == SelectionMode::GeometryPolyline)
    {
        handleGeometrySelection(scenePos);
        return;
    }
    if (selectionMode_ == SelectionMode::None)
        return;

    if (!boundingBoxItem_ || nx_ <= 0 || ny_ <= 0)
        return;

    // Bounding box coordinates in scene space
    const QRectF box = boundingBoxItem_->sceneBoundingRect();

    // Mouse position relative to the bounding box
    const double x = scenePos.x() - box.left();
    const double y = scenePos.y() - box.top();

    // Cell dimensions
    const double dx = box.width() / nx_;
    const double dy = box.height() / ny_;

    // Ignore clicks outside the mesh
    if (x < 0.0 || y < 0.0 ||
        x >= box.width() || y >= box.height())
        return;

    if (selectionMode_ == SelectionMode::Cell)
    {
        // Determine Cartesian indices
        const int i = static_cast<int>(x / dx);
        const int j = static_cast<int>(y / dy);

        // Retrieve actual mesh cell
        geom::Cell* cell = grid_.cellAt(i, j);

        if (!cell)
            return;

        // NEW: If this cell is already selected, deselect it
        if (selectedCell_ == cell)
        {
            clearSelectionHighlight();
            outputText_->append("Cell deselected.");
            return;
        }

        // Remove previous selection highlight
        clearSelectionHighlight();

        // NEW: Remember the newly selected cell
        selectedCell_ = cell;

        // Calculate selected cell rectangle
        QRectF cellRect(
            box.left() + i * dx,
            box.top() + j * dy,
            dx,
            dy);

        // Blue highlight
        QPen pen(QColor(0, 120, 255));
        pen.setWidth(2);
        pen.setCosmetic(true);

        selectedCellItem_ = scene_->addRect(
            cellRect,
            pen,
            QBrush(QColor(0, 120, 255, 65)));

        selectedCellItem_->setZValue(100);

        // Print cell information
        outputText_->append(
            QString(
                "Selected Cell\n"
                "  ID: %1\n"
                "  Index: (%2, %3)\n"
                "  Faces: %4\n"
                "  Flags: %5\n")
                .arg(cell->id)
                .arg(i)
                .arg(j)
                .arg(cell->faces.size())
                .arg(cell->flags));
    }
    else if (selectionMode_ == SelectionMode::Face)
    {
        // Nearest vertical face indices
        const int vi = std::clamp(
            static_cast<int>(std::round(x / dx)), 0, nx_);

        const int vj = std::clamp(
            static_cast<int>(y / dy), 0, ny_ - 1);

        // Nearest horizontal face indices
        const int hi = std::clamp(
            static_cast<int>(x / dx), 0, nx_ - 1);

        const int hj = std::clamp(
            static_cast<int>(std::round(y / dy)), 0, ny_);

        // Distance to the nearest vertical and horizontal grid lines
        const double distV = std::abs(x - vi * dx);
        const double distH = std::abs(y - hj * dy);

        // Convert 7 screen pixels to scene-space tolerance
        const QPointF p0 = view_->mapToScene(QPoint(0, 0));
        const QPointF px = view_->mapToScene(QPoint(7, 0));
        const QPointF py = view_->mapToScene(QPoint(0, 7));

        const double tolX = std::abs(px.x() - p0.x());
        const double tolY = std::abs(py.y() - p0.y());

        // Normalised distances
        const double normV = distV / std::max(tolX, 1e-12);
        const double normH = distH / std::max(tolY, 1e-12);

        geom::Face* face = nullptr;
        QLineF faceLine;

        // Pick vertical face if it is closer
        if (normV <= normH && normV <= 1.0)
        {
            face = grid_.verticalFaceAt(vi, vj);

            const double fx = box.left() + vi * dx;
            const double fy = box.top() + vj * dy;

            faceLine = QLineF(fx, fy, fx, fy + dy);
        }
        // Otherwise try horizontal face
        else if (normH <= 1.0)
        {
            face = grid_.horizontalFaceAt(hi, hj);

            const double fx = box.left() + hi * dx;
            const double fy = box.top() + hj * dy;

            faceLine = QLineF(fx, fy, fx + dx, fy);
        }

        // Click wasn't close enough to a face
        if (!face)
        {
            clearSelectionHighlight();
            return;
        }

        // Clicking the same face again deselects it
        if (selectedFace_ == face)
        {
            clearSelectionHighlight();
            outputText_->append("Face deselected.");
            return;
        }

        // Remove previous selection
        clearSelectionHighlight();

        // Remember selected mesh face
        selectedFace_ = face;

        // Draw red highlight
        QPen pen(QColor(255, 70, 40));
        pen.setWidth(4);
        pen.setCosmetic(true);

        selectedFaceItem_ = scene_->addLine(faceLine, pen);
        selectedFaceItem_->setZValue(101);

        // Display face information
        outputText_->append(
            QString(
                "Selected Face\n"
                "  ID: %1\n"
                "  Owner Cell: %2\n"
                "  Neighbour Cell: %3\n"
                "  Flags: %4\n")
                .arg(face->id)
                .arg(face->owner
                    ? QString::number(face->owner->id)
                    : "None")
                .arg(face->neighbor
                    ? QString::number(face->neighbor->id)
                    : "Boundary")
                .arg(face->flags)
        );
    }
    else if (selectionMode_ == SelectionMode::Node)
    {
        // ----------------------------------------
        // 1. Find nearest Cartesian node
        // ----------------------------------------

        const int ni = std::clamp(
            static_cast<int>(std::round(x / dx)), 0, nx_);

        const int nj = std::clamp(
            static_cast<int>(std::round(y / dy)), 0, ny_);

        // ----------------------------------------
        // 2. Calculate node position in scene
        // ----------------------------------------

        const double nodeX = box.left() + ni * dx;
        const double nodeY = box.top() + nj * dy;

        // ----------------------------------------
        // 3. Check click distance in screen pixels
        // ----------------------------------------

        const QPointF nodeScreenPos =
            view_->mapFromScene(QPointF(nodeX, nodeY));

        const QPointF clickScreenPos =
            view_->mapFromScene(scenePos);

        const double distance =
            QLineF(nodeScreenPos, clickScreenPos).length();

        constexpr double tolerance = 8.0;

        if (distance > tolerance)
        {
            clearSelectionHighlight();
            return;
        }

        // ----------------------------------------
        // 4. Retrieve actual mesh node
        // ----------------------------------------

        geom::Node* node = grid_.nodeAt(ni, nj);

        if (!node)
            return;

        // ----------------------------------------
        // 5. Deselect if already selected
        // ----------------------------------------

        if (selectedNode_ == node)
        {
            clearSelectionHighlight();
            outputText_->append("Node deselected.");
            return;
        }

        // ----------------------------------------
        // 6. Clear previous selection
        // ----------------------------------------

        clearSelectionHighlight();

        selectedNode_ = node;

        // ----------------------------------------
        // 7. Highlight selected node
        // ----------------------------------------

        QPen pen(QColor(0, 180, 80));
        pen.setWidth(2);
        pen.setCosmetic(true);

        // Create ellipse centred on local origin (0, 0)
        selectedNodeItem_ = scene_->addEllipse(
            -5.0,
            -5.0,
            10.0,
            10.0,
            pen,
            QBrush(QColor(0, 220, 100)));

        // Position the ellipse at the exact node location
        selectedNodeItem_->setPos(nodeX, nodeY);

        selectedNodeItem_->setFlag(
            QGraphicsItem::ItemIgnoresTransformations, true);

        selectedNodeItem_->setZValue(102);

        // ----------------------------------------
        // 8. Print node information
        // ----------------------------------------

        outputText_->append(
            QString(
                "Selected Node\n"
                "  ID: %1\n"
                "  Index: (%2, %3)\n"
                "  Coordinates: (%4, %5)\n"
                "  Incident Faces: %6\n"
                "  Thread ID: %7\n"
                "  Flags: %8\n")
                .arg(node->id)
                .arg(ni)
                .arg(nj)
                .arg(node->x)
                .arg(node->y)
                .arg(node->faces.size())
                .arg(node->thread
                    ? QString::number(node->thread->id)
                    : "None")
                .arg(node->flags)
        );
    }

}

void Gui2DWindow::fitMeshView()
{
    if (!boundingBoxItem_)
        return;

    view_->fitInView(
        scene_->sceneRect(),
        Qt::KeepAspectRatio
    );
}

void Gui2DWindow::clearSelectionHighlight()
{
     // Remove selected cell highlight
    if (selectedCellItem_)
    {
        scene_->removeItem(selectedCellItem_);
        delete selectedCellItem_;
        selectedCellItem_ = nullptr;
    }

    // Remove selected face highlight
    if (selectedFaceItem_)
    {
        scene_->removeItem(selectedFaceItem_);
        delete selectedFaceItem_;
        selectedFaceItem_ = nullptr;
    }

    if (selectedNodeItem_)
    {
        scene_->removeItem(selectedNodeItem_);
        delete selectedNodeItem_;
        selectedNodeItem_ = nullptr;
    }

    // NEW: Forget which mesh cell was selected
    selectedCell_ = nullptr;
    selectedFace_ = nullptr;
    selectedNode_ = nullptr;
}


void Gui2DWindow::handleDrawingClick(
    const QPointF& scenePos)
{
    if (selectionMode_ != SelectionMode::DrawLine && selectionMode_ != SelectionMode::DrawPolyline &&
        selectionMode_ != SelectionMode::DrawEdge && selectionMode_ != SelectionMode::DrawVertex)
        return;

    if (!boundingBoxItem_)
        return;

    const QRectF box =
    boundingBoxItem_->mapRectToScene(boundingBoxItem_->rect());

    if (!box.contains(scenePos))
        return;

    // Standalone vertex: intentionally not added to a polyline.
    if (selectionMode_ == SelectionMode::DrawVertex)
    {
        const auto id = geometry_.addVertex(scenePos.x(), scenePos.y());
        redrawGeometry();
        outputText_->append(QString("Created vertex %1").arg(id));
        return;
    }
    // Start a new polyline
    if (!activePolyline_)
    {
        activePolyline_ = geometry_.createPolyline();
    }

    // Avoid creating a zero-length edge
    if (activePolyline_)
    {
        const auto& polyline =
            geometry_.polylines().at(*activePolyline_);

        if (!polyline.vertices.empty())
        {
            const auto& last = geometry_.vertices().at(
                polyline.vertices.back());

            if (QLineF(QPointF(last.x, last.y), scenePos)
                    .length() < 1e-9)
            {
                return;
            }
        }
    }

    // Add a vertex to the geometry data model
    const auto vertexID = geometry_.addVertex(
        scenePos.x(), scenePos.y());

    geometry_.appendVertexToPolyline(
        *activePolyline_, vertexID);

    const auto& polyline =
        geometry_.polylines().at(*activePolyline_);

    // Draw the vertex
    QPen vertexPen(QColor(30, 100, 220));
    vertexPen.setWidth(1);
    vertexPen.setCosmetic(true);

    auto* vertexItem = scene_->addEllipse(
        -4.0, -4.0, 8.0, 8.0,
        vertexPen,
        QBrush(QColor(30, 100, 220)));

    vertexItem->setPos(scenePos);
    vertexItem->setFlag(
        QGraphicsItem::ItemIgnoresTransformations,
        true);

    vertexItem->setZValue(110);

    activeDrawingItems_.push_back(vertexItem);
    geometryItems_.push_back(vertexItem);

    // Draw the connecting edge
    if (polyline.vertices.size() >= 2)
    {
        const auto previousID =
            polyline.vertices[polyline.vertices.size() - 2];

        const auto& previous =
            geometry_.vertices().at(previousID);

        QPen edgePen(QColor(220, 40, 40));
        edgePen.setWidth(2);
        edgePen.setCosmetic(true);

        auto* edgeItem = scene_->addLine(
            previous.x, previous.y,
            scenePos.x(), scenePos.y(),
            edgePen);

        edgeItem->setZValue(105);

        activeDrawingItems_.push_back(edgeItem);
        geometryItems_.push_back(edgeItem);
    }

    if (selectionMode_ == SelectionMode::DrawEdge && polyline.vertices.size() == 2)
        finishDrawing();
    outputText_->append(
        QString("Geometry vertex %1 added at (%2, %3)")
            .arg(vertexID)
            .arg(scenePos.x())
            .arg(scenePos.y()));
}


void Gui2DWindow::handleDrawingMove(
    const QPointF& scenePos)
{
    if ((selectionMode_ != SelectionMode::DrawLine && selectionMode_ != SelectionMode::DrawPolyline && selectionMode_ != SelectionMode::DrawEdge) ||
        !activePolyline_)
    {
        clearPreviewLine();
        return;
    }

    const auto& polyline =
        geometry_.polylines().at(*activePolyline_);

    if (polyline.vertices.empty())
        return;

    const auto lastID = polyline.vertices.back();
    const auto& last = geometry_.vertices().at(lastID);

    QPen previewPen(QColor(255, 160, 0));
    previewPen.setWidth(2);
    previewPen.setCosmetic(true);
    previewPen.setStyle(Qt::DashLine);

    if (!previewLineItem_)
    {
        previewLineItem_ = scene_->addLine(
            last.x, last.y,
            scenePos.x(), scenePos.y(),
            previewPen);

        previewLineItem_->setZValue(108);
    }
    else
    {
        previewLineItem_->setLine(
            last.x, last.y,
            scenePos.x(), scenePos.y());
    }
}

void Gui2DWindow::clearPreviewLine()
{
    if (previewLineItem_)
    {
        scene_->removeItem(previewLineItem_);
        delete previewLineItem_;
        previewLineItem_ = nullptr;
    }
}

void Gui2DWindow::finishDrawing()
{
    if (!activePolyline_)
        return;

    clearPreviewLine();

    const auto polylineID = *activePolyline_;
    const auto& polyline =
        geometry_.polylines().at(polylineID);

    outputText_->append(
        QString("Polyline %1 completed: %2 vertices, %3 edges.")
            .arg(polylineID)
            .arg(polyline.vertices.size())
            .arg(polyline.edges.size()));

    if (polylineGraphics_.size() <= polylineID)
        polylineGraphics_.resize(
            static_cast<std::size_t>(polylineID) + 1);
    polylineGraphics_[polylineID] = std::move(activeDrawingItems_);

    activePolyline_.reset();
    activeDrawingItems_.clear();
    refreshGeometryTree();
}

void Gui2DWindow::clearGeometryGraphics()
{
    clearPreviewLine();
    clearGeometryPartHighlight();
    for (QGraphicsItem* item : geometryItems_)
    {
        scene_->removeItem(item);
        delete item;
    }

    geometryItems_.clear();
    polylineGraphics_.clear();
    activeDrawingItems_.clear();
    activePolyline_.reset();

    refreshGeometryTree();
}

void Gui2DWindow::cancelDrawing()
{
    if (!activePolyline_)
        return;

    clearPreviewLine();

    // Remove graphics belonging to the active polyline
    for (QGraphicsItem* item : activeDrawingItems_)
    {
        scene_->removeItem(item);

        // Remove from the complete graphics list
        auto it = std::find(
            geometryItems_.begin(),
            geometryItems_.end(),
            item);

        if (it != geometryItems_.end())
            geometryItems_.erase(it);

        delete item;
    }

    activeDrawingItems_.clear();

    // Roll back the underlying geometry
    geometry_.discardLastPolyline();

    activePolyline_.reset();

    outputText_->append("Unfinished polyline cancelled.");
}


void Gui2DWindow::refreshGeometryTree()
{
    if (!geometryRootNode_)
        return;

    // Remove previous displayed children.
    while (geometryRootNode_->childCount() > 0)
    {
        delete geometryRootNode_->takeChild(0);
    }

    unassignedNode_ =
        new QTreeWidgetItem(geometryRootNode_);

    unassignedNode_->setText(0, "Unassigned");

    // Create tree nodes for each geometry thread.
    for (const auto& thread : geometry_.threads())
    {
        auto* groupNode =
            new QTreeWidgetItem(geometryRootNode_);

        groupNode->setText(
            0, QString::fromStdString(thread.name));

        groupNode->setData(
            0, Qt::UserRole, QString("group"));

        groupNode->setData(
            0, Qt::UserRole + 1,
            QVariant::fromValue(thread.id));

        // Add the thread's polylines.
        for (const auto polylineID : thread.polylines)
        {
            const auto& polyline =
                geometry_.polylines().at(polylineID);

            if (polyline.deleted)
                continue;

            auto* partNode =
                new QTreeWidgetItem(groupNode);

            partNode->setText(
                0, QString::fromStdString(polyline.name));

            partNode->setData(
                0, Qt::UserRole, QString("polyline"));

            partNode->setData(
                0, Qt::UserRole + 1,
                QVariant::fromValue(polylineID));
        }

        groupNode->setExpanded(true);
    }

    // Add all unassigned polylines.
    for (const auto& polyline : geometry_.polylines())
    {
        if (polyline.deleted)
            continue;

        if (polyline.threadID)
            continue;

        // Do not show an unfinished drawing.
        if (activePolyline_ &&
            *activePolyline_ == polyline.id)
            continue;

        auto* partNode =
            new QTreeWidgetItem(unassignedNode_);

        partNode->setText(
            0, QString::fromStdString(polyline.name));

        partNode->setData(
            0, Qt::UserRole, QString("polyline"));

        partNode->setData(
            0, Qt::UserRole + 1,
            QVariant::fromValue(polyline.id));
    }

    geometryRootNode_->setExpanded(true);
    unassignedNode_->setExpanded(true);
}


void Gui2DWindow::createGeometryThread()
{
    bool ok = false;

    const QString defaultName =
        QString("Geom_Group_%1")
            .arg(geometry_.threads().size());

    const QString name = QInputDialog::getText(
        this,
        "Create Geometry Group",
        "Group name:",
        QLineEdit::Normal,
        defaultName,
        &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    const auto id =
        geometry_.createThread(name.trimmed().toStdString());

    refreshGeometryTree();

    outputText_->append(
        QString("Created geometry thread %1: %2")
            .arg(id)
            .arg(name));
}


void Gui2DWindow::assignSelectedPolyline()
{
    auto* item = workflowTree_->currentItem();

    if (!item ||
        item->data(0, Qt::UserRole).toString() != "polyline")
        return;

    const auto polylineID =
        item->data(0, Qt::UserRole + 1)
            .value<geom::PolylineID>();

    QStringList names;
    names << "Unassigned";

    for (const auto& thread : geometry_.threads())
    {
        names << QString("%1 (Thread %2)")
                     .arg(QString::fromStdString(thread.name))
                     .arg(thread.id);
    }

    bool ok = false;

    const QString chosen = QInputDialog::getItem(
        this,
        "Assign Geometry",
        "Target group:",
        names,
        0,
        false,
        &ok);

    if (!ok)
        return;

    std::optional<geom::GeometryThreadID> target;

    const int index = names.indexOf(chosen);

    if (index < 0)
        return;

    if (index > 0)
    {
        target = geometry_.threads().at(
            static_cast<std::size_t>(index - 1)).id;
    }

    if (geometry_.assignPolylineToThread(polylineID, target))
    {
        refreshGeometryTree();

        outputText_->append(
            QString("Polyline %1 assigned to %2")
                .arg(polylineID)
                .arg(chosen));
    }
}

void Gui2DWindow::renameSelectedGeometry()
{
    auto* item = workflowTree_->currentItem();

    if (!item)
        return;

    const QString kind =
        item->data(0, Qt::UserRole).toString();

    if (kind != "group" && kind != "polyline")
        return;

    bool ok = false;

    const QString name = QInputDialog::getText(
        this,
        "Rename Geometry",
        "New name:",
        QLineEdit::Normal,
        item->text(0),
        &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    const auto id =
        item->data(0, Qt::UserRole + 1)
            .value<std::uint32_t>();

    if (kind == "group")
    {
        geometry_.renameThread(
            id, name.trimmed().toStdString());
    }
    else
    {
        geometry_.renamePolyline(
            id, name.trimmed().toStdString());
    }

    refreshGeometryTree();
}

void Gui2DWindow::showGeometryContextMenu(
    const QPoint& pos)
{
    QTreeWidgetItem* item =
        workflowTree_->itemAt(pos);

    if (!item)
        return;

    QMenu menu(this);

    if (item == geometryRootNode_)
    {
        QAction* create =
            menu.addAction("New Geometry Group");

        connect(create, &QAction::triggered,
                this, &Gui2DWindow::createGeometryThread);
    }
    else if (item == unassignedNode_)
    {
        menu.addAction(
            "New Geometry Group",
            this,
            &Gui2DWindow::createGeometryThread);
    }
    else
    {
        const QString kind =
            item->data(0, Qt::UserRole).toString();

        if (kind == "polyline")
        {
            menu.addAction(
                "Assign to Group",
                this,
                &Gui2DWindow::assignSelectedPolyline);

            menu.addAction(
                "Rename",
                this,
                &Gui2DWindow::renameSelectedGeometry);

            menu.addSeparator();

            // Connect to deleteSelectedGeometry()
            // after implementing safe model deletion.
            menu.addAction("Delete", this,
                            &Gui2DWindow::deleteSelectedGeometry);
        }
        else if (kind == "group")
        {
            menu.addAction(
                "Rename Group",
                this,
                &Gui2DWindow::renameSelectedGeometry);
            menu.addSeparator();
            menu.addAction(
                "Delete Group",
                this,
                &Gui2DWindow::deleteSelectedGeometry);
        }
        else
        {
            return;
        }
    }

    workflowTree_->setCurrentItem(item);

    menu.exec(
        workflowTree_->viewport()->mapToGlobal(pos));
}


void Gui2DWindow::clearGeometryPartHighlight()
{
    for (QGraphicsItem* item : geometryHighlightItems_)
    {
        scene_->removeItem(item);
        delete item;
    }

    geometryHighlightItems_.clear();
}

void Gui2DWindow::highlightGeometryPart(
    geom::PolylineID id)
{
    clearGeometryPartHighlight();

    if (id >= geometry_.polylines().size())
        return;

    const auto& polyline =
        geometry_.polylines().at(id);

    QPen edgePen(QColor(255, 210, 0));
    edgePen.setWidth(5);
    edgePen.setCosmetic(true);

    QPen vertexPen(QColor(255, 210, 0));
    vertexPen.setWidth(2);
    vertexPen.setCosmetic(true);

    for (const auto edgeID : polyline.edges)
    {
        const auto& edge =
            geometry_.edges().at(edgeID);

        const auto& a =
            geometry_.vertices().at(edge.vertex0);

        const auto& b =
            geometry_.vertices().at(edge.vertex1);

        auto* line = scene_->addLine(
            a.x, a.y, b.x, b.y, edgePen);

        line->setZValue(200);

        geometryHighlightItems_.push_back(line);
    }

    for (const auto vertexID : polyline.vertices)
    {
        const auto& vertex =
            geometry_.vertices().at(vertexID);

        auto* marker = scene_->addEllipse(
            -5, -5, 10, 10,
            vertexPen,
            QBrush(QColor(255, 210, 0)));

        marker->setPos(vertex.x, vertex.y);

        marker->setFlag(
            QGraphicsItem::ItemIgnoresTransformations,
            true);

        marker->setZValue(201);

        geometryHighlightItems_.push_back(marker);
    }
}


void Gui2DWindow::deleteSelectedGeometry()
{
    auto* item = workflowTree_->currentItem();

    if (!item)
        return;

    const QString kind =
        item->data(0, Qt::UserRole).toString();

    const auto id =
        item->data(0, Qt::UserRole + 1)
            .value<std::uint32_t>();

    if (kind == "polyline")
    {
        const auto answer = QMessageBox::question(
            this,
            "Delete Polyline",
            "Delete this polyline and its geometry?",
            QMessageBox::Yes | QMessageBox::No);

        if (answer != QMessageBox::Yes)
            return;

        if (!geometry_.deletePolyline(id))
            return;

        clearGeometryPartHighlight();

        // Delete only the graphics of this polyline.
        if (id < polylineGraphics_.size())
        {
            for (auto* graphic : polylineGraphics_[id])
            {
                auto it = std::find(
                    geometryItems_.begin(),
                    geometryItems_.end(),
                    graphic);

                if (it != geometryItems_.end())
                    geometryItems_.erase(it);

                scene_->removeItem(graphic);
                delete graphic;
            }

            polylineGraphics_[id].clear();
        }

        outputText_->append(
            QString("Deleted polyline %1").arg(id));
    }
    else if (kind == "group")
    {
        const auto answer = QMessageBox::question(
            this,
            "Delete Geometry Group",
            "Delete this group? Its polylines will "
            "be moved to Unassigned.",
            QMessageBox::Yes | QMessageBox::No);

        if (answer != QMessageBox::Yes)
            return;

        if (!geometry_.deleteThread(id))
            return;

        clearGeometryPartHighlight();

        outputText_->append(
            QString("Deleted geometry group %1").arg(id));
    }
    else
    {
        return;
    }

    refreshGeometryTree();
}


QPointF Gui2DWindow::nearestMeshNode(const QPointF& point) const
{
    if (grid_.nodeCount() == 0 || nx_ < 0 || ny_ < 0)
        return point;
    double best = std::numeric_limits<double>::infinity();
    QPointF result = point;
    for (int j = 0; j <= ny_; ++j)
        for (int i = 0; i <= nx_; ++i)
        {
            const auto* n = grid_.nodeAt(static_cast<std::uint32_t>(i), static_cast<std::uint32_t>(j));
            if (!n) continue;
            const double dx = n->x - point.x(), dy = n->y - point.y();
            const double d2 = dx*dx + dy*dy;
            if (d2 < best) { best = d2; result = QPointF(n->x, n->y); }
        }
    return result;
}

void Gui2DWindow::applySnap()
{
    if (!grid_.nodeCount()) { outputText_->append("Snap: generate a mesh first."); return; }
    std::vector<geom::GeometryVertexID> ids;
    if (snapAllAction_->isChecked())
    {
        for (const auto& v : geometry_.vertices()) if (!v.deleted) ids.push_back(v.id);
    }
    else if (selectedGeometryVertex_)
        ids.push_back(*selectedGeometryVertex_);
    else if (selectedGeometryEdge_ && geometry_.isEdgeActive(*selectedGeometryEdge_))
    {
        const auto& e = geometry_.edges().at(*selectedGeometryEdge_);
        ids = {e.vertex0, e.vertex1};
    }
    else if (selectedGeometryPolyline_ && *selectedGeometryPolyline_ < geometry_.polylines().size())
        ids = geometry_.polylines().at(*selectedGeometryPolyline_).vertices;
    else
    {
        outputText_->append("Snap: select a geometry vertex, edge or polyline, or enable Snap All.");
        return;
    }
    std::vector<std::pair<geom::GeometryVertexID, QPointF>> moves;
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    for (auto id : ids)
    {
        if (!geometry_.isVertexActive(id)) continue;
        const auto& v = geometry_.vertices().at(id);
        moves.emplace_back(id, nearestMeshNode(QPointF(v.x, v.y)));
    }
    // Reject a snap that collapses any active edge; do not partially move geometry.
    auto proposed = [&moves, this](geom::GeometryVertexID id) {
        for (const auto& m : moves) if (m.first == id) return m.second;
        const auto& v = geometry_.vertices().at(id);
        return QPointF(v.x, v.y);
    };
    for (const auto& e : geometry_.edges())
    {
        if (e.deleted) continue;
        const QPointF a = proposed(e.vertex0), b = proposed(e.vertex1);
        if (QLineF(a, b).length() < 1e-9)
        {
            outputText_->append(QString("Snap cancelled: edge %1 would collapse.").arg(e.id));
            return;
        }
    }
    for (const auto& m : moves) geometry_.moveVertex(m.first, m.second.x(), m.second.y());
    redrawGeometry();
    outputText_->append(QString("Snapped %1 geometry vertices to nearest mesh nodes.").arg(moves.size()));
}

void Gui2DWindow::closeActivePolyline()
{
    const auto target = activePolyline_ ? activePolyline_ : selectedGeometryPolyline_;
    if (!target) { outputText_->append("No active or selected polyline."); return; }
    const auto id = *target;
    if (id >= geometry_.polylines().size() || geometry_.polylines()[id].deleted)
    { outputText_->append("Polyline no longer exists."); return; }
    const auto& poly = geometry_.polylines()[id];
    if (poly.closed) { outputText_->append("Polyline is already closed."); return; }
    if (newEdgeAction_->isChecked())
    {
        if (poly.vertices.size() < 3)
        { outputText_->append("Closure needs at least three vertices."); return; }
        const auto& a = geometry_.vertices().at(poly.vertices.front());
        const auto& b = geometry_.vertices().at(poly.vertices.back());
        if (QLineF(QPointF(a.x,a.y), QPointF(b.x,b.y)).length() < 1e-9)
        { outputText_->append("Cannot close: endpoints coincide."); return; }
        geometry_.closePolyline(id);
    }
    else if (!geometry_.closePolylineReusingLastEdge(id))
    {
        outputText_->append("Cannot close without a new edge: need at least four vertices and an unshared final edge.");
        return;
    }
    if (activePolyline_) finishDrawing();
    redrawGeometry();
    refreshGeometryTree();
    outputText_->append(QString("Closed polyline %1.").arg(id));
}

void Gui2DWindow::redrawGeometry()
{
    clearPreviewLine();
    clearGeometryPartHighlight();
    for (auto* item : geometryItems_) { scene_->removeItem(item); delete item; }
    geometryItems_.clear();
    polylineGraphics_.clear();
    activeDrawingItems_.clear();
    QPen edgePen(QColor(220,40,40)); edgePen.setCosmetic(true); edgePen.setWidth(2);
    QPen vertexPen(QColor(30,100,220)); vertexPen.setCosmetic(true);
    auto drawVertex = [this, &vertexPen](geom::GeometryVertexID id) {
        if (!geometry_.isVertexActive(id)) return static_cast<QGraphicsItem*>(nullptr);
        const auto& v = geometry_.vertices().at(id);
        auto* item = scene_->addEllipse(-4,-4,8,8,vertexPen,QBrush(QColor(30,100,220)));
        item->setPos(v.x,v.y);
        item->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
        item->setZValue(110);
        geometryItems_.push_back(item);
        return static_cast<QGraphicsItem*>(item);
    };
    QSet<geom::GeometryVertexID> drawn;
    polylineGraphics_.resize(geometry_.polylines().size());
    for (const auto& poly : geometry_.polylines())
    {
        if (poly.deleted) continue;
        for (auto id : poly.edges)
        {
            if (!geometry_.isEdgeActive(id)) continue;
            const auto& e = geometry_.edges().at(id);
            const auto& a = geometry_.vertices().at(e.vertex0);
            const auto& b = geometry_.vertices().at(e.vertex1);
            auto* line = scene_->addLine(a.x,a.y,b.x,b.y,edgePen);
            line->setZValue(105);
            geometryItems_.push_back(line);
            polylineGraphics_[poly.id].push_back(line);
        }
        for (auto id : poly.vertices)
        {
            if (drawn.contains(id)) continue;
            drawn.insert(id);
            auto* item = drawVertex(id);
            if (item) polylineGraphics_[poly.id].push_back(item);
        }
    }
    for (const auto& v : geometry_.vertices())
        if (!v.deleted && !drawn.contains(v.id)) drawVertex(v.id);
    if (selectedGeometryPolyline_ && *selectedGeometryPolyline_ < geometry_.polylines().size() &&
        !geometry_.polylines()[*selectedGeometryPolyline_].deleted)
        highlightGeometryPart(*selectedGeometryPolyline_);
    refreshGeometryTree();
}

void Gui2DWindow::handleGeometrySelection(const QPointF& pos)
{
    selectedGeometryVertex_.reset(); selectedGeometryEdge_.reset(); selectedGeometryPolyline_.reset();
    clearGeometryPartHighlight();
    double best = 12.0;
    const auto screen = view_->mapFromScene(pos);
    if (selectionMode_ == SelectionMode::GeometryVertex)
    {
        for (const auto& v : geometry_.vertices())
        {
            if (v.deleted) continue;
            const double d = QLineF(QPointF(screen), QPointF(view_->mapFromScene(QPointF(v.x,v.y)))).length();
            if (d < best) { best = d; selectedGeometryVertex_ = v.id; }
        }
    }
    else
    {
        for (const auto& e : geometry_.edges())
        {
            if (e.deleted) continue;
            const auto& a = geometry_.vertices().at(e.vertex0);
            const auto& b = geometry_.vertices().at(e.vertex1);
            const QPointF p = view_->mapFromScene(QPointF(a.x,a.y));
            const QPointF q = view_->mapFromScene(QPointF(b.x,b.y));
            const QPointF d = q-p;
            const double denom = QPointF::dotProduct(d,d);
            if (denom < 1e-12) continue;
            const double t = std::clamp(QPointF::dotProduct(QPointF(screen)-p,d)/denom,0.0,1.0);
            const double distance = QLineF(QPointF(screen),p+t*d).length();
            if (distance < best) { best = distance; selectedGeometryEdge_ = e.id; }
        }
        if (selectionMode_ == SelectionMode::GeometryPolyline && selectedGeometryEdge_)
        {
            const auto eid = *selectedGeometryEdge_;
            for (const auto& poly : geometry_.polylines())
                if (!poly.deleted && std::find(poly.edges.begin(),poly.edges.end(),eid)!=poly.edges.end())
                { selectedGeometryPolyline_ = poly.id; break; }
            selectedGeometryEdge_.reset();
        }
    }
    if (selectedGeometryPolyline_) highlightGeometryPart(*selectedGeometryPolyline_);
    else if (selectedGeometryVertex_)
    {
        const auto& v = geometry_.vertices().at(*selectedGeometryVertex_);
        QPen pen(Qt::yellow); pen.setWidth(2); pen.setCosmetic(true);
        auto* item = scene_->addEllipse(-6,-6,12,12,pen,QBrush(Qt::yellow));
        item->setPos(v.x,v.y); item->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
        item->setZValue(201); geometryHighlightItems_.push_back(item);
    }
    else if (selectedGeometryEdge_)
    {
        const auto& e = geometry_.edges().at(*selectedGeometryEdge_);
        const auto& a = geometry_.vertices().at(e.vertex0);
        const auto& b = geometry_.vertices().at(e.vertex1);
        QPen pen(Qt::yellow); pen.setWidth(5); pen.setCosmetic(true);
        auto* item = scene_->addLine(a.x,a.y,b.x,b.y,pen);
        item->setZValue(201); geometryHighlightItems_.push_back(item);
    }
    printSelectedInfo();
}

void Gui2DWindow::printSelectedInfo()
{
    if (selectedGeometryVertex_)
    {
        const auto& v = geometry_.vertices().at(*selectedGeometryVertex_);
        outputText_->append(QString("Geometry vertex %1: (%2, %3)").arg(v.id).arg(v.x).arg(v.y));
    }
    else if (selectedGeometryEdge_)
    {
        const auto& e = geometry_.edges().at(*selectedGeometryEdge_);
        const auto& a = geometry_.vertices().at(e.vertex0);
        const auto& b = geometry_.vertices().at(e.vertex1);
        outputText_->append(QString("Geometry edge %1: vertices %2-%3, length %4")
            .arg(e.id).arg(e.vertex0).arg(e.vertex1).arg(QLineF(QPointF(a.x,a.y),QPointF(b.x,b.y)).length()));
    }
    else if (selectedGeometryPolyline_)
    {
        const auto& p = geometry_.polylines().at(*selectedGeometryPolyline_);
        double length = 0;
        for (auto eid : p.edges) if (geometry_.isEdgeActive(eid))
        {
            const auto& e = geometry_.edges().at(eid);
            const auto& a = geometry_.vertices().at(e.vertex0);
            const auto& b = geometry_.vertices().at(e.vertex1);
            length += QLineF(QPointF(a.x,a.y),QPointF(b.x,b.y)).length();
        }
        outputText_->append(QString("Polyline %1: %2 vertices, %3 edges, length %4, closed=%5, group=%6")
            .arg(p.id).arg(p.vertices.size()).arg(p.edges.size()).arg(length)
            .arg(p.closed ? "yes":"no").arg(p.threadID ? QString::number(*p.threadID):"unassigned"));
    }
    else if (selectedNode_)
        outputText_->append(QString("Mesh node %1: (%2,%3), faces=%4, flags=%5")
            .arg(selectedNode_->id).arg(selectedNode_->x).arg(selectedNode_->y)
            .arg(selectedNode_->faces.size()).arg(selectedNode_->flags));
    else if (selectedFace_)
        outputText_->append(QString("Mesh face %1: nodes %2-%3, owner=%4, neighbor=%5, flags=%6")
            .arg(selectedFace_->id).arg(selectedFace_->inode ? QString::number(selectedFace_->inode->id):"none")
            .arg(selectedFace_->jnode ? QString::number(selectedFace_->jnode->id):"none")
            .arg(selectedFace_->owner ? QString::number(selectedFace_->owner->id):"none")
            .arg(selectedFace_->neighbor ? QString::number(selectedFace_->neighbor->id):"none")
            .arg(selectedFace_->flags));
    else if (selectedCell_)
        outputText_->append(QString("Mesh cell %1: %2 faces, %3 nodes, flags=%4")
            .arg(selectedCell_->id).arg(selectedCell_->faces.size()).arg(selectedCell_->nodes.size()).arg(selectedCell_->flags));
    else
        outputText_->append("Info: no selected mesh or geometry entity.");
}

void Gui2DWindow::deleteSelectedEntity()
{
    bool ok = false;
    if (selectedGeometryVertex_) ok = geometry_.deleteVertex(*selectedGeometryVertex_);
    else if (selectedGeometryEdge_) ok = geometry_.deleteEdge(*selectedGeometryEdge_);
    else if (selectedGeometryPolyline_) ok = geometry_.deletePolyline(*selectedGeometryPolyline_);
    if (!ok) { outputText_->append("Delete: select a geometry entity first."); return; }
    selectedGeometryVertex_.reset(); selectedGeometryEdge_.reset(); selectedGeometryPolyline_.reset();
    redrawGeometry();
    const auto errors = geometry_.validate();
    outputText_->append(QString("Geometry edited. Validation issues: %1").arg(errors.size()));
    for (const auto& error : errors) outputText_->append(QString::fromStdString(error));
}
