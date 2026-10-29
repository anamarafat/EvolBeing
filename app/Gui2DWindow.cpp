#include "Gui2DWindow.hpp"
#include "MeshGraphicsView.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

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
#include <QLineF>


#include <algorithm>
#include <cmath>
#include <limits>

Gui2DWindow::Gui2DWindow(QWidget* parent)
    : QMainWindow(parent)
{
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

    // Toolbar for mesh viewing and selection
    QToolBar* meshToolbar = new QToolBar(leftPanel);
    meshToolbar->setMovable(false);

    // Fit view button
    fitViewAction_ = meshToolbar->addAction("Fit View");
    meshToolbar->addSeparator();

    // Selection buttons
    cellSelectAction_ = meshToolbar->addAction("Cell");
    faceSelectAction_ = meshToolbar->addAction("Face");

    // Allow buttons to remain pressed/highlighted
    cellSelectAction_->setCheckable(true);
    faceSelectAction_->setCheckable(true);

    // Group the selection buttons
    QActionGroup* selectionGroup = new QActionGroup(this);
    selectionGroup->setExclusionPolicy(
        QActionGroup::ExclusionPolicy::ExclusiveOptional);

    selectionGroup->addAction(cellSelectAction_);
    selectionGroup->addAction(faceSelectAction_);

    // Fit View
    connect(fitViewAction_, &QAction::triggered,
            this, &Gui2DWindow::fitMeshView);

    // Cell selection
    connect(cellSelectAction_, &QAction::toggled,
            this, [this](bool checked)
            {
                if (checked)
                    selectionMode_ = SelectionMode::Cell;
                else if (!faceSelectAction_->isChecked())
                    selectionMode_ = SelectionMode::None;

                clearSelectionHighlight();
            });

    // Face selection
    connect(faceSelectAction_, &QAction::toggled,
            this, [this](bool checked)
            {
                if (checked)
                    selectionMode_ = SelectionMode::Face;
                else if (!cellSelectAction_->isChecked())
                    selectionMode_ = SelectionMode::None;

                clearSelectionHighlight();
            });


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
        QString("Cell Threads    : %1").arg(grid_.cellThreadCount())
    );
    outputText_->append(
        QString("Face Threads    : %1").arg(grid_.faceThreadCount())
    );
    outputText_->append("====================================");

}

void Gui2DWindow::handleMeshClick(const QPointF& scenePos)
{
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

    // NEW: Forget which mesh cell was selected
    selectedCell_ = nullptr;
    selectedFace_ = nullptr;

}