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
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>
#include <QColorDialog>
#include <QMessageBox>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <limits>

#include <QMenu>
#include <QMenuBar>
#include <QKeySequence>
#include <stdexcept>
#include <QFileDialog>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QSignalBlocker>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QSettings>

Gui2DWindow::Gui2DWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("EvolBeing - GUI2D");
    resize(1100, 750);

    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);
    auto* leftPanel = new QWidget(central);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);

    setupSimulationView(leftPanel);
    applySimulationTheme();
    setupMenus();
    auto* meshToolbar = createMeshToolbar(leftPanel);
    auto* geometryToolbar = createGeometryToolbar(leftPanel);
    configureToolModes();
    auto* analyzeToolbar = createAnalyzeToolbar(leftPanel);

    leftLayout->addWidget(meshToolbar);
    leftLayout->addWidget(geometryToolbar);
    leftLayout->addWidget(view_, 1);
    leftLayout->addWidget(analyzeToolbar);
    setupOutputPanel(leftPanel, leftLayout);
    mainLayout->addWidget(leftPanel, 1);

    setupWorkflowPanel(central, mainLayout);
    setCentralWidget(central);
}

void Gui2DWindow::applySimulationTheme()
{
    // Settings are stored per user; no separate namespace or global theme object.
    QSettings settings("EvolBeing", "EvolBeing_Gen_Alpha");
    const int stored = settings.value("simulation/theme", 0).toInt();
    if (stored >= 0 && stored <= 2)
        themePreset_ = static_cast<ThemePreset>(stored);
    updateSimulationTheme();
}

void Gui2DWindow::setSimulationTheme(ThemePreset preset)
{
    themePreset_ = preset;
    QSettings settings("EvolBeing", "EvolBeing_Gen_Alpha");
    settings.setValue("simulation/theme", static_cast<int>(preset));
    updateSimulationTheme();
}

void Gui2DWindow::updateSimulationTheme()
{
    switch (themePreset_) {
    case ThemePreset::SlateDark:
        theme_ = {QColor("#303A46"), QColor("#34584F"), QColor("#46515E"),
                  QColor("#796044"), QColor("#62717F"), QColor("#FFB36B"),
                  QColor("#00E5FF"), QColor("#D7E0E8")};
        break;
    case ThemePreset::DeepDark:
        theme_ = {QColor("#111820"), QColor("#193D36"), QColor("#25313D"),
                  QColor("#65502D"), QColor("#40505D"), QColor("#F0A35E"),
                  QColor("#00E5FF"), QColor("#D7E0E8")};
        break;
    case ThemePreset::LightGrey:
        theme_ = {QColor("#DCE2E8"), QColor("#B5D5C7"), QColor("#C6CFD8"),
                  QColor("#E3CC9E"), QColor("#9CAAB7"), QColor("#B35D20"),
                  QColor("#007C99"), QColor("#586774")};
        break;
    }

    if (scene_)
        scene_->setBackgroundBrush(theme_.background);
    if (view_) {
        view_->setBackgroundBrush(theme_.background);
        view_->viewport()->update();
    }
    if (gridItem_) {
        QPen pen = gridItem_->pen();
        pen.setColor(theme_.gridLines);
        gridItem_->setPen(pen);
    }
    if (boundingBoxItem_) {
        QPen pen = boundingBoxItem_->pen();
        pen.setColor(theme_.boundingBox);
        boundingBoxItem_->setPen(pen);
    }
}

void Gui2DWindow::setupSimulationView(QWidget* leftPanel)
{
    scene_ = new QGraphicsScene(this);

    // Background is applied by applySimulationTheme() in the constructor.

    // Initial empty scene
    scene_->setSceneRect(0, 0, 800, 600);


    // Graphics viewport
    view_ = new MeshGraphicsView(leftPanel);
    view_->setScene(scene_);

    auto* meshView = static_cast<MeshGraphicsView*>(view_);
    meshView->setClickHandler(
        [this](const QPointF& scenePos)
        {
            if (regionPickHandler_) {
                auto callback = std::move(regionPickHandler_);
                regionPickHandler_ = {};
                callback(scenePos);
                return;
            }
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

}

void Gui2DWindow::setupMenus()
{
    // Stage 4: independent single-row menu, mesh and geometry toolbars.
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Load Geometry...", this, &Gui2DWindow::loadGeometry);
    fileMenu->addAction("Save Geometry...", this, &Gui2DWindow::saveGeometry);
    fileMenu->addSeparator();
    auto* themeMenu = fileMenu->addMenu("Simulation Theme");
    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    for (const auto& option : {std::pair{QStringLiteral("Slate Dark (Default)"), ThemePreset::SlateDark},
                               std::pair{QStringLiteral("Deep Dark"), ThemePreset::DeepDark},
                               std::pair{QStringLiteral("Light Grey"), ThemePreset::LightGrey}}) {
        auto* action = themeMenu->addAction(option.first);
        action->setCheckable(true);
        action->setChecked(themePreset_ == option.second);
        themeGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, preset = option.second]() {
            setSimulationTheme(preset);
        });
    }

    auto* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction("Scale Geometry...", this, &Gui2DWindow::scaleGeometry);
    auto* undoAction = editMenu->addAction("Undo", this, &Gui2DWindow::undoGeometry);
    undoAction->setShortcut(QKeySequence::Undo);
    auto* redoAction = editMenu->addAction("Redo", this, &Gui2DWindow::redoGeometry);
    redoAction->setShortcut(QKeySequence::Redo);

}

