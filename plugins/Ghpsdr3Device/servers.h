#ifndef SERVERS_H
#define SERVERS_H
#include <QNetworkReply>
#include <QtCore>
#include <QTreeWidget>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QDialog>
#endif

class Ghpsdr3Device;

namespace Ui {
    class Servers;
}
class UI;
class Servers : public QObject
{
    Q_OBJECT

public:
	explicit Servers(Ghpsdr3Device *_sdr, QWidget *_parent = 0 );

    ~Servers();
    void refreshList();
signals:

private slots:
    void finishedSlot(QNetworkReply* reply);
    void on_refreshButton_clicked();
    void TimerFired();
	void lineClicked(QTreeWidgetItem *item, int col);

private:
	//Order of fields returned by http query
	enum FIELD {
		Status = 0,
		Call,
		Location,
		Band,
		Rig,
		Antenna,
		LastReport,
		IP
	};

	Ghpsdr3Device *sdr;

	QWidget *parent;
    Ui::Servers *ui;
	QNetworkAccessManager* nam;

    void populateList(QStringList list);
    void addLine(QString line);

};

#endif // SERVERS_H


