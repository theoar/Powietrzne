#include "dopasowanie.hpp"

const QString Dopasowanie::MAXD = "MAXD";
const QString Dopasowanie::MIND = "MIND";
const QString Dopasowanie::KAT = "KAT";
const QString Dopasowanie::AZYMUT = "AZYMUT";

Dopasowanie::Dopasowanie()
{
	A = nullptr;
	B = nullptr;	
}

double Dopasowanie::ObliczDopasowanie(Point* P1, Point* P2)
{
	if(P1==nullptr || P2==nullptr)
		return -1;

	A=P1;
	B=P2;

    Params[KAT].Wartosc = pointKatDifference();
    Params[AZYMUT].Wartosc = pointAzymutDifference();
    Params[MAXD].Wartosc = pointDistance();
    Params[MIND].Wartosc = pointDistance();

    for(auto X : Params.keys())
	{
        char Typ = Params[X].TypOgraniczenia;
        switch(Typ)
        {
        case '>':
            if(!(Params[X].Wartosc>Params[X].WartoscOgraniczenia))
                return -1;
            break;
        case '<':
            if(!(Params[X].Wartosc<Params[X].WartoscOgraniczenia))
                return -1;
            break;
        case '=':
            if(!(Params[X].Wartosc==Params[X].WartoscOgraniczenia))
                return -1;
            break;
        case '!':
            if(!(Params[X].Wartosc!=Params[X].WartoscOgraniczenia))
                return -1;
            break;
        }
	}

	double SumaWag = 0;
	double SumaWartosci = 0;
    for(auto X : Params.keys())
	{
        SumaWag+=Params[X].Waga;
        SumaWartosci+=Params[X].Waga*Params[X].Wartosc;
	}

	A=nullptr;
	B=nullptr;

    if(SumaWag==0)
        return -1;

	return SumaWartosci/SumaWag;
}

//first ograniczenie, second value
void Dopasowanie::setParamsValues(QString ParamName, char Ograniczenie, double Waga, double Value)
{
    Params[ParamName].TypOgraniczenia=Ograniczenie;
    Params[ParamName].WartoscOgraniczenia=Value;
    Params[ParamName].Waga = Waga;
}

double Dopasowanie::MaksymalnyWskaznikDop()
{
	double SumaWag = 0;
	double SumaWartosci = 0;
    for(auto X : Params.keys())
    {
        SumaWag+=Params[X].Waga;
        SumaWartosci+=Params[X].WartoscOgraniczenia*Params[X].Waga;
    }

    if(SumaWag==0)
        return -1;

	return SumaWartosci/SumaWag;
}

QStringList Dopasowanie::getParamNames() const
{
    return QStringList({MAXD, MIND, AZYMUT, KAT});
}

double Dopasowanie::pointDistance()
{
    return sqrt(pow(A->X-B->X,2)+pow(A->Y-B->Y,2));
}

double Dopasowanie::pointKatDifference()
{
	return qFabs(qFabs(A->Kat-B->Kat)-180);
}

double Dopasowanie::pointAzymutDifference()
{
	double DeltaX = B->X-A->X;
	double DeltaY = B->Y-A->Y;
	double Azymut = 0;

	double temp = DeltaX/DeltaY;
	double KatAB =qFabs(atan(qFabs(temp)));
	KatAB = qRadiansToDegrees(KatAB);

	if(DeltaY>0 && DeltaX<0)
		Azymut = KatAB;
	if(DeltaY<0 && DeltaX<0)
		Azymut = 180-KatAB;
	if(DeltaY<0 && DeltaX>0)
		Azymut = KatAB+180;
	if(DeltaY>0 && DeltaX>0)
		Azymut = 360-KatAB;

	return qFabs(Azymut-A->Kat);
}

