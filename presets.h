#ifndef PRESETS_H
#define PRESETS_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QtCore>
#include <QDockWidget>
#include "ui_presets.h"
#include "receiverWidget.h"
#include "demod.h"

class Preset
{
public:
	Preset();
	~Preset();
	QString name;
	double low; //Band range, or single frequency
	double high;
	DEMODMODE mode;
	QDateTime schedule; //Switch to a preset at specified time, or what's available now
	QString notes;
};

class Presets : public QDockWidget
{
	Q_OBJECT

public:
	static const int GROW_PRESETS = 1;
	Presets(ReceiverWidget *rw,QWidget * parent = 0);
	~Presets();
	void CreateTable(double low, double high);

private:
	Preset *presets; //Array of presets, with some room at end for growth
	int numPresets; //Number of presets
	int presetsLen; //Actual size of presets array

	Ui::PresetsDockWidget ui;
	ReceiverWidget *rw;
	QString presetsFile;
	void Save();
	void Read();
	//Sets columns, row height, and anything else that is common to all rows
	void NewRow(int row, int index, Preset p);
	bool editMode; //True if we're editing entries, false if we're selecting them to set freq
	int editRow; //Row being edited, limit clicks to this row
	QBrush editBrush; 
	private slots:
		void cellClicked(int row, int col);
		void newClicked(bool b);
		void saveClicked(bool b);
		void editClicked(bool b);
		void deleteClicked(bool b);
};

#endif // PRESETS_H
