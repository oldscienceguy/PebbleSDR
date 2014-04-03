#ifndef SERVERS_H
#define SERVERS_H
#include <QNetworkReply>
#include <QtCore>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QDialog>
#endif

namespace Ui {
    class Servers;
}
class UI;
class Servers : public QObject
{
    Q_OBJECT

public:
	explicit Servers(QWidget *parent = 0 );

    ~Servers();
    void refreshList();
signals:

private slots:
    void finishedSlot(QNetworkReply* reply);
    void on_refreshButton_clicked();
    void TimerFired();
	void lineClicked();

private:
    Ui::Servers *ui;
	QNetworkAccessManager* nam;

    void populateList(QStringList list);
    void addLine(QString line);

};

#endif // SERVERS_H