QToolBar* Gui2DWindow::createMeshToolbar(QWidget* leftPanel)
{
    // Toolbar for mesh viewing and selection
    QToolBar* meshToolbar = new QToolBar(leftPanel);
    meshToolbar->setMovable(false);

    meshToolbar->setFloatable(false);
    meshToolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto* meshLabel = new QLabel("Mesh", this);
    QFont font = meshLabel->font();
    font.setBold(true);
    meshLabel->setFont(font);
    meshToolbar->addWidget(meshLabel);
    meshToolbar->addSeparator();

    meshToolbar->addAction("Generate Mesh")->setEnabled(false); // placeholder
    auto* showMeshAction = meshToolbar->addAction("Show Mesh");
    showMeshAction->setObjectName("showMeshVisibility");
    showMeshAction->setCheckable(true);
    showMeshAction->setChecked(true);
    connect(showMeshAction, &QAction::toggled, this, [this](bool visible) {
        if (gridItem_) gridItem_->setVisible(visible);
    });
    // Fit view button
    fitViewAction_ = meshToolbar->addAction("Fit View");
    meshToolbar->addSeparator();
    cellSelectAction_ = meshToolbar->addAction("Cell");
    faceSelectAction_ = meshToolbar->addAction("Face");
    nodeSelectAction_ = meshToolbar->addAction("Node");
    return meshToolbar;
}

QToolBar* Gui2DWindow::createGeometryToolbar(QWidget* leftPanel)
{
    QToolBar* geometryToolbar = new QToolBar(leftPanel);
    geometryToolbar->setMovable(false);
    geometryToolbar->setFloatable(false);
    geometryToolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    //title
    auto* geomLabel = new QLabel("Geom", this);
    QFont font = geomLabel->font();
    font.setBold(true);
    geomLabel->setFont(font);
    geometryToolbar->addWidget(geomLabel);
    geometryToolbar->addSeparator();

    // Geometry drawing button
    drawLineAction_ = geometryToolbar->addAction("Draw Line");
    drawPolylineAction_ = geometryToolbar->addAction("Polyline");
    drawEdgeAction_ = geometryToolbar->addAction("Draw Edge");
    drawVertexAction_ = geometryToolbar->addAction("Draw Vertex");
    geometryToolbar->addSeparator();
    selectGeometryVertexAction_ = geometryToolbar->addAction("Select Vertex");
    selectGeometryEdgeAction_ = geometryToolbar->addAction("Select Edge");
    selectGeometryPolylineAction_ = geometryToolbar->addAction("Select Polyline");
    geometryToolbar->addSeparator();
    connectNodesAction_ = geometryToolbar->addAction("Connect Nodes");
    auto* snapButton = new QToolButton(geometryToolbar);
    snapButton->setText("Snap");
    snapButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto* snapMenu = new QMenu(snapButton);
    snapAllAction_ = snapMenu->addAction("Snap All geometry vertices");
    snapAllAction_->setCheckable(true);
    snapButton->setMenu(snapMenu);
    geometryToolbar->addWidget(snapButton);
    connect(snapButton, &QToolButton::clicked, this, &Gui2DWindow::applySnap);
    auto* closeButton = new QToolButton(geometryToolbar);
    closeButton->setText("Close Polyline");
    closeButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto* closeMenu = new QMenu(closeButton);
    newEdgeAction_ = closeMenu->addAction("New Edge");
    newEdgeAction_->setCheckable(true);
    newEdgeAction_->setChecked(true);
    closeButton->setMenu(closeMenu);
    geometryToolbar->addWidget(closeButton);
    connect(closeButton, &QToolButton::clicked, this, &Gui2DWindow::closeActivePolyline);
    auto* deleteEntityAction = geometryToolbar->addAction("Delete Entity");
    connect(deleteEntityAction, &QAction::triggered, this, &Gui2DWindow::deleteSelectedEntity);

    finalizeGeometryAction_ = geometryToolbar->addAction("Finalize Geom");
    connect(finalizeGeometryAction_, &QAction::triggered,
            this, &Gui2DWindow::finalizeGeometryForWorkflow);
    return geometryToolbar;
}

QToolBar* Gui2DWindow::createAnalyzeToolbar(QWidget* leftPanel)
{
    auto* analyzeToolbar = new QToolBar(leftPanel);
    analyzeToolbar->setMovable(false);
    analyzeToolbar->setFloatable(false);
    analyzeToolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    analyzeToolbar->addWidget(new QLabel("Analyze  ", analyzeToolbar));
    analyzeToolbar->addSeparator();
    infoAction_ = analyzeToolbar->addAction("Info");
    connect(infoAction_, &QAction::triggered,
            this, &Gui2DWindow::printSelectedInfo);
    return analyzeToolbar;
}

void Gui2DWindow::configureToolModes()
{
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
    connectNodesAction_->setCheckable(true);
    selectionGroup->addAction(connectNodesAction_);
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
    connectTool(connectNodesAction_, SelectionMode::ConnectNodes);
    connectTool(drawLineAction_, SelectionMode::DrawLine);
    connectTool(drawPolylineAction_, SelectionMode::DrawPolyline);
    connectTool(drawEdgeAction_, SelectionMode::DrawEdge);
    connectTool(drawVertexAction_, SelectionMode::DrawVertex);
    connectTool(selectGeometryVertexAction_, SelectionMode::GeometryVertex);
    connectTool(selectGeometryEdgeAction_, SelectionMode::GeometryEdge);
    connectTool(selectGeometryPolylineAction_, SelectionMode::GeometryPolyline);

}

