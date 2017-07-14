#ifndef WORKER_HPP
#define WORKER_HPP

#include <QObject>
#include <QList>
#include <QDebug>
#include "point.hpp"
#include "dopasowanie.hpp"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = 0);

private:
    QString fitKerg(const Point &A, const Point &B);
signals:
    void jobDone(QVector<QPair<Point, Point>> OutputList, int TeoretycznieMozliwe);
    void setProgressRange( int X1, int X2 );
    void setProgress( int i );

public slots:
    void startComputing(const QVector<Point*> & Points, Dopasowanie Dop, bool RegExpEnable, bool DifferentKerg, QString Filter );
    void test();
};

#endif // WORKER_HPP
