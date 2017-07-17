#include "worker.hpp"

Worker::Worker(QObject *parent) : QObject(parent)
{

}

QString Worker::fitKerg(const Point &A, const Point &B)
{
    int RokA, RokB;
    int NumA, NumB;
    int IndexNumer, IndexRok;

    if(A.Kerg==B.Kerg)
        return A.Kerg;

    QRegExp Reg("#P\\.\\d+\\.(\\d+)\\.(\\d+)");

    if(A.Kerg=="_")
        return B.Kerg;
    if(B.Kerg=="_")
        return A.Kerg;

    if(A.Kerg.contains(Reg) && !B.Kerg.contains(Reg))
        return A.Kerg;
    if(B.Kerg.contains(Reg) && !A.Kerg.contains(Reg))
        return B.Kerg;

    if(A.Kerg.contains(Reg) && B.Kerg.contains(Reg))
    {
        IndexNumer = 2;
        IndexRok = 1;
    }
    else
    {
        Reg.setPattern("(\\d+)\\/(\\d+)");
        IndexNumer = 1;
        IndexRok = 2;
    }

    Reg.indexIn(A.Kerg);
    QString Rok = Reg.cap(IndexRok);
    RokA = Reg.cap(IndexRok).toInt();
    NumA = Reg.cap(IndexNumer).toInt();

    if(Rok[0]=='0')
        RokA+=2000;
    else
        if(Rok.length()<3)
            RokA+=1900;

    Reg.indexIn(B.Kerg);
    Rok = Reg.cap(IndexRok);
    RokB = Reg.cap(IndexRok).toInt();
    NumB = Reg.cap(IndexNumer).toInt();

    if(Rok[0]=='0')
        RokB+=2000;
    else
        if(Rok.length()<3)
            RokB+=1900;

    if(RokA>RokB)
        return A.Kerg;
    if(RokB>RokA)
        return B.Kerg;
    if(NumA>NumB)
        return A.Kerg;
    if(NumB>NumA)
        return B.Kerg;

    return A.Kerg;
}

void Worker::setRunning()
{
    QMutexLocker MLocker(&Mutex);
    Cancel=false;
}

bool Worker::keepRunning()
{
    QMutexLocker MLocker(&Mutex);
    return !Cancel;
}

void Worker::startComputing(const QVector<Point> Points,  Dopasowanie Dop, bool RegExpEnable, bool DifferentKerg, QString Filter)
{
    setRunning();

    bool KodFilterEnabled = !Filter.isEmpty() && !Filter.isNull();
    double MaxWskaznikDopasowania = Dop.MaksymalnyWskaznikDop();

    QVector<QPair<int, int>> ListaDopasowania;

    int TeoretycznieMozliwe = 0;
    int Progress = 0;

    emit setProgressRange(0 , Points.length());

    for(int X = 0; X<Points.length(); ++X)
    {
        const Point &PointBegin = Points[X];
        int DopasowanyPunkt = -1;
        double NajlepszyWskaznikDopasowania = MaxWskaznikDopasowania;

        QRegExp RegExp(Filter);
        if( !(KodFilterEnabled && ((RegExpEnable && !PointBegin.Kod.contains(RegExp) ) || (!RegExpEnable && !(PointBegin.Kod == Filter)))) )
        {
            for(int Y = 0; Y<Points.length(); ++Y)
            {
                const Point &PointEnd = Points[Y];
                if( X==Y || (PointBegin.Kod!=PointEnd.Kod) || (PointBegin.Kerg!=PointEnd.Kerg && !DifferentKerg) ||
                        ( KodFilterEnabled && ((RegExpEnable && !PointBegin.Kod.contains(RegExp)) || (!RegExpEnable && !(PointBegin.Kod == Filter)))) )
                    continue;

                double NowyWskaznikDopasowania = Dop.ObliczDopasowanie(PointBegin, PointEnd);
                if(NowyWskaznikDopasowania>0 && NowyWskaznikDopasowania<NajlepszyWskaznikDopasowania)
                {
                    DopasowanyPunkt=Y;
                    NajlepszyWskaznikDopasowania = NowyWskaznikDopasowania;
                }

                if(!keepRunning())
                {
                    qDebug() << "calncelled";
                    emit cancelled();
                    return;
                }
            }

            if(DopasowanyPunkt>=0)
                ListaDopasowania.push_back(QPair<int, int>(X, DopasowanyPunkt));

            TeoretycznieMozliwe++;
        }

        emit setProgress(Progress+=1);
    }

    Progress=0;
    setProgressRange(Progress, ListaDopasowania.length());

    QVector<QPair<Point, Point>> OutputList;
    for(int X = 0; X<ListaDopasowania.length(); ++X)
    {
        QPair<int, int> & ParaPierwsza = ListaDopasowania[X];
        if(ParaPierwsza.first>=0 && ParaPierwsza.second>=0)
        {
            for(int Y = 0; Y<ListaDopasowania.length(); ++Y)
            {
                QPair<int, int> & ParaDruga = ListaDopasowania[Y];
                if(X!=Y && ParaDruga.second==ParaPierwsza.first && ParaDruga.first==ParaPierwsza.second)
                {
                    QPair<Point, Point> Pair = { Points[ParaPierwsza.first], Points[ParaPierwsza.second]};
                    ListaDopasowania[Y].first = ListaDopasowania[Y].second = ListaDopasowania[X].first = ListaDopasowania[X].second = -1;
                    Pair.first.Kerg = Pair.second.Kerg = fitKerg(Pair.first, Pair.second);
                    OutputList.push_back(Pair);
                    break;
                }
            }
        }
        setProgress(Progress+=1);
    }

    emit jobDone(OutputList, TeoretycznieMozliwe);

}

