//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "presets.h"
#include <QMenu>
#include <QMessageBox>

Presets::Presets(ReceiverWidget *w)
{
	rw = w;

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    memoryFile = path + "/PebbleData/memory.csv";
    bandsFile = path + "/PebbleData/bands.csv";
    eibiFile = path + "/PebbleData/eibi.csv";

    bands = NULL;
    stations = NULL;
    memories = NULL;

    //Bands MUST be read before stations!!
    if (!ReadBands()) {
        QMessageBox::information(NULL,"Pebble","No bands.csv file!");
    }
    else if(!ReadStations()) {
        QMessageBox::information(NULL,"Pebble","No eibi.csv file!");
    }

}

Presets::~Presets()
{

    if (stations != NULL)
        delete [] stations;
    if (bands != NULL)
        delete [] bands;
    if (memories != NULL)
        delete [] memories;
}

//Utility function for CSV
//Handle EOL as 0x0d, 0x0a, both in any order
//Qt doesn't handle this case with standard readLine
QString Presets::csvReadLine(QFile *file)
{
    char byte;
    QString line;
    while (!file->atEnd()) {
        file->read(&byte,1);
        if (byte == 0x0d || byte == 0x0a)
            return line;
        line.append(byte);
    }
    //Handle end of file
    if (line.length() > 0)
        return line;
    else
        return NULL;
}
//Counts lines for mac or win EOL standard
int Presets::csvCountLines(QFile *file)
{
    char byte;
    int lineCount = 0;
    int charCount = 0;
    file->seek(0); //Start at beginning
    while (!file->atEnd()) {
        file->read(&byte,1);
        charCount++;
        if (byte == 0x0d || byte == 0x0a) {
            lineCount++;
            charCount=0;
        }
    }
    //If last line has character, but no EOL
    if (charCount > 0)
        lineCount ++;

    file->seek(0);
    return lineCount;
}

//Same as QString split, except handles quoted delimters
QStringList Presets::csvSplit(QString line, char split)
{
    QStringList list;
    int len = line.length();
    bool inQuote = false;
    int pos=0;
    int start = 0;
    char c;
    while (pos < len) {
        //c = line.at(pos).toAscii();
        c = line.at(pos).toLatin1();
        //Todo: strip quotes, handle escaped quotes within quotes, other csv exceptions
        if (c == '"') {
            inQuote = !inQuote;
        }
        if (!inQuote && c == split) {
            if (start == pos)
                list.append(""); //Empty
            else
                list.append(line.mid(start,pos-start));
            pos++;
            start = pos;
        } else {
            pos++;
        }
    }
    //Grab trailing part
    if (start == pos)
        list.append(""); //Empty
    else
        list.append(line.mid(start,pos-start));

    //Strip quotes
    list.replaceInStrings("\"","");

    return list;
}

//WARNING: CSV file must have cr/lf endings
//Mac Excel does not save in this format.  On Mac, use TextWrangler and choose Windows EOL
bool Presets::ReadBands()
{
    QFile file(bandsFile);
    if (!file.open(QIODevice::ReadOnly))// | QIODevice::Text))
        return false;

    QString line;
    QStringList parts;
    //How many lines in file
    numBands = csvCountLines(&file);
    if (numBands <= 1) {
        //bands.csv must have at least on line plus header
        numBands = 0;
        file.close();
        return false;
    }
    //1st line is header, don't count
    numBands--;
    //Reset stream
    file.seek(0);
    //Allocate buffer
    bands = new Band[numBands];
    double low;
    //Read file into memory, throw away header line
    line = csvReadLine(&file);
    //qDebug("Bands: %s",line);
    parts = csvSplit(line);
    if (parts.count() != 7) {
        file.close();
        return false; //Not right # columns
    }



    for (int i=0; i<numBands; i++)
    {
        line = csvReadLine(&file);
        //Need to handle escaped delimiters, delimiters in string quotes
        parts = csvSplit(line);
        if (parts.count() != 7)
            continue; //Skip invalid line, should have 7 elements
        //Low freq must have a valid value
        bands[i].low = parts[Band::C_LOW].toDouble();
        bands[i].high = parts[Band::C_HIGH].toDouble();
        bands[i].tune = parts[Band::C_TUNE].toDouble();
        bands[i].name = parts[Band::C_NAME];
        bands[i].bType = StringToBandType(parts[Band::C_BANDTYPE]);
        bands[i].mode = Demod::StringToMode(parts[Band::C_MODE]);
        bands[i].notes = parts[Band::C_NOTES];
        bands[i].stations = NULL;
        bands[i].numStations = 0;
        bands[i].bandIndex = i; //back ref to position in bands[]
    }
    file.close();
    return true;
}

