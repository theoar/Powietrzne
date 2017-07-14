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

    Mudzin = new Worker();
    Mudzin->moveToThread(&Thread);
    Thread.start();

    connect(this, &MainWindow::requestComputing, Mudzin, &Worker::startComputing);
    connect(Mudzin, &Worker::setProgressRange, ui->ProgressBar, &QProgressBar::setRange );
    connect(Mudzin, &Worker::setProgress, ui->ProgressBar, &QProgressBar::setValue);
    connect(Mudzin, &Worker::jobDone, this, &MainWindow::onJobDone);
    connect(this, &MainWindow::requestComputing, ui->ProgressBar, &QProgressBar::show);
    connect(Mudzin, &Worker::jobDone, ui->ProgressBar, &QProgressBar::hide);

}

MainWindow::~MainWindow()
{
    Thread.exit();
    Thread.wait();

    clearPoints();

    delete Mudzin;
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
    statusBar()->showMessage( "Konwersja w toku..." );

    Dopasowanie Fitter;
    Fitter.setParamsValues(Dopasowanie::AZYMUT, '<', ui->SpinBoxWagaAzy->value(),ui->SpinBoxAzy->value());
    Fitter.setParamsValues(Dopasowanie::KAT, '<', ui->SpinBoxWagaKat->value(), ui->SpinBoxKat->value());
    Fitter.setParamsValues(Dopasowanie::MAXD, '<', ui->SpinBoxWagaOdl->value(),ui->SpinBoxOdl->value());
    Fitter.setParamsValues(Dopasowanie::MIND, '>', 0, ui->SpinBoxOdlMin->value());


    emit requestComputing(Points, Fitter, ui->CheckBoxRegExp->isChecked(), ui->ChecBoxKerg->isChecked(), ui->KodEdit->text() );

    ui->CentralWidget->setEnabled(false);
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

void MainWindow::distableButtons()
{
    ui->CentralWidget->setEnabled(false);
}

void MainWindow::enableButtions()
{
    ui->CentralWidget->setEnabled(true);
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

void MainWindow::onJobDone(QVector<QPair<Point, Point>> OutputList, int TeoretycznieMozliwe)
{
    ui->CentralWidget->setEnabled(true);
    ui->ProgressBar->hide();
    statusBar()->showMessage(QString("Liczba utworzonych par: %1 na %2 teoretycznie możliwych").arg(
                                 QString::number(OutputList.length())).arg(
                                 QString::number(TeoretycznieMozliwe/2))
                             );

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();
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
