#ifndef DOPASOWANIE_HPP
#define DOPASOWANIE_HPP

#include "point.hpp"
#include <QVector>
#include <QtMath>
#include <QMap>

#define PARAMS_NUMBERS 4
#define COFITIENTS 3

class Dopasowanie
{
	private:
		QVector<double> Wagi;
		QVector<double> Maksyma;
		Point *A;
		Point *B;		

        struct Param{
            double Waga;
            double WartoscOgraniczenia;
            double Wartosc;
            char TypOgraniczenia;
        };


        QMap<QString, Param> Params = {
            { MAXD, {0, 0, 0, 0} },
            { KAT, {0, 0, 0, 0} },
            { AZYMUT, {0, 0, 0, 0 } },
            { AZYMUT, {0, 0, 0, 0 } },

        };

		double pointDistance();
		double pointKatDifference();
		double pointAzymutDifference();
	public:
		Dopasowanie();
		double ObliczDopasowanie(Point *P1, Point *P2);	

        void setParamsValues(QString ParamName, char Ograniczenie, double Waga, double Value);

		double MaksymalnyWskaznikDop();

        QStringList getParamNames() const;

        static const QString MAXD;
        static const QString MIND;
        static const QString KAT;
        static const QString AZYMUT;
};

#endif // DOPASOWANIE_HPP