Band *Presets::FindBand(double freq)
{
    if (bands == NULL || freq==0)
        return NULL;

    //Brute force, first only for now
    for (int i=0; i<numBands; i++) {
        if (freq >= bands[i].low && freq < bands[i].high)
            return &bands[i];
    }
    return NULL;
}
int Presets::FindBandIndex(double freq)
{
    if (bands == NULL || freq==0)
        return -1;

    //Brute force, first only for now
    for (int i=0; i<numBands; i++) {
        if (freq >= bands[i].low && freq < bands[i].high)
            return i;
    }
    return -1;
}

int Presets::GetNumBands()
{
    if (bands == NULL)
        return 0;
    else
        return numBands;
}

/*
 * Read all stations and memory into master 'stations' array
 *
 */

bool Presets::ReadStations()
{
    //Bands MUST be read first
    if (bands == NULL)
        return false;

    QFile fEibi(eibiFile);
    if (!fEibi.open(QIODevice::ReadOnly))// | QIODevice::Text))
        return false;
    //How many lines in file
    numEibi = csvCountLines(&fEibi);
    if (numEibi <= 1) {
        //bands.csv must have at least on line plus header
        numEibi = 0;
        fEibi.close();
        return false;
    }
    //1st line is header, don't count
    numEibi--;
    //Reset stream
    fEibi.seek(0);

    QFile fMemory(memoryFile);
    if (!fMemory.open(QIODevice::ReadOnly))// | QIODevice::Text))
        return false;
    //How many lines in file
    numMemory = csvCountLines(&fMemory);
    if (numMemory <= 1) {
        //bands.csv must have at least on line plus header
        numMemory = 0;
        maxMemory = 0;
        fMemory.close();
        return false;
    }
    //1st line is header, don't count
    numMemory--;
    maxMemory = numMemory + 50; //Room to save new memories from UI
    //Reset stream
    fMemory.seek(0);

    QString line;
    QStringList parts;

    //Allocate buffer
    numStations = numEibi + numMemory;
    stations = new Station[numStations];
    memories = new Station[maxMemory];  //More than we need

    //Read file into memory, throw away header line
    line = csvReadLine(&fEibi);
    //qDebug("Bands: %s",line);
    parts = csvSplit(line, ';');
    //Header has an extra ';' in file, don't check exact count, just min
    if (parts.count() < 11) {
        fEibi.close();
        return false; //Not right # columns
    }

    int bandIndex;
    int stationIndex;

    //Process eibi.csv file first.  Then can be updated with new downloads
    for (int i=0; i<numEibi; i++)
    {
        line = csvReadLine(&fEibi);
        //Need to handle escaped delimiters, delimiters in string quotes
        parts = csvSplit(line, ';');

        if (parts.count() != 11)
            continue; //Skip invalid line, should have 7 elements

        stationIndex = i;
        AddStation(stations, stationIndex, parts);

        //Find band for each station
        bandIndex = FindBandIndex(stations[stationIndex].freq);
        stations[stationIndex].bandIndex = bandIndex;
        if (bandIndex != -1) {
            //Count # stations per band, we'll assign in next step
            bands[bandIndex].numStations ++;
        }
    }

    //Add our own memory.csv file entries
    //Throw away 1st line
    line = csvReadLine(&fMemory);
    for (int i=0; i<numMemory; i++)
    {
        line = csvReadLine(&fMemory);
        //Need to handle escaped delimiters, delimiters in string quotes
        parts = csvSplit(line, ','); //Comma for memory.csv

        if (parts.count() != 11)
            continue; //Skip invalid line, should have 7 elements

        //Merge on end of station list
        stationIndex = numEibi + i;
        AddStation(stations, stationIndex, parts);
        AddStation(memories, i,parts); //Keep separate so we can add and update file

        //Find band for each station
        bandIndex = FindBandIndex(stations[stationIndex].freq);
        stations[stationIndex].bandIndex = bandIndex;
        if (bandIndex != -1) {
            //Count # stations per band, we'll assign in next step
            bands[bandIndex].numStations ++;
        }
    }

    //Allocate memory for station array in each band
    for (int i=0; i<numBands; i++) {
        bands[i].stations = new int[bands[i].numStations];
        bands[i].numStations = 0; //Will be re-incremented as we add below
    }

    //One more pass to assign stations to bands
    stationIndex = 0;
    for (int i=0; i<numStations; i++) {
        bandIndex = stations[i].bandIndex;
        if (bandIndex != -1) {
            //Add to stations[] in band
            stationIndex = bands[bandIndex].numStations++;
            bands[bandIndex].stations[stationIndex] = i;
        }
    }

    fEibi.close();
    fMemory.close();
    return true;

}

