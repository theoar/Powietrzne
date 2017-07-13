#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
    ui->ProgressBar->hide();
	ui->SpinBoxAzy->setSuffix(QString(QChar(' ')) + QString(QChar(0x030A)));
    ui->SpinBoxKat->setSuffix(QString(QChar(' ')) + QString(QChar(0x030A)));

    connect(ui->ButtonWyczysc, &QPushButton::clicked, this, [this](void)->void {
        if(this->ListaWyjsciowa.isEmpty())                {

            this->statusBar()->showMessage("Lista wyjściowa akutalnie pusta");
            return;
        }

        this->statusBar()->showMessage(QString("Usunięte pary: $1").arg(this->ListaWyjsciowa.length()));
        this->ListaWyjsciowa.clear();

        this->aktualizujStanPrzyciskow();
        this->aktualizujLiczniki();
    } );

    fillComobBox();
    initTable();
}

MainWindow::~MainWindow()
{
    clearPoints();

	delete ui;
}

void MainWindow::onButtonOtworz()
{
	QString FilePath = QFileDialog::getOpenFileName(this, "Otwórz plik");

    if(FilePath.isNull() || FilePath.isEmpty())
		return;

	QFile File(FilePath);
	if(!File.open(QFile::ReadOnly))
	{
        statusBar()->showMessage(QString("Nie mozna wczytac pliku: $1").arg(FilePath));
        QMessageBox::information(this, "Błąd", QString("Nie można otworzyć pliku: $1").arg(FilePath));
		return;
	}
    ui->ProgressBar->show();
    ui->ProgressBar->setRange(0, File.size());
    statusBar()->showMessage(QString("Wczytuje plik: ").append(FilePath));

    clearPoints();
    ListaWyjsciowa.clear();

    qint64 Bytes = 0;
	QTextStream Stream(&File);
    Stream.setRealNumberPrecision(20);

    qDebug() << File.size();

	while(!Stream.atEnd())
	{        
        QString Line = Stream.readLine();
        QString Temp;

		QTextStream LineStream(&Line);
        QStringList Lista = Line.split(' ', QString::SkipEmptyParts);

        ui->ProgressBar->setValue(Bytes+=Line.toLocal8Bit().size());

        if(Lista.length()<8)
			continue;

        Point * NewPoint = new Point();        

        LineStream >> Temp;
		LineStream >> NewPoint->X;
		LineStream >> NewPoint->Y;
        LineStream >> NewPoint->Wys;
		LineStream >> NewPoint->Kat;
        LineStream >> NewPoint->Flaga;

        LineStream >> NewPoint->Kod;
        LineStream >> NewPoint->Kerg;

        if( !NewPoint->Kod.contains('"') || NewPoint->Kod.contains(QRegExp("\"(?:\\d+\\.?\\d*)?\\|?(?:\\d+\\.?\\d*)\"")) )
            NewPoint->Kod = "BRAK";

        NewPoint->Kerg = '_';

        Points.push_back(NewPoint);

	}

    this->setWindowTitle(File.fileName());
    File.close();

    statusBar()->showMessage(QString("Liczba wczytanych: %1 z pliku %2").arg(QString::number(Points.length())).arg(FilePath));
    if(Points.length() < 1)
    {
        statusBar()->showMessage(QString("Wczytano za mało punków, minimalna liczba punktów: 2"));
        clearPoints();

    }
    else
    {

        statusBar()->showMessage(QString("Wczytywanie danych do tabeli"));

        Bytes = 0;
        ui->ProgressBar->setRange(0, Points.length());
        for(Point * P : Points)
        {
            insertItem(P);
            ui->ProgressBar->setValue(Bytes+=1);
        }
    }

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();

    ui->ProgressBar->hide();
}

