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

void Worker::startComputing(const QVector<Point*> &Points,  Dopasowanie Dop, bool RegExpEnable, bool DifferentKerg, QString Filter)
{
    bool KodFilterEnabled = !Filter.isEmpty() && !Filter.isNull();
    double MaxWskaznikDopasowania = Dop.MaksymalnyWskaznikDop();

    QVector<QPair<int, int>> ListaDopasowania;

    int TeoretycznieMozliwe = 0;
    int Progress = 0;

    emit setProgressRange(0 , Points.length());

    for(int X = 0; X<Points.length(); ++X)
    {
        Point &PointBegin = *(Points[X]);
        int DopasowanyPunkt = -1;
        double NajlepszyWskaznikDopasowania = MaxWskaznikDopasowania;

        QRegExp RegExp(Filter);
        if( !(KodFilterEnabled && ((RegExpEnable && !PointBegin.Kod.contains(RegExp) ) || (!RegExpEnable && !(PointBegin.Kod == Filter)))) )
        {
            for(int Y = 0; Y<Points.length(); ++Y)
            {
                Point &PointEnd = *(Points[Y]);
                if( X==Y || (PointBegin.Kod!=PointEnd.Kod) || (PointBegin.Kerg!=PointEnd.Kerg && !DifferentKerg) ||
                        ( KodFilterEnabled && ((RegExpEnable && !PointBegin.Kod.contains(RegExp)) || (!RegExpEnable && !(PointBegin.Kod == Filter)))) )
                    continue;

                double NowyWskaznikDopasowania = Dop.ObliczDopasowanie(PointBegin, PointEnd);
                if(NowyWskaznikDopasowania>0 && NowyWskaznikDopasowania<NajlepszyWskaznikDopasowania)
                {
                    DopasowanyPunkt=Y;
                    NajlepszyWskaznikDopasowania = NowyWskaznikDopasowania;
                }
            }

            if(DopasowanyPunkt)
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
        if(ParaPierwsza.first && ParaPierwsza.second)
        {
            for(int Y = 0; Y<ListaDopasowania.length(); ++Y)
            {
                QPair<int, int> & ParaDruga = ListaDopasowania[Y];
                if(X!=Y && ParaDruga.second==ParaPierwsza.first && ParaDruga.first==ParaPierwsza.second)
                {
                    Point P1 = *(Points[ParaPierwsza.first]);
                    Point P2 = *(Points[ParaPierwsza.second]);
                    ParaDruga.first = ParaPierwsza.first = ParaDruga.second = ParaPierwsza.second = -1;
                    P1.Kerg = P2.Kerg = fitKerg(P1, P2);
                    OutputList.push_back({P1, P2});
                    break;
                }
            }
        }
        setProgress(Progress+=1);
    }

    emit jobDone(OutputList, TeoretycznieMozliwe);

}

void Worker::test()
{
 qDebug() << "oks";
}
