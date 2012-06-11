//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "presets.h"
#include <QMenu>

enum COL {REC=0,EDIT,NAME,FREQ,MODE,NOTES};
Presets::Presets(ReceiverWidget *w, QWidget *parent)
	: QDockWidget(parent)
{
	rw = w;

	ui.setupUi(this);
	//setFixedWidth(300);
	ui.dockWidgetContents->setStyleSheet("background-color: rgb(200,200,200)");

	ui.tableWidget->setColumnCount(6);
	//Hides the vertical header, ie row lables 1,2,3,etc
	ui.tableWidget->verticalHeader()->setVisible(false);
	//Hide REC col, has index to presets array
	ui.tableWidget->setColumnHidden(REC,true);

	ui.tableWidget->setColumnWidth(EDIT,12);
	ui.tableWidget->setColumnWidth(NAME,60);
	ui.tableWidget->setColumnWidth(FREQ,70); //Size to #char and font
	ui.tableWidget->setColumnWidth(MODE,35);
	ui.tableWidget->setColumnWidth(NOTES,100);
	//This was hard to find, it sets the last col of the header to stretch to width of window
	ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
	//Another hidden function, turns off spreadsheet like selection
	//Don't need this because we're not setting each cell in NewRow
	//ui.tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
	 
	connect(ui.tableWidget,SIGNAL(cellClicked(int,int)),this,SLOT(cellClicked(int,int)));
	connect(ui.newButton,SIGNAL(clicked(bool)),this,SLOT(newClicked(bool)));
	connect(ui.saveButton,SIGNAL(clicked(bool)),this,SLOT(saveClicked(bool)));

	QStringList header;
	header << "Rec"<<""<<"Name"<<"Freq"<<"Mode"<<"Notes";
	ui.tableWidget->setHorizontalHeaderLabels(header);

	QFont tableFont;
	tableFont.setFamily("MS Shell Dlg 2");
	tableFont.setPointSize(8);
	ui.tableWidget->setFont(tableFont);
	ui.tableWidget->horizontalHeader()->setFont(tableFont);

	presetsFile = QCoreApplication::applicationDirPath()+"/presets.csv";
    bandsFile = QCoreApplication::applicationDirPath()+"/bands.csv";
    ReadBands();

	Read();

	editMode = false;
#if (0)
	//Test data
	int row=0;
	ui.tableWidget->setItem(row,COL::FREQ,new QTableWidgetItem("146790000"));
	ui.tableWidget->setItem(row++,COL::MODE,new QTableWidgetItem("FMN"));
	ui.tableWidget->setItem(row,COL::FREQ,new QTableWidgetItem("155565000"));
	ui.tableWidget->setItem(row++,COL::MODE,new QTableWidgetItem("FMN"));
	ui.tableWidget->setItem(row,COL::FREQ,new QTableWidgetItem("146640000"));
	ui.tableWidget->setItem(row++,COL::MODE,new QTableWidgetItem("FMN"));
	ui.tableWidget->setItem(row,COL::FREQ,new QTableWidgetItem("147180000"));
	ui.tableWidget->setItem(row++,COL::MODE,new QTableWidgetItem("FMN"));
	ui.tableWidget->setItem(row,COL::FREQ,new QTableWidgetItem("135650000"));
	ui.tableWidget->setItem(row++,COL::MODE,new QTableWidgetItem("AM"));
#endif
}

Presets::~Presets()
{

}
//Creates table, with presets filtered to range
void Presets::CreateTable(double low, double high)
{
	//Make sure table is empty
	ui.tableWidget->clearContents();
	ui.tableWidget->setRowCount(0);
	//Turn off sorting while inserting so things don't move around
	ui.tableWidget->setSortingEnabled(false);
	int row = ui.tableWidget->rowCount();
	for (int i=0; i<numPresets; i++)
	{
		if (presets[i].low >= low && presets[i].low <= high) {
			ui.tableWidget->insertRow(row);
			NewRow(row,i,presets[i]);
			row++;
		}
	}
	ui.tableWidget->setSortingEnabled(true);
    ui.tableWidget->sortByColumn(0,Qt::AscendingOrder);
}