void Gui2DWindow::setupOutputPanel(QWidget* leftPanel, QVBoxLayout* leftLayout)
{
    outputText_ = new QTextEdit(leftPanel);
    outputText_->setReadOnly(true);
    outputText_->setFixedHeight(120);
    outputText_->setPlaceholderText("Output messages will appear here...");
    leftLayout->addWidget(outputText_);
}

void Gui2DWindow::setupWorkflowPanel(QWidget* central, QHBoxLayout* mainLayout)
{
    auto* panel = new QWidget(central);
    panel->setFixedWidth(240);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->addWidget(new QLabel("Workflow", panel));
    setupWorkflowTree(panel);
    panelLayout->addWidget(workflowTree_, 1);
    applyButton_ = new QPushButton("Apply", panel);
    panelLayout->addWidget(applyButton_);
    mainLayout->addWidget(panel);
    connect(applyButton_, &QPushButton::clicked,
            this, &Gui2DWindow::createBoundingBox);
}

void Gui2DWindow::setupWorkflowTree(QWidget* panel)
{
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


    // Compute Mesh workflow: region seeds are independent of geometry polygons.
    computeMeshNode_ = new QTreeWidgetItem(workflowTree_);
    computeMeshNode_->setText(0, "Compute Mesh");
    regionsNode_ = new QTreeWidgetItem(computeMeshNode_);
    regionsNode_->setText(0, "Regions");
    computeMeshNode_->setDisabled(true);
    computeMeshNode_->setExpanded(false);
    regionsNode_->setExpanded(true);

    connect(workflowTree_, &QTreeWidget::itemClicked,
            this, [this](QTreeWidgetItem* item, int) {
        if (!item || item->data(0, Qt::UserRole).toString() != "region")
            return;
        toggleRegionHighlight(item->data(0, Qt::UserRole + 1).toUInt());
    });

}

void Gui2DWindow::finalizeGeometryForWorkflow()
{
    // GUI-only gate. Geometry validation, loop construction and half-edges
    // will be added to this function in a later development step.
    geometryFinalized_ = true;
    if (computeMeshNode_) {
        computeMeshNode_->setDisabled(false);
        computeMeshNode_->setExpanded(true);
    }
    if (outputText_)
        outputText_->append("Finalize Geom: Compute Mesh enabled (geometry processing not yet implemented).");
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

    // Bounding box outline follows the current theme.
    QPen pen(theme_.boundingBox);
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

    QPen gridPen(theme_.gridLines);
    gridPen.setWidth(0);

    gridItem_ = scene_->addPath(
        gridPath,
        gridPen,
        Qt::NoBrush
    );

    gridItem_->setZValue(-1);
    // Preserve mesh visibility when rebuilding the grid.
    if (auto* a = findChild<QAction*>("showMeshVisibility")) gridItem_->setVisible(a->isChecked());
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
    if (selectionMode_ == SelectionMode::ConnectNodes) {
        connectNodesClick(scenePos);
        return;
    }
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
        checkpoint();
        const auto id = geometry_.addVertex(scenePos.x(), scenePos.y());
        redrawGeometry();
        outputText_->append(QString("Created vertex %1").arg(id));
        return;
    }
    // Start a new polyline
    if (!activePolyline_)
    {
        checkpoint();
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
    if (activePolyline_ && !geometry_.polylines().at(*activePolyline_).vertices.empty()) checkpoint();
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

    checkpoint();
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

    checkpoint();
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
        checkpoint();
        geometry_.renameThread(
            id, name.trimmed().toStdString());
    }
    else
    {
        checkpoint();
        geometry_.renamePolyline(
            id, name.trimmed().toStdString());
    }

    refreshGeometryTree();
}

void Gui2DWindow::validateGeometryFromMenu()
{
    const auto issues = geometry_.validate();
    outputText_->append("========== GEOMETRY VALIDATION ==========");
    outputText_->append(QString("Vertices: %1, Edges: %2, Polylines: %3")
        .arg(geometry_.vertexCount()).arg(geometry_.edgeCount()).arg(geometry_.polylineCount()));
    if (issues.empty()) outputText_->append("No geometry validation errors.");
    for (const auto& issue : issues)
        outputText_->append(QString::fromStdString(issue));
    outputText_->append(QString("Validation issues: %1").arg(issues.size()));
}

