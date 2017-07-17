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
    ui->ButtonCancel->hide();

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
    aktualizujLiczniki();
    aktualizujStanPrzyciskow();

    Mudzin = new Worker();
    Mudzin->moveToThread(&Thread);
    Thread.start();

    connect(this, &MainWindow::requestOpening, Mudzin, &Worker::startOpening);
    connect(this, &MainWindow::requestComputing, Mudzin, &Worker::startComputing);
    connect(this, &MainWindow::requestSaving, Mudzin, &Worker::startSaving);

    connect(this, &MainWindow::requestOpening, this, &MainWindow::distableGui);
    connect(this, &MainWindow::requestComputing, this, &MainWindow::distableGui);
    connect(this, &MainWindow::requestSaving, this, &MainWindow::distableGui);

    connect(Mudzin, &Worker::jobDone, this, &MainWindow::enableGui);
    connect(Mudzin, &Worker::pointsLoaded, this, &MainWindow::enableGui);
    connect(Mudzin, &Worker::pointsLoadingFailed, this, &MainWindow::enableGui);
    connect(Mudzin, &Worker::pointsSaved, this, &MainWindow::enableGui);
    connect(Mudzin, &Worker::pointsSavingFailed, this, &MainWindow::enableGui);
    connect(Mudzin, &Worker::pointsAddedToView, this, &MainWindow::enableGui);
    connect(Mudzin, &Worker::cancelled, this, &MainWindow::enableGui);

    connect(Mudzin, &Worker::jobDone, this, &MainWindow::onJobDone);
    connect(Mudzin, &Worker::pointsLoaded, this, &MainWindow::onPointsLoaded);
    connect(Mudzin, &Worker::pointsSaved, this, &MainWindow::onPointsSaved);
    connect(Mudzin, &Worker::pointsLoadingFailed, this, &MainWindow::onPointsLodingFailed);
    connect(Mudzin, &Worker::cancelled, this, &MainWindow::onJobCancelled);

    connect(Mudzin, &Worker::setProgressRange, ui->ProgressBar, &QProgressBar::setRange );
    connect(Mudzin, &Worker::setProgress, ui->ProgressBar, &QProgressBar::setValue );

    connect(this, &MainWindow::requestComputing, ui->ProgressBar, &QProgressBar::show);
    connect(this, &MainWindow::requestOpening, ui->ProgressBar, &QProgressBar::show);
    connect(this, &MainWindow::requestAppendToList, ui->ProgressBar, &QProgressBar::show);
    connect(this, &MainWindow::requestSaving, ui->ProgressBar, &QProgressBar::show);

    connect(Mudzin, &Worker::jobDone, ui->ProgressBar, &QProgressBar::hide);    
    connect(Mudzin, &Worker::pointsLoaded, ui->ProgressBar, &QProgressBar::hide);
    connect(Mudzin, &Worker::pointsSaved, ui->ProgressBar, &QProgressBar::hide);
    connect(Mudzin, &Worker::pointsLoadingFailed, ui->ProgressBar, &QProgressBar::hide);
    connect(Mudzin, &Worker::pointsSavingFailed, ui->ProgressBar, &QProgressBar::hide);
    connect(Mudzin, &Worker::cancelled, ui->ProgressBar, &QProgressBar::hide);

    connect(this, &MainWindow::requestComputing, ui->ButtonCancel, &QPushButton::show);
    connect(Mudzin, &Worker::jobDone, ui->ButtonCancel, &QPushButton::hide);

    connect(ui->ButtonCancel, &QPushButton::clicked, [this](void) -> void {
       Mudzin->cancelCopmuting();
    });

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

    statusBar()->showMessage(QString("Wczytuje plik: ").append(FilePath));
    this->setWindowTitle(FilePath);

    emit requestOpening(FilePath);
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
}


