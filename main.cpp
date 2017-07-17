#include "mainwindow.hpp"
#include <QApplication>
#include "dopasowanie.hpp"

int main(int argc, char *argv[])
{    
    QApplication a(argc, argv);
    qRegisterMetaType< Dopasowanie >("Dopasowanie");
    qRegisterMetaType< QVector<Point> >("QVector<Point>");
    qRegisterMetaType< QVector<QPair<Point,Point>> >("QVector<QPair<Point,Point>>");

    MainWindow w;
    w.show();

    return a.exec();
}