void Gui2DWindow::showGeometryProperties()
{
    auto* item = workflowTree_->currentItem();
    if (!item) return;
    const QString kind = item->data(0, Qt::UserRole).toString();
    if (kind != "group" && kind != "polyline") return;
    const auto id = item->data(0, Qt::UserRole + 1).toUInt();
    QDialog dialog(this);
    dialog.setWindowTitle(kind == "group" ? "Geometry Group Properties" : "Polyline Properties");
    QVBoxLayout layout(&dialog);
    QFormLayout form;
    QLineEdit name;
    QComboBox type;
    QCheckBox active("Enabled");
    QCheckBox inherit("Inherit group colour");
    QCheckBox inheritThickness("Inherit group thickness");
    QDoubleSpinBox thickness;
    thickness.setRange(0.1, 50.0);
    thickness.setSingleStep(0.5);
    thickness.setDecimals(1);
    QPushButton colorButton("Choose Colour...");
    QColor chosen;
    if (kind == "group")
    {
        const auto& groups = geometry_.threads();
        const auto it = std::find_if(groups.begin(), groups.end(),
            [id](const auto& g) { return g.id == id; });
        if (it == groups.end()) return;
        name.setText(QString::fromStdString(it->name));
        const char* names[] = {"Unassigned", "Wall", "Entrance", "Exit", "Obstacle", "Region"};
        for (const char* n : names) type.addItem(n);
        type.setCurrentIndex(static_cast<int>(it->type));
        active.setChecked(it->active);
        chosen = QColor::fromRgb(it->colorRGB);
        thickness.setValue(it->thickness);
        form.addRow("Geometry type:", &type);
        form.addRow("Active:", &active);
    }
    else
    {
        if (id >= geometry_.polylines().size() || geometry_.polylines()[id].deleted) return;
        const auto& poly = geometry_.polylines()[id];
        name.setText(QString::fromStdString(poly.name));
        inherit.setChecked(!poly.colorOverrideRGB);
        chosen = QColor::fromRgb(geometry_.effectivePolylineColor(id));
        inheritThickness.setChecked(!poly.thicknessOverride);
        thickness.setValue(geometry_.effectivePolylineThickness(id));
        form.addRow("Thickness inheritance:", &inheritThickness);
        form.addRow("Colour inheritance:", &inherit);
    }
    form.insertRow(0, "Name:", &name);
    form.addRow("Display colour:", &colorButton);
    form.addRow("Line thickness (x):", &thickness);
    auto updateSwatch = [&]() {
        colorButton.setText(chosen.name(QColor::HexRgb).toUpper());
        colorButton.setStyleSheet(QString("QPushButton { background-color: %1; color: %2; }")
            .arg(chosen.name(), chosen.lightness() < 128 ? "white" : "black"));
    };
    updateSwatch();
    QObject::connect(&colorButton, &QPushButton::clicked, &dialog, [&]() {
        QColor next = QColorDialog::getColor(chosen, &dialog, "Choose geometry colour");
        if (next.isValid()) { chosen = next; updateSwatch(); }
    });
    if (kind == "polyline")
    {
        colorButton.setEnabled(!inherit.isChecked());
        thickness.setEnabled(!inheritThickness.isChecked());
        QObject::connect(&inheritThickness, &QCheckBox::toggled, &dialog,
                         [&](bool checked) { thickness.setEnabled(!checked); });
        QObject::connect(&inherit, &QCheckBox::toggled, &dialog,
                         [&](bool checked) { colorButton.setEnabled(!checked); });
    }
    layout.addLayout(&form);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addWidget(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    if (name.text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Invalid name", "Name must not be empty.");
        return;
    }
    if (kind == "group")
    {
        checkpoint();
        geometry_.renameThread(id, name.text().trimmed().toStdString());
        geometry_.setThreadProperties(id, static_cast<geom::GeometryType>(type.currentIndex()),
                                     active.isChecked(), chosen.rgb() & 0xFFFFFF);
        geometry_.setThreadThickness(id, thickness.value());
    }
    else
    {
        checkpoint();
        geometry_.renamePolyline(id, name.text().trimmed().toStdString());
        geometry_.setPolylineColorOverride(id,
            inherit.isChecked() ? std::nullopt :
            std::optional<std::uint32_t>(chosen.rgb() & 0xFFFFFF));
        geometry_.setPolylineThicknessOverride(id,
            inheritThickness.isChecked() ? std::nullopt : std::optional<double>(thickness.value()));
    }
    redrawGeometry();
    outputText_->append("Geometry properties updated.");
}

void Gui2DWindow::showGeometryContextMenu(
    const QPoint& pos)
{
    QTreeWidgetItem* item =
        workflowTree_->itemAt(pos);

    if (!item)
        return;

    // Disabled Compute Mesh items cannot be operated through the context menu.
    if (computeMeshNode_ && !geometryFinalized_ &&
        (item == computeMeshNode_ || item == regionsNode_ ||
         item->parent() == regionsNode_))
        return;

    QMenu menu(this);

    if (item == regionsNode_) {
        menu.addAction("New Region...", this, &Gui2DWindow::createRegion);
        menu.exec(workflowTree_->viewport()->mapToGlobal(pos));
        return;
    }
    if (item->data(0, Qt::UserRole).toString() == "region") {
        const auto id = item->data(0, Qt::UserRole + 1).toUInt();
        menu.addAction("Properties...", this, [this, id] { editRegion(id); });
        menu.exec(workflowTree_->viewport()->mapToGlobal(pos));
        return;
    }
    if (item == computeMeshNode_) {
        menu.addAction("New Region...", this, &Gui2DWindow::createRegion);
        menu.exec(workflowTree_->viewport()->mapToGlobal(pos));
        return;
    }

    if (item == geometryRootNode_)
    {
        menu.addAction("Validate Geometry", this, &Gui2DWindow::validateGeometryFromMenu);
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
            menu.addAction("Properties...", this, &Gui2DWindow::showGeometryProperties);
            menu.addSeparator();
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
            menu.addAction("Properties...", this, &Gui2DWindow::showGeometryProperties);
            menu.addSeparator();
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

        checkpoint();
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

        checkpoint();
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
    if (!moves.empty()) checkpoint();
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
        checkpoint();
        geometry_.closePolyline(id);
    }
    else { checkpoint(); if (!geometry_.closePolylineReusingLastEdge(id))
    {
        outputText_->append("Cannot close without a new edge: need at least four vertices and an unshared final edge.");
        return;
    }
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
            QPen polyPen(edgePen);
            polyPen.setColor(QColor::fromRgb(geometry_.effectivePolylineColor(poly.id)));
            polyPen.setWidthF(2.0 * geometry_.effectivePolylineThickness(poly.id));
            auto* line = scene_->addLine(a.x,a.y,b.x,b.y,polyPen);
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
    if (selectedGeometryVertex_ || selectedGeometryEdge_ || selectedGeometryPolyline_) checkpoint();
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

void Gui2DWindow::checkpoint()
{
    undoHistory_.push_back(geometry_);
    if (undoHistory_.size() > 80) undoHistory_.erase(undoHistory_.begin());
    redoHistory_.clear();
}

void Gui2DWindow::undoGeometry()
{
    if (undoHistory_.empty()) { outputText_->append("Nothing to undo."); return; }
    activePolyline_.reset();
    redoHistory_.push_back(geometry_);
    geometry_ = std::move(undoHistory_.back()); undoHistory_.pop_back();
    selectedGeometryVertex_.reset(); selectedGeometryEdge_.reset(); selectedGeometryPolyline_.reset();
    redrawGeometry();
}

void Gui2DWindow::redoGeometry()
{
    if (redoHistory_.empty()) { outputText_->append("Nothing to redo."); return; }
    activePolyline_.reset();
    undoHistory_.push_back(geometry_);
    geometry_ = std::move(redoHistory_.back()); redoHistory_.pop_back();
    selectedGeometryVertex_.reset(); selectedGeometryEdge_.reset(); selectedGeometryPolyline_.reset();
    redrawGeometry();
}

void Gui2DWindow::saveGeometry()
{
    const QString filename = QFileDialog::getSaveFileName(this, "Save Geometry", geometryFilePath_,
                                                          "EvolBeing Geometry (*.eg2d.json);;JSON (*.json)");
    if (filename.isEmpty()) return;
    QJsonObject root;
    root["format"] = "EvolBeing.Geometry2D";
    root["version"] = 1;
    QJsonArray vertices, edges, polylines, groups;
    for (const auto& v : geometry_.vertices()) {
        QJsonObject o; o["id"] = int(v.id); o["x"] = v.x; o["y"] = v.y; o["deleted"] = v.deleted;
        vertices.append(o);
    }
    for (const auto& e : geometry_.edges()) {
        QJsonObject o; o["id"] = int(e.id); o["vertex0"] = int(e.vertex0);
        o["vertex1"] = int(e.vertex1); o["deleted"] = e.deleted; edges.append(o);
    }
    for (const auto& g : geometry_.threads()) {
        QJsonObject o; o["id"] = double(g.id); o["name"] = QString::fromStdString(g.name);
        o["type"] = int(g.type); o["active"] = g.active;
        o["colorRGB"] = double(g.colorRGB); o["thickness"] = g.thickness;
        QJsonArray members; for (auto id : g.polylines) members.append(double(id));
        o["polylines"] = members; groups.append(o);
    }
    for (const auto& p : geometry_.polylines()) {
        QJsonObject o; o["id"] = double(p.id); o["name"] = QString::fromStdString(p.name);
        o["closed"] = p.closed; o["deleted"] = p.deleted;
        o["threadID"] = p.threadID ? QJsonValue(double(*p.threadID)) : QJsonValue(QJsonValue::Null);
        o["colorOverrideRGB"] = p.colorOverrideRGB ? QJsonValue(double(*p.colorOverrideRGB)) : QJsonValue(QJsonValue::Null);
        o["thicknessOverride"] = p.thicknessOverride ? QJsonValue(*p.thicknessOverride) : QJsonValue(QJsonValue::Null);
        QJsonArray vs, es;
        for (auto id : p.vertices) vs.append(double(id));
        for (auto id : p.edges) es.append(double(id));
        o["vertices"] = vs; o["edges"] = es; polylines.append(o);
    }
    root["vertices"] = vertices; root["edges"] = edges;
    root["polylines"] = polylines; root["groups"] = groups;
    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        QMessageBox::warning(this, "Save Geometry", "Unable to write geometry file."); return;
    }
    geometryFilePath_ = filename;
    outputText_->append("Saved geometry: " + filename);
}

void Gui2DWindow::loadGeometry()
{
    const QString filename = QFileDialog::getOpenFileName(this, "Load Geometry", geometryFilePath_,
                                                          "EvolBeing Geometry (*.eg2d.json *.json)");
    if (filename.isEmpty()) return;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Load Geometry", "Cannot read file."); return; }
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, "Load Geometry", "Invalid JSON document."); return;
    }
    const auto root = document.object();
    if (root.value("format").toString() != "EvolBeing.Geometry2D" || root.value("version").toInt() != 1) {
        QMessageBox::warning(this, "Load Geometry", "Unsupported geometry file format/version."); return;
    }
    try {
        auto requireArray = [&](const QJsonObject& o, const char* key) -> QJsonArray {
            if (!o.value(key).isArray()) throw std::runtime_error("Missing array field");
            return o.value(key).toArray();
        };
        auto integer = [&](const QJsonObject& o, const char* key) -> std::uint32_t {
            auto v=o.value(key);
            if (!v.isDouble() || v.toDouble() < 0 || v.toDouble() > 4294967295.0 ||
                std::floor(v.toDouble()) != v.toDouble()) throw std::runtime_error("Invalid ID");
            return static_cast<std::uint32_t>(v.toDouble());
        };
        auto idArray = [&](const QJsonObject& o, const char* key) {
            std::vector<std::uint32_t> ids;
            for (const auto& v : requireArray(o,key)) {
                QJsonObject temp; temp["id"] = v; ids.push_back(integer(temp,"id"));
            }
            return ids;
        };
        std::vector<geom::GeometryVertex2D> vertices;
        std::vector<geom::GeometryEdge2D> edges;
        std::vector<geom::Polyline2D> polylines;
        std::vector<geom::GeometryThread2D> groups;
        for (const auto& value : requireArray(root,"vertices")) {
            if (!value.isObject()) throw std::runtime_error("Invalid vertex");
            auto o=value.toObject(); geom::GeometryVertex2D v;
            v.id=integer(o,"id");
            if (!o.value("x").isDouble() || !o.value("y").isDouble() || !o.value("deleted").isBool()) throw std::runtime_error("Invalid vertex fields");
            v.x=o.value("x").toDouble(); v.y=o.value("y").toDouble(); v.deleted=o.value("deleted").toBool(); vertices.push_back(v);
        }
        for (const auto& value : requireArray(root,"edges")) {
            if (!value.isObject()) throw std::runtime_error("Invalid edge");
            auto o=value.toObject(); geom::GeometryEdge2D e;
            e.id=integer(o,"id"); e.vertex0=integer(o,"vertex0"); e.vertex1=integer(o,"vertex1");
            if (!o.value("deleted").isBool()) throw std::runtime_error("Invalid edge deletion state");
            e.deleted=o.value("deleted").toBool(); edges.push_back(e);
        }
        for (const auto& value : requireArray(root,"groups")) {
            if (!value.isObject()) throw std::runtime_error("Invalid group");
            auto o=value.toObject(); geom::GeometryThread2D g;
            g.id=integer(o,"id"); g.colorRGB=integer(o,"colorRGB");
            if (!o.value("name").isString() || !o.value("active").isBool() || !o.value("thickness").isDouble()) throw std::runtime_error("Invalid group fields");
            g.name=o.value("name").toString().toStdString(); g.active=o.value("active").toBool();
            g.type=static_cast<geom::GeometryType>(integer(o,"type")); g.thickness=o.value("thickness").toDouble();
            g.polylines=idArray(o,"polylines"); groups.push_back(std::move(g));
        }
        for (const auto& value : requireArray(root,"polylines")) {
            if (!value.isObject()) throw std::runtime_error("Invalid polyline");
            auto o=value.toObject(); geom::Polyline2D p;
            p.id=integer(o,"id");
            if (!o.value("name").isString() || !o.value("closed").isBool() || !o.value("deleted").isBool()) throw std::runtime_error("Invalid polyline fields");
            p.name=o.value("name").toString().toStdString(); p.closed=o.value("closed").toBool(); p.deleted=o.value("deleted").toBool();
            if (!o.value("threadID").isNull()) p.threadID=integer(o,"threadID");
            if (!o.value("colorOverrideRGB").isNull()) p.colorOverrideRGB=integer(o,"colorOverrideRGB");
            if (!o.value("thicknessOverride").isNull()) {
                if (!o.value("thicknessOverride").isDouble()) throw std::runtime_error("Invalid thickness override");
                p.thicknessOverride=o.value("thicknessOverride").toDouble();
            }
            p.vertices=idArray(o,"vertices"); p.edges=idArray(o,"edges"); polylines.push_back(std::move(p));
        }
        geom::Geometry2D loaded;
        if (!loaded.importState(std::move(vertices),std::move(edges),std::move(polylines),std::move(groups)))
            throw std::runtime_error("Geometry connectivity or metadata validation failed");
        if (QMessageBox::question(this, "Load Geometry", "Replace the current geometry with the loaded file?\nThe Cartesian mesh will be preserved.") != QMessageBox::Yes) return;
        checkpoint();
        cancelDrawing();
        geometry_=std::move(loaded);
        selectedGeometryVertex_.reset(); selectedGeometryEdge_.reset(); selectedGeometryPolyline_.reset();
        geometryFilePath_=filename;
        redrawGeometry();
        outputText_->append("Loaded geometry: " + filename);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Load Geometry", QString("Invalid geometry file: %1").arg(e.what()));
    }
}