void MainWindow::onButtonKonwertuj()
{	
	QList<QPair<Point*, Point*>> ListaDopasowania;

	Dopasowanie Fitter;
    Fitter.setParamsValues(Dopasowanie::AZYMUT, '<', ui->SpinBoxWagaAzy->value(),ui->SpinBoxAzy->value());
    Fitter.setParamsValues(Dopasowanie::KAT, '<', ui->SpinBoxWagaKat->value(), ui->SpinBoxKat->value());
    Fitter.setParamsValues(Dopasowanie::MAXD, '<', ui->SpinBoxWagaOdl->value(),ui->SpinBoxOdl->value());
    Fitter.setParamsValues(Dopasowanie::MIND, '>', 0, ui->SpinBoxOdlMin->value());

	double MaxWskaznikDopasowania = Fitter.MaksymalnyWskaznikDop();

	QString KodFilter = ui->KodEdit->text();
	bool RegExpEnabled = ui->CheckBoxRegExp->isChecked();
	bool KodFilterEnabled = !KodFilter.isEmpty() && !KodFilter.isNull();
    bool LaczRozneKergi = ui->ChecBoxKerg->isChecked();

	int TeoretycznieMozliwe = 0;
    int Progress = 0;

	statusBar()->showMessage("Konwersja w toku");
    ui->ProgressBar->show();
    ui->ProgressBar->setRange(0, Points.length());

    ListaWyjsciowa.clear();
	for(Point * PointBegin : Points)
	{        
        ui->ProgressBar->setValue(Progress+=1);

		double NajlepszyWskaznikDopasowania = MaxWskaznikDopasowania;
		Point *DopasowanyPunkt = nullptr;   

		if(KodFilterEnabled)
		{
			if(RegExpEnabled)
			{
				QRegExp RegExp(KodFilter);
				if(!PointBegin->Kod.contains(RegExp))
					continue;
			}
			else
			{
				if(!(PointBegin->Kod == KodFilter))
					continue;
			}
		}        
        TeoretycznieMozliwe++;

		for(Point * PointEnd : Points)
		{
			if(PointBegin==PointEnd)
                continue;

            if(PointBegin->Kod!=PointEnd->Kod)
                continue;

            if(PointBegin->Kerg!=PointEnd->Kerg && !LaczRozneKergi)
                continue;

			if(KodFilterEnabled)
			{
				if(RegExpEnabled)
				{
					QRegExp RegExp(KodFilter);
					if(!PointEnd->Kod.contains(RegExp))
						continue;
				}
				else
				{
					if(!(PointEnd->Kod == KodFilter))
						continue;
				}
			}

			double NowyWskaznikDopasowania = Fitter.ObliczDopasowanie(PointBegin, PointEnd);

			if(NowyWskaznikDopasowania<0)
				continue;

			if(NowyWskaznikDopasowania<NajlepszyWskaznikDopasowania)
			{
				DopasowanyPunkt=PointEnd;
				NajlepszyWskaznikDopasowania = NowyWskaznikDopasowania;
			}
        }

		if(DopasowanyPunkt!=nullptr)
			ListaDopasowania.push_back(QPair<Point*, Point*>(PointBegin, DopasowanyPunkt));

	}


    Progress=0;

    ui->ProgressBar->setRange(Progress, ListaDopasowania.length());
    for(QPair<Point*, Point*> & P : ListaDopasowania)
	{       
        ui->ProgressBar->setValue(Progress+1);

        if(P.first==nullptr && P.second==nullptr)
        {
            qDebug() << "nulle";
            continue;
        }

        if(ListaDopasowania.contains(QPair<Point*, Point*>(P.second, P.first)))
        {
            if(LaczRozneKergi && P.first->Kerg!=P.second->Kerg)
            {
                P.first->Kerg=fitKerg(P.first, P.second);
                P.second->Kerg=P.first->Kerg;
            }            
             ListaWyjsciowa.push_back(P);

             P.first=nullptr;
             P.second=nullptr;
        }
	}

	statusBar()->showMessage(QString("Liczba utworzonych par: %1 na %2 teoretycznie możliwych").arg(
							QString::number(ListaWyjsciowa.length())).arg(
							QString::number(TeoretycznieMozliwe/2))
						);

    ui->ProgressBar->hide();

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();

}

void MainWindow::insertItem(Point *Item)
{
    int row = ui->Table->rowCount();
    ui->Table->insertRow(row);

    ui->Table->setItem(row, 0, new QTableWidgetItem( QString::number(Item->X)  ));
    ui->Table->setItem(row, 1, new QTableWidgetItem( QString::number(Item->Y) ));
    ui->Table->setItem(row, 2, new QTableWidgetItem( QString::number(Item->Wys) ));
    ui->Table->setItem(row, 3, new QTableWidgetItem( QString::number(Item->Kat) ));
    ui->Table->setItem(row, 4, new QTableWidgetItem( QString::number(Item->Flaga) ));
    ui->Table->setItem(row, 5, new QTableWidgetItem( Item->Kod.remove('"')));
    ui->Table->setItem(row, 6, new QTableWidgetItem( Item->Kerg ));
}

