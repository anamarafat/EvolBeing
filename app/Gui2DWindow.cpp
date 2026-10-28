#include "Gui2DWindow.hpp"

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
    view_ = new QGraphicsView(scene_, leftPanel);
    view_->setRenderHint(QPainter::Antialiasing);

    // Viewport takes all remaining vertical space
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