void Presets::AddStation(Station *s, int i, QStringList parts)
{
    s[i].freq = parts[Station::KHZ].toDouble()*1000;
    s[i].time = parts[Station::TIME];
    s[i].days = parts[Station::DAYS];
    s[i].itu = parts[Station::ITU];
    s[i].station = parts[Station::STATION];
    s[i].language = parts[Station::LNG];
    s[i].target = parts[Station::TARGET];
    s[i].remarks = parts[Station::REMARKS];
    s[i].p = parts[Station::P];
    s[i].start = parts[Station::START];
    s[i].stop = parts[Station::STOP];

}

/*
 * Options
 * Exact Match
 * Within N khz
 * First match
 * All matches
 */
QList<Station> Presets::FindStation(double currentFreq, int fRange)
{
    QList<Station> stationList;
    //Only check at kHz level
    double fKhz = trunc(currentFreq / 1000);
    double fKhzTemp;
    Band *b = FindBand(currentFreq);
    int stationIndex;
    Station s;

    if (b != NULL) {
        int numS = b->numStations;
        for (int i=0; i < numS; i++) {
            stationIndex = b->stations[i];
            if (stationIndex < 0)
                continue; //Invalid
            s = stations[stationIndex];
            //Check if within range
            fKhzTemp = trunc(s.freq / 1000);
            if ((fKhz >= fKhzTemp - fRange) && (fKhz <= fKhzTemp + fRange))
                stationList.append(s);
        }
    }
    return stationList;
}

Band::BANDTYPE Presets::StringToBandType(QString s)
{
    if (s.compare("HAM",Qt::CaseInsensitive)==0)
        return Band::HAM;
    else if (s.compare("SW", Qt::CaseInsensitive)==0)
        return Band::SW;
    else if (s.compare("SCANNER",Qt::CaseInsensitive)==0)
        return Band::SCANNER;

    return Band::OTHER;
}

Band::Band()
{
    low = 0;
    high = 0;
    tune = 0;
    stations = NULL;
    bandIndex = -1;
    numStations = 0;
}
Band::~Band()
{
    if (stations != NULL)
        free (stations);
}
Station::Station()
{
    freq = 0;
    time = "";
    days = "";
    itu = "";
    station = "";
    language = "";
    target = "";
    remarks = "";
    p = "";
    start = "";
    stop = "";
    bandIndex = -1;
}
Station::~Station()
{
}

void Presets::SaveMemoryCSV()
{

    QFile file(memoryFile);
     if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
         return;

     QTextStream out(&file);
     //QTableWidgetItem *cell;
     QString str;
     //Write header row for reference
     //Must be the same format as eibi.csv so we can read them both with same code
     out << "kHz:75,Time(UTC):93,Days:59,ITU:49,Station:20,Lng:49,Target:62,Remarks:135,P:35,Start:60,Stop:60\n";
     for (int i=0; i<numMemory; i++)
     {
         //Not writing decimal
         double temp = memories[i].freq / 1000.0;
         out << QString::number(temp,'f',3);  //Limit to 3 decimal places
         out << ",";
         out << memories[i].time;
         out << ",";
         out << memories[i].days;
         out << ",";
         out << memories[i].itu;
         out << ",";
         out << memories[i].station;
         out << ",";
         out << memories[i].language;
         out << ",";
         out << memories[i].target;
         out << ",";
         out << memories[i].remarks;
         out << ",";
         out << memories[i].p;
         out << ",";
         out << memories[i].start;
         out << ",";
         out << memories[i].stop;
         //Don't output last EOL
         if (i+1 < numMemory)
            out << "\n";
     }
     file.close();
}

bool Presets::AddMemory(double freq, QString station, QString note)
{
    if (numMemory == maxMemory)
        return false; //No more room at the inn
    Station *s = &memories[numMemory]; //Next empty memory
    s->freq = freq;
    s->station = station;
    s->remarks = note;

    numMemory++;
    SaveMemoryCSV();

    return true;
}