void Gui2DWindow::scaleGeometry()
{
    QDialog dialog(this); dialog.setWindowTitle("Scale Geometry");
    QFormLayout layout(&dialog);
    QDoubleSpinBox sx, sy; sx.setRange(0.001,10000); sy.setRange(0.001,10000);
    sx.setDecimals(4); sy.setDecimals(4); sx.setValue(1); sy.setValue(1);
    QCheckBox uniform("Uniform scaling"); uniform.setChecked(true);
    QObject::connect(&sx, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &dialog,
                     [&](double v) { if (uniform.isChecked()) sy.setValue(v); });
    layout.addRow("X factor:", &sx); layout.addRow("Y factor:", &sy); layout.addRow(&uniform);
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    layout.addRow(&buttons);
    QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if (dialog.exec()!=QDialog::Accepted) return;
    bool found=false; double xmin=0,xmax=0,ymin=0,ymax=0;
    for (const auto& v : geometry_.vertices()) if (!v.deleted) {
        if (!found) { xmin=xmax=v.x; ymin=ymax=v.y; found=true; }
        else { xmin=std::min(xmin,v.x); xmax=std::max(xmax,v.x); ymin=std::min(ymin,v.y); ymax=std::max(ymax,v.y); }
    }
    if (!found) return;
    const double cx=(xmin+xmax)*0.5, cy=(ymin+ymax)*0.5;
    checkpoint();
    for (const auto& v : geometry_.vertices()) if (!v.deleted)
        geometry_.moveVertex(v.id,cx+sx.value()*(v.x-cx),cy+sy.value()*(v.y-cy));
    redrawGeometry(); outputText_->append("Geometry scaled; Cartesian mesh unchanged.");
}

