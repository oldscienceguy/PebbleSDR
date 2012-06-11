#ifndef PRESETS_H
#define PRESETS_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QtCore>
#include <QDockWidget>
#include "ui/ui_presets.h"
#include "receiverWidget.h"
#include "demod.h"

class Preset
{
public:
    //ALL only used for menu, not in csv
    enum TYPE {ALL,HAM,SW,SCANNER,OTHER};
	Preset();
	~Preset();
	QString name;
	double low; //Band range, or single frequency
	double high;
    double tune; //Default freq when user switches to band
    TYPE type;
	DEMODMODE mode;
    //For Station source like EIBI
    QDateTime startTime;
    QDateTime endTime;
	QString notes;
};

class Presets : public QDockWidget
{
	Q_OBJECT

public:
    //Bands.csv column order
    //Low	High	Tune	Name	Type	Mode	Notes
    enum BTYPE {B_LOW = 0,B_HIGH,B_TUNE,B_NAME,B_TYPE,B_MODE,B_NOTES};

	static const int GROW_PRESETS = 1;
	Presets(ReceiverWidget *rw,QWidget * parent = 0);
	~Presets();
	void CreateTable(double low, double high);
    bool ReadBands();
    //Returns record(s) that match current frequency
    Preset *FindBand(double currentFreq);
    Preset *GetBands() {return bands;}
    int GetNumBands() {return numBands;}

    bool ReadStations();
    Preset *FindStation(double currentFreq);

    bool ReadPresets();
    Preset *FindPreset(double currentFreq);

    Preset::TYPE StringToPresetType(QString s);  //Converts string from file to type
private:
    QString presetsFile;
    Preset *presets; //Array of presets, with some room at end for growth
	int numPresets; //Number of presets
	int presetsLen; //Actual size of presets array

    QString bandsFile;
    Preset *bands;
    int numBands;

    QString stationsFile;
    Preset *stations;
    int numStations;

	Ui::PresetsDockWidget ui;
	ReceiverWidget *rw;
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