void MainWindow::initTable()
{
    ui->Table->setColumnCount(7);
    ui->Table->setHorizontalHeaderLabels(Headers);
    ui->Table->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::onButtonZapisz()
{
	QString FilePath = QFileDialog::getSaveFileName(this, "Wybierz plik do zapisu");
	QFile File(FilePath);

    QString Pattern = ui->ComboBox->currentText();
    Pattern.remove(QRegExp(("&.*&")));

	if(File.open(QFile::WriteOnly))
	{
		QTextStream Stream(&File);        
        Stream.setRealNumberPrecision(20);

        ui->ProgressBar->show();
        ui->ProgressBar->setRange(0, ListaWyjsciowa.length());

		for(const QPair<Point*, Point*> & P : ListaWyjsciowa)
		{
            QString Line = Pattern;

            Line.replace("$X1", QString::number(P.first->X, 'E', 20));
            Line.replace("$Y1", QString::number(P.first->Y, 'E', 20));
            Line.replace("$X2", QString::number(P.second->X, 'E', 20));
            Line.replace("$Y2", QString::number(P.second->Y, 'E', 20));
            Line.replace("$KERG", P.first->Kerg);

            Stream << Line << "\r\n";

            ui->ProgressBar->setValue( ui->ProgressBar->value()+1 );
		}
    }

	else
	{
		statusBar()->showMessage(QString("Nie można otworzyć pliku: %1 do zapisu").arg(FilePath));
        QMessageBox::information(this, QString("Błąd"), QString("Nie można otworzyć pliku: %1 do zapisu").arg(FilePath));
		return;
	}
    File.close();

    ListaWyjsciowa.clear();
    aktualizujStanPrzyciskow();
    aktualizujLiczniki();

    ui->ProgressBar->hide();
    statusBar()->showMessage(QString("Zapisano plik %1").arg(FilePath));
    this->setWindowTitle("Konwerter");
}


void MainWindow::clearPoints()
{
	for(Point * P : Points)
		delete P;
	Points.clear();

   ui->Table->setRowCount(0);
}

void MainWindow::aktualizujStanPrzyciskow()
{
    ui->ButtonKonwertuj->setEnabled(Points.length()>1);
    ui->ButtonZapisz->setEnabled(ListaWyjsciowa.length());
}

void MainWindow::aktualizujLiczniki()
{
    ui->LcdPNumber->display(Points.length());
    ui->LcdKNumber->display(ListaWyjsciowa.length());
}

void MainWindow::fillComobBox()
{
    for(auto X : WyjscieWzor)
        ui->ComboBox->insertItem(-1, X);

}

QString MainWindow::fitKerg(Point *A, Point *B)
{

    int RokA, RokB;
    int NumA, NumB;
    int IndexNumer, IndexRok;

    QRegExp Reg("#P\\.\\d+\\.(\\d+)\\.(\\d+)");

    if(A->Kerg=="_")
        return B->Kerg;
    if(B->Kerg=="_")
        return A->Kerg;

    if(A->Kerg.contains(Reg) && !B->Kerg.contains(Reg))
        return A->Kerg;
    if(B->Kerg.contains(Reg) && !A->Kerg.contains(Reg))
        return B->Kerg;

    if(A->Kerg.contains(Reg) && B->Kerg.contains(Reg))
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

    Reg.indexIn(A->Kerg);
    QString Rok = Reg.cap(IndexRok);
    RokA = Reg.cap(IndexRok).toInt();
    NumA = Reg.cap(IndexNumer).toInt();

    if(Rok[0]=='0')
        RokA+=2000;
    else
        if(Rok.length()<3)
            RokA+=1900;

    Reg.indexIn(B->Kerg);
    Rok = Reg.cap(IndexRok);
    RokB = Reg.cap(IndexRok).toInt();
    NumB = Reg.cap(IndexNumer).toInt();

    if(Rok[0]=='0')
        RokB+=2000;
    else
        if(Rok.length()<3)
            RokB+=1900;

    if(RokA>RokB)
        return A->Kerg;
    if(RokB>RokA)
        return B->Kerg;
    if(NumA>NumB)
        return A->Kerg;
    if(NumB>NumA)
        return B->Kerg;

    return A->Kerg;

}