void Gui2DWindow::connectNodesClick(const QPointF& pos)
{
    // Reuse an existing geometry vertex, or create a geometry vertex at a mesh node.
    std::optional<geom::GeometryVertexID> chosen;
    double best=12.0;
    const QPoint screen=view_->mapFromScene(pos);
    for (const auto& v : geometry_.vertices()) if (!v.deleted) {
        const double d=QLineF(QPointF(screen),QPointF(view_->mapFromScene(QPointF(v.x,v.y)))).length();
        if (d < best) { best=d; chosen=v.id; }
    }
    QPointF target=pos;
    if (!chosen && grid_.nodeCount()) target=nearestMeshNode(pos);
    if (!chosen) {
        if (!grid_.nodeCount() || QLineF(QPointF(screen),QPointF(view_->mapFromScene(target))).length()>12.0) {
            outputText_->append("Connect Nodes: click an existing geometry or Cartesian node."); return;
        }
        for (const auto& v : geometry_.vertices()) if (!v.deleted && QLineF(QPointF(v.x,v.y),target).length()<1e-9) { chosen=v.id; break; }
    }
    if (!chosen) { checkpoint(); chosen=geometry_.addVertex(target.x(),target.y()); }
    if (!connectStart_) { connectStart_=chosen; outputText_->append(QString("Connect Nodes: first vertex %1 selected.").arg(*chosen)); redrawGeometry(); return; }
    const auto first=*connectStart_; connectStart_.reset();
    if (first==*chosen) { outputText_->append("Connect Nodes: select two different nodes."); return; }
    for (const auto& e : geometry_.edges()) if (!e.deleted &&
        ((e.vertex0==first && e.vertex1==*chosen)||(e.vertex1==first && e.vertex0==*chosen))) {
        outputText_->append("Connect Nodes: edge already exists."); return;
    }
    checkpoint();
    const auto id=geometry_.createPolyline();
    geometry_.appendVertexToPolyline(id,first);
    geometry_.appendVertexToPolyline(id,*chosen);
    redrawGeometry();
    outputText_->append(QString("Connected geometry vertices %1 and %2 (polyline %3).").arg(first).arg(*chosen).arg(id));
}


