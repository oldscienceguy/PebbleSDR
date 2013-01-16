#ifndef PRESETS_H
#define PRESETS_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QtCore>
#include "receiverWidget.h"
#include "demod.h"

/*
    EIBI CSV format - semi-colon delimted file
    kHz:75;Time(UTC):93;Days:59;ITU:49;Station:201;Lng:49;Target:62;Remarks:135;P:35;Start:60;Stop:60;
    Format of the CSV database (sked-):
    Entry #1 (float between 150 and 30000): Frequency in kHz
    Entry #2 (9-character string): Start and end time, UTC, as two '(I04)' numbers separated by a '-'
    Entry #3 (string of up to 5 characters): Days of operation, or some special comments:
             Mo,Tu,We,Th,Fr,Sa,Su - Days of the week
             1245 - Days of the week, 1=Monday etc. (this example is "Monday, Tuesday, Thursday, Friday")
             irr - Irregular operation
             alt - Alternative frequency, not usually in use
             Ram - Ramadan special schedule
             Haj - Special broadcast for the Haj
             15Sep - Broadcast only on 15 September
             RSHD - Special broadcast on Radio St.Helena Day, once a year, usu. Oct/Nov
             I,II,etc. - Month in Roman numerals (IX=September etc.)
    Entry #4 (string up to 3 characters): ITU code (see below) of the station's home country
    Entry #5 (string up to 23 characters): Station name
    Entry #6 (string up to 3 characters): Language code
    Entry #7 (string up to 3 characters): Target-area code
    Entry #8 (string): Transmitter-site code
    Entry #9 (integer): Persistence code, where:
             1 = This broadcast is everlasting (i.e., it will be copied into next season's file automatically)
             2 = As 1, but with a DST shift (northern hemisphere)
             3 = As 1, but with a DST shift (southern hemisphere)
             4 = As 1, but active only in the winter season
             5 = As 1, but active only in the summer season
             6 = Valid only for part of this season; dates given in entries #10 and #11. Useful to re-check old logs.
             8 = Inactive entry, not copied into the bc and freq files.
    Entry #10: Start date if entry #9=6. 0401 = 4th January. Sometimes used with #9=1 if a new permanent service is started, for information purposes only.
    Entry #11: As 10, but end date.

*/
class Station
{
public:
    Station();
    ~Station();

    enum COLS {KHZ,TIME,DAYS,ITU,STATION,LNG,TARGET,REMARKS,P,START,STOP};
    double freq;  //In Hz
    QString time;
    QString days;
    QString itu;
    QString station;
    QString language;
    QString target;
    QString remarks;
    QString p;
    QString start;
    QString stop;
    int bandIndex; //Link to Bands[]
};

/*
  bands.csv records
*/
class Band
{
public:
    Band();
    ~Band();
    //ALL only used for menu, not in csv
    enum BANDTYPE {ALL,HAM,SW,SCANNER,OTHER};
    //Bands.csv column order
    //Low	High	Tune	Name	Type	Mode	Notes
    enum COLS {C_LOW = 0,C_HIGH,C_TUNE,C_NAME,C_BANDTYPE,C_MODE,C_NOTES};

    QString name;
    double low; //Band range, or single frequency
    double high;
    double tune; //Default freq when user switches to band
    BANDTYPE bType;
    DEMODMODE mode;
    QString notes;
    //List of stations in band
    int *stations; //Array of indices to stations[]
    int numStations;
    int bandIndex; //Reference back to bands[]
};


class Presets
{
    //Q_OBJECT

public:

	static const int GROW_PRESETS = 1;
    Presets(ReceiverWidget *rw);
	~Presets();
    bool ReadBands();
    //Returns record(s) that match current frequency
    Band *FindBand(double currentFreq); //Return a Band object
    int FindBandIndex(double currentFreq); // Returns index of band in bands[]
    Band *GetBands() {return bands;}
    int GetNumBands();

    bool ReadStations();
    Station *FindStation(double currentFreq, int fRange = 0);
    Station *GetStations(){return stations;}

    Band::BANDTYPE StringToBandType(QString s);  //Converts string from file to type

    void SaveMemoryCSV();
    //Add current freq to memories
    bool AddMemory(double freq, QString station, QString note);

private:
    QString csvReadLine(QFile *file);
    int csvCountLines(QFile *file);
    QStringList csvSplit(QString line, char split = ',');

    QString memoryFile;
    int numMemory;
    int maxMemory; //max size of memories array

    Station *memories;

    QString bandsFile;
    Band *bands;
    int numBands;

    void AddStation(Station *s, int i, QStringList parts);

    QString eibiFile;
    int numEibi;

    Station *stations; //eibi.csv + memory.csv
    int numStations;

	ReceiverWidget *rw;
};

#endif // PRESETS_H
