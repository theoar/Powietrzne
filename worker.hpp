#ifndef WORKER_HPP
#define WORKER_HPP

#include <QObject>
#include <QList>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QTableWidget>
#include <QMutex>
#include <QMutexLocker>

#include "point.hpp"
#include "dopasowanie.hpp"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = 0);

private:
    QString fitKerg(const Point &A, const Point &B);
    bool keepRunning();
    void setRunning();

    bool Cancel = false;
    QMutex Mutex;
signals:
    void jobDone(QVector<QPair<Point, Point>> OutputList, int TeoretycznieMozliwe);
    void pointsLoaded(QVector<Point> P);
    void pointsLoadingFailed(QString Path);
    void pointsSavingFailed(QString Path);
    void pointsAddedToView(void);
    void pointsSaved(void);

    void setProgressRange( int X1, int X2 );
    void setProgress( int i );

    void cancelled();


public slots:
    void startComputing(const QVector<Point> Points, Dopasowanie Dop, bool RegExpEnable, bool DifferentKerg, QString Filter );
    void startOpening(QString Path);
    void startSaving(const QVector<QPair<Point, Point>> Points, QString Path, QString Pattern);

    void test();

public:
    void cancelCopmuting();
};

#endif // WORKER_HPP