void Worker::startOpening(QString Path)
{
    QFile File(Path);
    if(!File.open(QFile::ReadOnly))
    {
        emit pointsLoadingFailed(Path);
        return;
    }
    QVector<Point> Points;

    qint64 Bytes = 0;
    QTextStream Stream(&File);
    Stream.setRealNumberPrecision(20);

    setProgressRange(0, File.size());
    setProgress(0);

    while(!Stream.atEnd())
    {
        QString Line = Stream.readLine();
        QString Temp;

        QTextStream LineStream(&Line);
        QStringList Lista = Line.split(' ', QString::SkipEmptyParts);


        if(Lista.length()>7)
        {
            Point NewPoint;

            LineStream >> Temp;
            LineStream >> NewPoint.X;
            LineStream >> NewPoint.Y;
            LineStream >> NewPoint.Wys;
            LineStream >> NewPoint.Kat;
            LineStream >> NewPoint.Flaga;

            LineStream >> NewPoint.Kod;
            LineStream >> NewPoint.Kerg;

            if( !NewPoint.Kod.contains('"') || NewPoint.Kod.contains(QRegExp("\"(?:\\d+\\.?\\d*)?\\|?(?:\\d+\\.?\\d*)\"")) )
                NewPoint.Kod = "BRAK";


            Points.push_back(NewPoint);
        }

        setProgress(Bytes+=Line.toLocal8Bit().size());

    }
    setProgress(File.size());

    File.close();

    emit pointsLoaded(Points);

}



void Worker::startSaving(const QVector<QPair<Point, Point> > Points, QString Path, QString Pattern)
{
    QFile File(Path);
    Pattern.remove(QRegExp(("&.*&")));

    int Progress;
    setProgressRange(0, Points.length());
    if(File.open(QFile::WriteOnly))
    {
        QTextStream Stream(&File);
        Stream.setRealNumberPrecision(20);

        setProgressRange(0, Points.length());

        for(const QPair<Point, Point> & P : Points)
        {
            QString Line = Pattern;

            Line.replace("$X1", QString::number(P.first.X, 'E', 20));
            Line.replace("$Y1", QString::number(P.first.Y, 'E', 20));
            Line.replace("$X2", QString::number(P.second.X, 'E', 20));
            Line.replace("$Y2", QString::number(P.second.Y, 'E', 20));
            Line.replace("$KERG", P.first.Kerg);

            Stream << Line << "\r\n";

            setProgress(Progress+=1);
        }

        emit pointsSaved();
    }
    File.close();

    emit pointsSavingFailed(Path);
}

void Worker::test()
{
    qDebug() << "oks";
}

void Worker::cancelCopmuting()
{
    QMutexLocker MLocker(&Mutex);
    Cancel=true;

    qDebug() << "clicked cancelled";
}