void MainWindow::initTable()
{
    ui->Table->setColumnCount(7);
    ui->Table->setHorizontalHeaderLabels(Headers);
    ui->Table->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::distableButtons()
{
    ui->ButtonKonwertuj->setDisabled(true);
    ui->ButtonOtworz->setDisabled(true);
    ui->ButtonWyczysc->setDisabled(true);
    ui->ButtonZapisz->setDisabled(true);
}

void MainWindow::enableButtions()
{   
    ui->ButtonOtworz->setEnabled(true);
    aktualizujStanPrzyciskow();
}

void MainWindow::onButtonZapisz()
{
    QString FilePath = QFileDialog::getSaveFileName(this, "Wybierz plik do zapisu");
    if(FilePath.isNull() || FilePath.isEmpty())
        return;

    emit requestSaving(ListaWyjsciowa, FilePath, ui->ComboBox->currentText());
}

void MainWindow::onJobDone(QVector<QPair<Point, Point>> OutputList, int TeoretycznieMozliwe)
{   
    ui->ProgressBar->hide();
    statusBar()->showMessage(QString("Liczba utworzonych par: %1 na %2 teoretycznie możliwych").arg(
                                 QString::number(OutputList.length())).arg(
                                 QString::number(TeoretycznieMozliwe/2))
                             );

    if(ListaWyjsciowa.length() > 0 && QMessageBox::question(this, "Czyszczenie", "Usunąć punkty z poprzedniej konwersji?")==QMessageBox::Yes)
        ListaWyjsciowa=OutputList;
    else
        ListaWyjsciowa.append(OutputList);

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();
}

void MainWindow::onJobCancelled()
{
    ui->StatusBar->showMessage("Anulowano przetwarzanie");
    ui->ButtonCancel->hide();

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();
}


void MainWindow::onPointsLoaded(QVector<Point> P)
{
    statusBar()->showMessage(QString("Liczba wczytanych: %1").arg(QString::number(Points.length())));
    if(P.length() < 1)
    {
        statusBar()->showMessage(QString("Wczytano za mało punków, minimalna liczba punktów: 2"));
        clearPoints();
    }
    else
    {
        ui->Table->setRowCount(0);
        statusBar()->showMessage(QString("Wczytywanie danych do tabeli"));
        Points = P;
        appendToView();
        statusBar()->showMessage(QString("Załadowane punkty: %1").arg(QString::number(Points.length())));
    }

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();

}

void MainWindow::onPointsLodingFailed(QString Path)
{
    statusBar()->showMessage(QString("Nie mozna wczytac pliku: $1").arg(Path));
    QMessageBox::information(this, "Błąd", QString("Nie można otworzyć pliku: $1").arg(Path));
}


void MainWindow::onPointsSaved()
{
    ListaWyjsciowa.clear();

    aktualizujStanPrzyciskow();
    aktualizujLiczniki();

    statusBar()->showMessage(QString("Zapisano plik"));
    this->setWindowTitle("Konwerter");
}

void MainWindow::onPointsSavingFailed(QString Path)
{
    aktualizujStanPrzyciskow();
    aktualizujLiczniki();

    ui->StatusBar->showMessage( QString("Nie można zapisać pliku: %1").arg(Path) );
    this->setWindowTitle("Konwerter");
}

void MainWindow::enableGui()
{
    enableButtions();
}

void MainWindow::distableGui()
{
    distableButtons();
}

void MainWindow::appendToView()
{
    auto Table = ui->Table;
    Table->setRowCount(0);
    for(const Point & Item : Points)
    {
        int row = Table->rowCount();
        Table->insertRow(row);

        Table->setItem(row, 0, new QTableWidgetItem( QString::number(Item.X)  ));
        Table->setItem(row, 1, new QTableWidgetItem( QString::number(Item.Y) ));
        Table->setItem(row, 2, new QTableWidgetItem( QString::number(Item.Wys) ));
        Table->setItem(row, 3, new QTableWidgetItem( QString::number(Item.Kat) ));
        Table->setItem(row, 4, new QTableWidgetItem( QString::number(Item.Flaga) ));
        Table->setItem(row, 5, new QTableWidgetItem( QString(Item.Kod).remove('"')));
        Table->setItem(row, 6, new QTableWidgetItem( Item.Kerg ));
    }
}

void MainWindow::clearPoints()
{    
    Points.clear();
    ui->Table->setRowCount(0);
}

void MainWindow::aktualizujStanPrzyciskow()
{
    ui->ButtonKonwertuj->setEnabled(Points.length());
    ui->ButtonZapisz->setEnabled(ListaWyjsciowa.length());
    ui->ButtonWyczysc->setEnabled(ListaWyjsciowa.length());
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