void Gui2DWindow::clearRegionHighlight()
{
    if (regionSeedHighlight_) {
        scene_->removeItem(regionSeedHighlight_);
        delete regionSeedHighlight_;
        regionSeedHighlight_ = nullptr;
    }
    highlightedRegionId_.reset();
}

void Gui2DWindow::toggleRegionHighlight(std::uint32_t id)
{
    if (highlightedRegionId_ && *highlightedRegionId_ == id) {
        clearRegionHighlight();
        return;
    }
    clearRegionHighlight();
    auto it = std::find_if(regionSeeds_.begin(), regionSeeds_.end(),
        [id](const RegionSeed& r) { return r.id == id; });
    if (it == regionSeeds_.end()) return;

    // The only region marker: created on selection, deleted on deselection.
    regionSeedHighlight_ = scene_->addEllipse(-5, -5, 10, 10,
        QPen(Qt::magenta, 2), QBrush(Qt::yellow));
    regionSeedHighlight_->setPos(it->vertex);
    regionSeedHighlight_->setZValue(2000);
    regionSeedHighlight_->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    highlightedRegionId_ = id;
}

void Gui2DWindow::refreshRegionsTree()
{
    if (!regionsNode_) return;
    // Do not destroy the highlight when rebuilding the tree.
    while (regionsNode_->childCount() > 0)
        delete regionsNode_->takeChild(0);
    for (const auto& region : regionSeeds_) {
        auto* item = new QTreeWidgetItem(regionsNode_);
        item->setText(0, QString("%1 (%2)").arg(region.name,
            region.type == RegionType::Live ? "Live" :
            region.type == RegionType::Dead ? "Dead" : "Obstacle"));
        item->setData(0, Qt::UserRole, "region");
        item->setData(0, Qt::UserRole + 1, region.id);
        item->setToolTip(0, QString("Seed: (%1, %2)")
            .arg(region.vertex.x()).arg(region.vertex.y()));
    }
    regionsNode_->setExpanded(true);
}

