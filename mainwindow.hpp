#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QString>
#include <QList>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QtMath>
#include <QRegExp>
#include <QMessageBox>
#include <QStringList>
#include <QThread>
#include <QTableWidget>

#include "point.hpp"
#include "dopasowanie.hpp"
#include "worker.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void requestComputing(const QVector<Point> Points, Dopasowanie Dop, bool RegExpEnable, bool DifferentKerg, QString Filter );
    void requestOpening(QString Path);
    void requestAppendToList(const QVector<Point> Points, QTableWidget *Table);
    void requestSaving(const QVector<QPair<Point, Point>> Points, QString Path, QString Pattern);

public slots:
    void onButtonOtworz();
    void onButtonKonwertuj();
    void onButtonZapisz();

    void onJobDone(QVector<QPair<Point, Point>> OutputList, int TeoretycznieMozliwe);
    void onJobCancelled();

    void onPointsLoaded(QVector<Point> P);
    void onPointsLodingFailed(QString Path);

    void onPointsSaved();
    void onPointsSavingFailed(QString Path);

    void enableGui();
    void distableGui();

    void appendToView();

private:
    Ui::MainWindow *ui;
    QVector<Point> Points;
    QVector<QPair<Point, Point>> ListaWyjsciowa;
    QThread Thread;
    Worker *Mudzin;

    double pointDistance(const Point A, const Point B);
    double pointKatDifference(const Point A, const Point B);
    double pointDopasowanie(double Odl, double Kat, double Wd, double Wk, double Azy, double Wa);
    double pointAzymutDifference(const Point A, const Point B);
    double sredniaWazona(QList<QPair<double, double>> Elementy); //Para: wartość, waga

    void clearPoints();

    void aktualizujStanPrzyciskow();
    void aktualizujLiczniki();
    void fillComobBox();    
    void initTable();
    void distableButtons();
    void enableButtions();

    QStringList WyjscieWzor = {"0 20 $X1 $Y1 $X2 $Y2 2008 0 $KERG &NN&",
                               "0 20 $X1 $Y1 $X2 $Y2 2007 0 $KERG &SN&",
                               "0 20 $X1 $Y1 $X2 $Y2 2006 0 $KERG &WN&",
                               "0 20 $X1 $Y1 $X2 $Y2 2005 0 $KERG &WW&",
                               "0 20 $X1 $Y1 $X2 $Y2 2016 0 $KERG &TELEKOM&"
                              };

    QStringList Headers = { "X", "Y", "Wys", "Kąt", "Flaga", "Kod", "Kerg" };
};

#endif // MAINWINDOW_HPP
