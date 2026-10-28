#pragma once

#include "SurfaceMesh.hpp"

#include <QWidget>

class QPushButton;
class QLabel;

class Gui2DWindow;

class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void importMesh();

    QPushButton *importButton_ = nullptr;
    QLabel *statusLabel_ = nullptr;

    Gui2DWindow* gui2DWindow_ = nullptr;

    geom::SurfaceMesh mesh_;
};