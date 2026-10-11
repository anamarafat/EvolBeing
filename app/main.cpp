#include "MainWindow.hpp"
#include <QApplication>

int main(int argc, char* argv[])
{
    /* Code to invoke QT*/
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();

    return 0;
}