void Gui2DWindow::createRegion()
{
    // Non-modal, heap-owned dialog: viewport clicks remain available.
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("New Region");
    auto* layout = new QVBoxLayout(dialog);
    auto* form = new QFormLayout();
    auto* name = new QLineEdit(QString("Region %1").arg(nextRegionId_), dialog);
    
    auto* x = new QDoubleSpinBox(dialog);
    auto* y = new QDoubleSpinBox(dialog);
    for (auto* spin : {x, y}) { spin->setRange(-1e9, 1e9); spin->setDecimals(6); }

    // Temporary vertex marker while the dialog is open.
    auto* previewMarker = scene_->addEllipse(
        -5.0, -5.0, 10.0, 10.0,
        QPen(Qt::magenta, 2),
        QBrush(Qt::yellow)
    );

    previewMarker->setZValue(2001);
    previewMarker->setFlag(
        QGraphicsItem::ItemIgnoresTransformations
    );

    // Initially show the marker at the entered coordinates.
    previewMarker->setPos(x->value(), y->value());

    // Move the marker whenever the coordinates change.
    auto updatePreview = [previewMarker, x, y]()
    {
        previewMarker->setPos(x->value(), y->value());
    };

    connect(x, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            dialog, updatePreview);

    connect(y, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            dialog, updatePreview);


    auto* type = new QComboBox(dialog);
    type->addItems({"Live", "Dead", "Obstacle"});
    form->addRow("Region name:", name);
    form->addRow("Vertex X:", x);
    form->addRow("Vertex Y:", y);
    form->addRow("Region type:", type);
    layout->addLayout(form);
    auto* pick = new QPushButton("Pick vertex in simulation view", dialog);
    layout->addWidget(pick);
    auto* hint = new QLabel("Enter coordinates, or click Pick then click the simulation view.", dialog);
    layout->addWidget(hint);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, dialog);
    layout->addWidget(buttons);

    connect(pick, &QPushButton::clicked, dialog, [this, dialog, x, y, hint] {
        hint->setText("Click a location in the simulation view...");
        regionPickHandler_ = [dialog, x, y, hint](const QPointF& p) {
            x->setValue(p.x());
            y->setValue(p.y());
            hint->setText("Vertex selected. Click OK to save.");
            dialog->raise();
            dialog->activateWindow();
        };
        this->raise();
        this->activateWindow();
        view_->setFocus();
    });
    connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog, name, x, y, type] {
        if (name->text().trimmed().isEmpty()) {
            QMessageBox::warning(dialog, "Region", "Enter a region name.");
            return;
        }
        RegionSeed candidate{nextRegionId_++, name->text().trimmed(),
                             QPointF(x->value(), y->value()),
                             static_cast<RegionType>(type->currentIndex())};
        regionSeeds_.push_back(std::move(candidate));
        refreshRegionsTree();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::finished, this,
    [this, previewMarker](int)
    {
        regionPickHandler_ = {};

        // Remove temporary vertex from scene.
        scene_->removeItem(previewMarker);
        delete previewMarker;
    });
    dialog->show();
}

void Gui2DWindow::editRegion(std::uint32_t id)
{
    auto it = std::find_if(regionSeeds_.begin(), regionSeeds_.end(),
        [id](const RegionSeed& r) { return r.id == id; });
    if (it == regionSeeds_.end()) return;

    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Region Properties");
    auto* layout = new QVBoxLayout(dialog);
    auto* form = new QFormLayout();
    auto* name = new QLineEdit(it->name, dialog);
    auto* x = new QDoubleSpinBox(dialog);
    auto* y = new QDoubleSpinBox(dialog);
    for (auto* spin : {x, y}) { spin->setRange(-1e9, 1e9); spin->setDecimals(6); }
    // Temporary vertex marker while the dialog is open.
    auto* previewMarker = scene_->addEllipse(
        -5.0, -5.0, 10.0, 10.0,
        QPen(Qt::magenta, 2),
        QBrush(Qt::yellow)
    );

    previewMarker->setZValue(2001);
    previewMarker->setFlag(
        QGraphicsItem::ItemIgnoresTransformations
    );

    // Initially show the marker at the entered coordinates.
    previewMarker->setPos(x->value(), y->value());

    // Move the marker whenever the coordinates change.
    auto updatePreview = [previewMarker, x, y]()
    {
        previewMarker->setPos(x->value(), y->value());
    };

    connect(x, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            dialog, updatePreview);

    connect(y, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            dialog, updatePreview);

    x->setValue(it->vertex.x());
    y->setValue(it->vertex.y());
    auto* type = new QComboBox(dialog);
    type->addItems({"Live", "Dead", "Obstacle"});
    type->setCurrentIndex(static_cast<int>(it->type));
    form->addRow("Region name:", name);
    form->addRow("Vertex X:", x);
    form->addRow("Vertex Y:", y);
    form->addRow("Region type:", type);
    layout->addLayout(form);
    auto* pick = new QPushButton("Pick vertex in simulation view", dialog);
    layout->addWidget(pick);
    auto* hint = new QLabel("Enter coordinates, or click Pick then click the simulation view.", dialog);
    layout->addWidget(hint);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, dialog);
    layout->addWidget(buttons);

    connect(pick, &QPushButton::clicked, dialog, [this, dialog, x, y, hint] {
        hint->setText("Click a location in the simulation view...");
        regionPickHandler_ = [dialog, x, y, hint](const QPointF& p) {
            x->setValue(p.x());
            y->setValue(p.y());
            hint->setText("Vertex selected. Click OK to save.");
            dialog->raise();
            dialog->activateWindow();
        };
        this->raise();
        this->activateWindow();
        view_->setFocus();
    });
    connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog, id, name, x, y, type] {
        if (name->text().trimmed().isEmpty()) {
            QMessageBox::warning(dialog, "Region", "Enter a region name.");
            return;
        }
        auto found = std::find_if(regionSeeds_.begin(), regionSeeds_.end(),
            [id](const RegionSeed& r) { return r.id == id; });
        if (found == regionSeeds_.end()) { dialog->reject(); return; }
        found->name = name->text().trimmed();
        found->vertex = QPointF(x->value(), y->value());
        found->type = static_cast<RegionType>(type->currentIndex());
        if (highlightedRegionId_ && *highlightedRegionId_ == id) {
            clearRegionHighlight();
            toggleRegionHighlight(id);
        }
        refreshRegionsTree();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::finished, this, [this, previewMarker](int) 
    { 
        regionPickHandler_ = {}; 
        scene_->removeItem(previewMarker); delete previewMarker; 
    });
    dialog->show();
}
