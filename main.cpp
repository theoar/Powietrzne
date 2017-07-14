#include "mainwindow.hpp"
#include <QApplication>
#include "dopasowanie.hpp"

int main(int argc, char *argv[])
{    
    QApplication a(argc, argv);
    qRegisterMetaType<Dopasowanie>("Dopasowanie");
    MainWindow w;
    w.show();

    return a.exec();
}