//WARNING: CSV file must have cr/lf endings
//Mac Excel does not save in this format.  On Mac, use TextWrangler and choose Windows EOL
bool Presets::ReadBands()
{
    QFile file(bandsFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    QString line;
    QStringList parts;
    //How many lines in file
    numBands = 0;
    while (!in.atEnd())
    {
        //This sometimes reads whole file as one line
        line = in.readLine();
        numBands++;
    }
    if (numBands <= 1) {
        //bands.csv must have at least on line plus header
        file.close();
        return false;
    }
    //1st line is header, don't count
    numBands--;
    //Reset stream
    in.seek(0);
    //Allocate buffer
    bands = new Preset[numBands];
    double low;
    //Read file into memory, throw away header line
    line = in.readLine();
    //qDebug("Bands: %s",line);
    parts = line.split(",");
    if (parts.count() != 7) {
        file.close();
        return false; //Not right # columns
    }



    for (int i=0; i<numBands; i++)
    {
        line = in.readLine();
        //Need to handle escaped delimiters, delimiters in string quotes
        parts = line.split(",");
        if (parts.count() != 7)
            continue; //Skip invalid line, should have 7 elements
        //Low freq must have a valid value
        bands[i].low = parts[B_LOW].toDouble();
        bands[i].high = parts[B_HIGH].toDouble();
        bands[i].tune = parts[B_TUNE].toDouble();
        bands[i].name = parts[B_NAME];
        bands[i].type = StringToPresetType(parts[B_TYPE]);
        bands[i].mode = Demod::StringToMode(parts[B_MODE]);
        bands[i].notes = parts[B_NOTES];
    }
    file.close();
    return true;
}

Preset *Presets::FindBand(double freq)
{
    if (bands == NULL)
        return NULL;

    //Brute force, first only for now
    for (int i=0; i<numBands; i++) {
        if (freq >= bands[i].low && freq <= bands[i].high)
            return &bands[i];
    }
    return NULL;
}

bool Presets::ReadStations()
{
    return false;
}

Preset *Presets::FindStation(double currentFreq)
{
    return NULL;
}

bool Presets::ReadPresets()
{
    return false;
}

Preset *Presets::FindPreset(double currentFreq)
{
    return NULL;
}

Preset::TYPE Presets::StringToPresetType(QString s)
{
    if (s.compare("HAM",Qt::CaseInsensitive)==0)
        return Preset::HAM;
    else if (s.compare("SW", Qt::CaseInsensitive)==0)
        return Preset::SW;
    else if (s.compare("SCANNER",Qt::CaseInsensitive)==0)
        return Preset::SCANNER;

    return Preset::OTHER;
}
void Presets::cellClicked(int r, int c)
{
	if (c == EDIT) {
		QMenu menu;
		if (!editMode) {
		menu.addAction("Edit (click again when done)",this,SLOT(editClicked(bool)));
		menu.addAction("Delete",this,SLOT(deleteClicked(bool)));
		} else {
			menu.addAction("Done",this,SLOT(editClicked(bool)));
		}
		menu.exec(QCursor::pos());
		return;
	}

	ui.tableWidget->selectRow(r);
	//double freq = ui.tableWidget->item(r,COL::FREQ)->data(Qt::DisplayRole).toDouble();
	int index = ui.tableWidget->item(r,REC)->data(Qt::DisplayRole).toInt();
	double freq = presets[index].low;
	if (freq == 0)
		return;
	//Tell receiver to change freq and mode
	rw->setLoMode(true); //Can't change if we're in mixer mode
	rw->SetFrequency(presets[index].low); 
	rw->SetMode(presets[index].mode);
	
}
void Presets::newClicked(bool b)
{
	if (editMode)
		return; //Can't add while editing

	//Do we have enough room in presets array
	if (numPresets == presetsLen) {
		//Expand array, We could do this more efficiently with an array of pointers to presets
		//But keep it simple for initial implementation
		int newPresetsLen = presetsLen + GROW_PRESETS;
		Preset *newPresets = new Preset[newPresetsLen];
		for (int i=0; i<newPresetsLen; i++) {
			if (i < numPresets)
				newPresets[i] = presets[i]; //Copy
			else
				newPresets[i] = Preset(); //Empty preset
		}
		//Todo: Memory leak, old presets[] is not freed
		//free (presets); //Crashes
		presets = newPresets;
		presetsLen = newPresetsLen;
	}
	int row = ui.tableWidget->rowCount();
	int index = numPresets;
	ui.tableWidget->insertRow(row);
	//Get current freq and mode and assume that's what we want to save
	presets[index].low = rw->GetFrequency();
	presets[index].mode = rw->GetMode();
	presets[index].name = "New"; //Todo: Insert date or something else useful in Notes field?
	numPresets++;
	ui.tableWidget->setSortingEnabled(false); //Critical, else table sorts after we add first item
	NewRow(row, index, presets[index]);
	ui.tableWidget->selectRow(row); //Fixes bug if no previous row selected
	editClicked(true); //Allow edit
	//Edit will turn sorting back on when done, otherwise new item is not visible to user
	//ui.tableWidget->setSortingEnabled(true);


}
void Presets::editClicked(bool b)
{
	if (editMode) {
		//Update item
		int index = ui.tableWidget->item(editRow,REC)->data(Qt::DisplayRole).toInt();
		presets[index].name = ui.tableWidget->item(editRow,NAME)->data(Qt::DisplayRole).toString();
		presets[index].notes = ui.tableWidget->item(editRow,NOTES)->data(Qt::DisplayRole).toString();
		presets[index].low = ui.tableWidget->item(editRow,FREQ)->data(Qt::DisplayRole).toDouble();
		QString qs = ui.tableWidget->item(editRow,MODE)->data(Qt::DisplayRole).toString();
		//Convert to mode and return something valid regardless of what was typed
		DEMODMODE m = Demod::StringToMode(qs);
		presets[index].mode = m;



		//Reset table row
		QTableWidgetItem *twi = ui.tableWidget->item(editRow,NAME);
		twi->setFlags(Qt::ItemIsEnabled);
		twi->setBackground(editBrush);
		twi = ui.tableWidget->item(editRow,FREQ);
		twi->setFlags(Qt::ItemIsEnabled);
		twi->setBackground(editBrush);
		twi = ui.tableWidget->item(editRow,MODE);
		//Update with corrected mode string
		twi->setData(Qt::DisplayRole,Demod::ModeToString(m));
		twi->setFlags(Qt::ItemIsEnabled);
		twi->setBackground(editBrush);
		twi = ui.tableWidget->item(editRow,NOTES);
		twi->setFlags(Qt::ItemIsEnabled);
		twi->setBackground(editBrush);

		editMode = false;
		//Restore sorting
		ui.tableWidget->setSortingEnabled(true);

	} else {
		//Make sure sorting is off while editing so rows don't changed
		ui.tableWidget->setSortingEnabled(false);

		QBrush brush;
		brush.setColor(Qt::white);
		brush.setStyle(Qt::SolidPattern);
		editRow = ui.tableWidget->currentRow();
		//Enable editing in each cell and change appearance so its obvious
		QTableWidgetItem *twi = ui.tableWidget->item(editRow,NAME);
		twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		//Save Brush so we can reset it
		editBrush = twi->background();
		twi->setBackground(brush);
		twi = ui.tableWidget->item(editRow,FREQ);
		twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		twi->setBackground(brush);
		twi = ui.tableWidget->item(editRow,MODE);
		twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		twi->setBackground(brush);
		twi = ui.tableWidget->item(editRow,NOTES);
		twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		twi->setBackground(brush);
		editMode = true;
	}
}
//Delete current row
void Presets::deleteClicked(bool b)
{
	int r = ui.tableWidget->currentRow();
	int index = ui.tableWidget->item(r,REC)->data(Qt::DisplayRole).toInt();
	//Flag as deleted, will be ignored when we write
	presets[index].name="";
	presets[index].low=0;
	ui.tableWidget->removeRow(r);
}
//Critical: Caller is responsible for turning sorting off before calling
//Index = array element
void Presets::NewRow(int row, int index, Preset p)
{
	

	QTableWidgetItem *twi;
	//C:\Qt\4.6.3\tools\designer\src\components\formeditor\images
	//Note pixmap() to scale down, original is 24x24
	twi = new QTableWidgetItem(QIcon("resources/edit.png").pixmap(8,8),"");
	twi->setToolTip("Edit or Delete this preset");
	ui.tableWidget->setItem(row,EDIT,twi);

	//Associate each row with index into presets array
	twi = new QTableWidgetItem(QString::number(index));
	ui.tableWidget->setItem(row,REC,twi);
	twi = new QTableWidgetItem(p.name);
	twi->setToolTip(p.name);
	//Flags are || together as needed
	//Qt::NoItemFlags to disable cell
	//Qt::ItemIsEnabled but not selectable or editable
	//Qt::ItemIsEditable
	twi->setFlags(Qt::ItemIsEnabled);
	ui.tableWidget->setItem(row,NAME,twi);

	twi = new QTableWidgetItem(QString::number(p.low,'f',0));
	twi->setFlags(Qt::ItemIsEnabled);

	ui.tableWidget->setItem(row,FREQ,twi);
	twi = new QTableWidgetItem(Demod::ModeToString(p.mode));

	twi->setFlags(Qt::ItemIsEnabled);
	ui.tableWidget->setItem(row,MODE,twi);

	twi = new QTableWidgetItem(p.notes);
	twi->setFlags(Qt::ItemIsEnabled);
	twi->setToolTip(p.notes);
	ui.tableWidget->setItem(row,NOTES,twi);

	ui.tableWidget->setRowHeight(row,17);
}
void Presets::saveClicked(bool b)
{
	Save();
}

void Presets::Read()
{
	QFile file(presetsFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream in(&file);
	QString line;
	QStringList parts;
	//How many lines in file
	numPresets = 0;
	int numLines = 0;
	while (!in.atEnd())
	{
		line = in.readLine();
		numLines++;
	}
	//1st line is header, don't count
	numLines--;
	//Reset stream
	in.seek(0);
	//Allocate buffer and extra space
	presetsLen = numLines + GROW_PRESETS;
	presets = new Preset[presetsLen];
	double low;
	//Read file into memory, throw away header line
	in.readLine();
	for (int i=0; i<numLines; i++)
	{
		line = in.readLine();
		parts = line.split(",");
		if (parts.count() < 2) 
			continue; //Skip invalid line, we need at least name and low freq
		//Low freq must have a valid value
		low = parts[1].toDouble();
		if (low == 0)
			continue;
		presets[i].name = parts[0];
		presets[i].low = low;
		numPresets++;
		if (parts.count() >= 3)
			presets[i].high = parts[2].toDouble();
		if (parts.count() >= 4)
			presets[i].mode = Demod::StringToMode(parts[3]);
		if (parts.count() >= 5)
			presets[i].notes = parts[4];
		
	}
	//TableWidget update moved to Show()
}
//Note, any invalid presets that were read are lost when we save
//Presets array is always valid
void Presets::Save()
{

	QFile file(presetsFile);
     if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
         return;

     QTextStream out(&file);
	 //QTableWidgetItem *cell;
	 QString str;
	 //Write header row for reference
	 out <<"Name,Low,High,Mode,Notes\n";
	 for (int i=0; i<numPresets; i++)
	 {
		 //Todo: Handle escape char is comma in string
		 //cell = ui.tableWidget->item(i,COL::FREQ);
		 //str = (cell == NULL) ? "" : cell->data(Qt::DisplayRole).toString(); 
		 //cell = ui.tableWidget->item(i,COL::MODE);
		 //str = (cell == NULL) ? "" : cell->data(Qt::DisplayRole).toString();
		 //cell = ui.tableWidget->item(i,COL::NOTES);
		 //str = (cell == NULL) ? "" : cell->data(Qt::DisplayRole).toString(); 

		 //Skip deleted entries, low==0
		 if (presets[i].low != 0) {
			 out << presets[i].name << ",";
			 out << QString::number(presets[i].low,'f',0) << ",";
			 out << QString::number(presets[i].high,'f',0) << ",";
			 out << Demod::ModeToString(presets[i].mode) << ",";
			 out << presets[i].notes;
			 out << "\n";
		 }
		 
	 }
	 file.close();
}
Preset::Preset()
{
	name="";
	notes="";
	low=0.0;
	high=0.0;
	mode = dmAM;
    //schedule = QDateTime();
}
Preset::~Preset()
{
}
