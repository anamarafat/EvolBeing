#include "MainWindow.hpp"
#include "ObjIO.hpp"
#include "Gui2DWindow.hpp"

#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <exception>
#include <filesystem>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("EvolBeing - Geometry Tools");
    resize(450, 200);

    auto* layout = new QVBoxLayout(this);

    /* Mesh Import */
    importButton_ = new QPushButton("Import Mesh (.obj)", this);
    statusLabel_ = new QLabel("No mesh loaded", this);

    


    statusLabel_->setWordWrap(true);

    layout->addWidget(importButton_);
    layout->addWidget(statusLabel_);
    

    connect(importButton_, &QPushButton::clicked,
            this, [this]()
    {
        importMesh();
    });

    /* 2d Window */
    auto* launchGui2DButton = new QPushButton("Launch GUI2D", this);
    layout->addWidget(launchGui2DButton);
    
    connect(launchGui2DButton, &QPushButton::clicked,
            this, [this]()
    {
        if (!gui2DWindow_)
        {
            gui2DWindow_ = new Gui2DWindow(this);
            gui2DWindow_->setWindowFlag(Qt::Window, true);
        }

        gui2DWindow_->show();
        gui2DWindow_->raise();
        gui2DWindow_->activateWindow();
    });


}

void MainWindow::importMesh()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Select OBJ Mesh",
        QString(),
        "OBJ Mesh Files (*.obj)"
    );

    if (filename.isEmpty())
        return;

    try
    {
        std::filesystem::path meshPath(filename.toStdWString());

        // Load into a temporary mesh first.
        // Existing geometry remains intact if parsing fails.
        geom::SurfaceMesh importedMesh = geom::loadOBJ(meshPath);

        mesh_ = std::move(importedMesh);

        QString message =
            QString("File: %1\nVertices: %2\nTriangles: %3")
                .arg(filename)
                .arg(mesh_.vertexCount())
                .arg(mesh_.faceCount());

        statusLabel_->setText(message);
    }
    catch (const std::exception& error)
    {
        statusLabel_->setText(
            QString("Import failed: %1").arg(error.what())
        );
    }
}