//Modified from QTRadio to work in Pebble options dialog

#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QtDebug>
#include <QColor>
#include <QMessageBox>
#include <QDialog>
#include "servers.h"
#include "ui_servers.h"
#include "ghpsdr3device.h"

Servers::Servers(Ghpsdr3Device *_sdr, QWidget *parent) :  QObject()
{
	ui = new Ui::Servers;
	ui->setupUi(parent);

	sdr = _sdr;

	//this->setAttribute(Qt::WA_DeleteOnClose);
	nam = new QNetworkAccessManager(this);
	connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));

	connect(ui->refreshButton,SIGNAL(clicked()),this,SLOT(on_refreshButton_clicked()));
	connect(ui->treelist,SIGNAL(itemClicked(QTreeWidgetItem*,int)),this,SLOT(lineClicked(QTreeWidgetItem*,int)));

	//ui->treelist->setHeaderLabels(QString("Status;Call;Location;Band;Rig;Antenna;Last Report;IP").split(";"));
	//Shorter list for Pebble
	ui->treelist->setColumnCount(5);
	ui->treelist->setColumnWidth(1,50); //Callsign
	ui->treelist->setColumnWidth(2,125); //Location
	ui->treelist->setColumnWidth(3,60); //Status
	//Last col, rig, autosizes to fill
	ui->treelist->setHeaderLabels(QString("IP;Call;Location;Status;Rig").split(";"));
	ui->treelist->setHeaderHidden(false);

	ui->IPEdit->setText(sdr->deviceAddress.toString());
	ui->receiverEdit->setText(QString::number(sdr->devicePort-8000));

	QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(TimerFired()));
    timer->start(60000); // 60 seconds
	refreshList();
}

Servers::~Servers()
{
    delete ui;
}

void Servers::refreshList()
{
   ui->treelist->clear();
   //Gets the delimited data without the html
   QUrl url("http://qtradio.napan.ca/qtradio/qtradiolist.pl");
   QNetworkReply* reply = nam->get(QNetworkRequest(url));
   Q_UNUSED(reply);
}

void Servers::finishedSlot(QNetworkReply* reply )
{
	if (reply->error() == QNetworkReply::NoError) {
        QByteArray replyBytes = reply->readAll();  // bytes
        QString replyString(replyBytes); // string
        //fprintf(stderr, "replyString length:%i\n", replyString.length());
        //qDebug() << replyString;
         QStringList list =  replyString.split("\n", QString::SkipEmptyParts);
         populateList(list);
    }
    // Some http error received
	else {
         fprintf(stderr, "Servers Reply error\n");
    }
    reply->deleteLater();
}

void Servers::on_refreshButton_clicked()
{
    refreshList();

}

void Servers::populateList(QStringList list)
{
    foreach (QString line, list) {
        addLine(line);
    }
}

void Servers::addLine(QString line)
{
	//Would be better to return this in standard JSON format
	//qDebug()<<line;
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->treelist);
    QStringList fields = line.split("~");
	//Set columns in Pebble order
	item->setText(0,fields[FIELD::IP]);
	item->setText(1,fields[FIELD::Call]);
	item->setText(2,fields[FIELD::Location]);
	item->setText(3,fields[FIELD::Status]);
	item->setText(4,fields[FIELD::Rig]);

	//Highlight if no users, means we can get master mode, else slave
    if (line.left(4) == "Idle"|| line.left(1) == "0"){
	  item->setBackgroundColor(3,QColor(Qt::green));
    }
    ui->treelist->addTopLevelItem(item);
}

void Servers::lineClicked(QTreeWidgetItem *item,int col)
{
	Q_UNUSED(col);
	QString IP = item->text(0); //Displayed IP column
	//qDebug() << "IP Selected: " << IP;
	ui->IPEdit->setText(IP);
	ui->receiverEdit->setText("0");
	//Save settings
	sdr->deviceAddress = QHostAddress(IP);
	sdr->devicePort = 8000;
	sdr->WriteSettings();
}

void Servers::TimerFired()
{
	refreshList();
}
