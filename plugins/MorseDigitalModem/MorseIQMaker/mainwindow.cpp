#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDir"
#include "db.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setupUi();

	m_sampleRate = 48000;
	quint32 tcwMs = MorseCode::wpmToTcwMs(20);
	quint32 maxSymbolMs = MorseCode::c_maxTcwPerSymbol * tcwMs;
	quint32 outBufLen = maxSymbolMs / (1000.0/m_sampleRate);
	m_outBuf = CPX::memalign(outBufLen);
	m_outBuf1 = CPX::memalign(outBufLen);
	m_outBuf2 = CPX::memalign(outBufLen);

	//Sample rate must be same for all gen and same as wav file
	m_morseGen1 = new MorseGen(m_sampleRate);
	m_morseGen2 = new MorseGen(m_sampleRate);
	m_morseGen3 = new MorseGen(m_sampleRate);
	m_morseGen4 = new MorseGen(m_sampleRate);
	m_morseGen5 = new MorseGen(m_sampleRate);


	//Trailing '=' is morse <BT> for new paragraph
	m_sampleText[0] = "=The quick brown fox jumped over the lazy dog 1,2,3,4,5,6,7,8,9,0 times=";
	m_sampleText[1] = "=Now is the time for all good men to come to the aid of their country=";
	m_sampleText[2] = "=Now is the time for all good men to come to the aid of their country=";
	m_sampleText[3] = "=Now is the time for all good men to come to the aid of their country=";
	//m_sampleText[4] = "=Now is the time for all good men to come to the aid of their country=";

	//Build text for all characters
	MorseSymbol *symbol;
	m_sampleText[4] = "=";
	for (quint32 i=0; i<MorseCode::c_morseTableSize; i++) {
		symbol = &MorseCode::m_morseTable[i];
		if (symbol->ascii == 0x00) {
			break; //End of table
		}
		m_sampleText[4].append(symbol->ascii);
	}
	m_sampleText[4].append("=");
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::setupUi()
{
	ui->timeBox->addItem("1 min", 60);
	ui->timeBox->addItem("2 min", 120);
	ui->timeBox->addItem("5 min", 300);
	ui->timeBox->addItem("10 min",600);

	ui->noiseBox->addItem("-40db", -40);
	ui->noiseBox->addItem("-45db", -45);
	ui->noiseBox->addItem("-50db", -50);
	ui->noiseBox->addItem("-55db", -55);
	ui->noiseBox->addItem("-60db", -60);
	ui->noiseBox->addItem("-60db", -65);
	ui->noiseBox->addItem("-60db", -70);
	ui->noiseBox->addItem("-60db", -75);
	ui->noiseBox->addItem("-60db", -80);
	ui->noiseBox->setCurrentText("-60db");

	ui->sourceBox_1->addItem("Sample1",0);
	ui->sourceBox_1->addItem("Sample2",1);
	ui->sourceBox_1->addItem("Sample3",2);
	ui->sourceBox_1->addItem("Sample4",3);
	ui->sourceBox_1->addItem("Table",4);

	ui->wpmBox_1->addItem("5 wpm", 5);
	ui->wpmBox_1->addItem("10 wpm", 10);
	ui->wpmBox_1->addItem("15 wpm", 15);
	ui->wpmBox_1->addItem("20 wpm", 20);
	ui->wpmBox_1->addItem("30 wpm", 30);
	ui->wpmBox_1->addItem("40 wpm", 40);
	ui->wpmBox_1->addItem("50 wpm", 50);
	ui->wpmBox_1->addItem("60 wpm", 60);
	ui->wpmBox_1->addItem("70 wpm", 70);
	ui->wpmBox_1->addItem("80 wpm", 80);

	ui->dbBox_1->addItem("-30db", -30);
	ui->dbBox_1->addItem("-35db", -35);
	ui->dbBox_1->addItem("-40db", -40);
	ui->dbBox_1->addItem("-45db", -45);
	ui->dbBox_1->addItem("-50db", -50);
	ui->dbBox_1->addItem("-55db", -55);
	ui->dbBox_1->addItem("-60db", -60);

	//2
	ui->sourceBox_2->addItem("Sample1",0);
	ui->sourceBox_2->addItem("Sample2",1);
	ui->sourceBox_2->addItem("Sample3",2);
	ui->sourceBox_2->addItem("Sample4",3);
	ui->sourceBox_2->addItem("Table",4);

	ui->wpmBox_2->addItem("5 wpm", 5);
	ui->wpmBox_2->addItem("10 wpm", 10);
	ui->wpmBox_2->addItem("15 wpm", 15);
	ui->wpmBox_2->addItem("20 wpm", 20);
	ui->wpmBox_2->addItem("30 wpm", 30);
	ui->wpmBox_2->addItem("40 wpm", 40);
	ui->wpmBox_2->addItem("50 wpm", 50);
	ui->wpmBox_2->addItem("60 wpm", 60);
	ui->wpmBox_2->addItem("70 wpm", 70);
	ui->wpmBox_2->addItem("80 wpm", 80);

	ui->dbBox_2->addItem("-30db", -30);
	ui->dbBox_2->addItem("-35db", -35);
	ui->dbBox_2->addItem("-40db", -40);
	ui->dbBox_2->addItem("-45db", -45);
	ui->dbBox_2->addItem("-50db", -50);
	ui->dbBox_2->addItem("-55db", -55);
	ui->dbBox_2->addItem("-60db", -60);

	//3
	ui->sourceBox_3->addItem("Sample1",0);
	ui->sourceBox_3->addItem("Sample2",1);
	ui->sourceBox_3->addItem("Sample3",2);
	ui->sourceBox_3->addItem("Sample4",3);
	ui->sourceBox_3->addItem("Table",4);

	ui->wpmBox_3->addItem("5 wpm", 5);
	ui->wpmBox_3->addItem("10 wpm", 10);
	ui->wpmBox_3->addItem("15 wpm", 15);
	ui->wpmBox_3->addItem("20 wpm", 20);
	ui->wpmBox_3->addItem("30 wpm", 30);
	ui->wpmBox_3->addItem("40 wpm", 40);
	ui->wpmBox_3->addItem("50 wpm", 50);
	ui->wpmBox_3->addItem("60 wpm", 60);
	ui->wpmBox_3->addItem("70 wpm", 70);
	ui->wpmBox_3->addItem("80 wpm", 80);

	ui->dbBox_3->addItem("-30db", -30);
	ui->dbBox_3->addItem("-35db", -35);
	ui->dbBox_3->addItem("-40db", -40);
	ui->dbBox_3->addItem("-45db", -45);
	ui->dbBox_3->addItem("-50db", -50);
	ui->dbBox_3->addItem("-55db", -55);
	ui->dbBox_3->addItem("-60db", -60);

	//4
	ui->sourceBox_4->addItem("Sample1",0);
	ui->sourceBox_4->addItem("Sample2",1);
	ui->sourceBox_4->addItem("Sample3",2);
	ui->sourceBox_4->addItem("Sample4",3);
	ui->sourceBox_4->addItem("Table",4);

	ui->wpmBox_4->addItem("5 wpm", 5);
	ui->wpmBox_4->addItem("10 wpm", 10);
	ui->wpmBox_4->addItem("15 wpm", 15);
	ui->wpmBox_4->addItem("20 wpm", 20);
	ui->wpmBox_4->addItem("30 wpm", 30);
	ui->wpmBox_4->addItem("40 wpm", 40);
	ui->wpmBox_4->addItem("50 wpm", 50);
	ui->wpmBox_4->addItem("60 wpm", 60);
	ui->wpmBox_4->addItem("70 wpm", 70);
	ui->wpmBox_4->addItem("80 wpm", 80);

	ui->dbBox_4->addItem("-30db", -30);
	ui->dbBox_4->addItem("-35db", -35);
	ui->dbBox_4->addItem("-40db", -40);
	ui->dbBox_4->addItem("-45db", -45);
	ui->dbBox_4->addItem("-50db", -50);
	ui->dbBox_4->addItem("-55db", -55);
	ui->dbBox_4->addItem("-60db", -60);

	//5
	ui->sourceBox_5->addItem("Sample1",0);
	ui->sourceBox_5->addItem("Sample2",1);
	ui->sourceBox_5->addItem("Sample3",2);
	ui->sourceBox_5->addItem("Sample4",3);
	ui->sourceBox_5->addItem("Table",4);

	ui->wpmBox_5->addItem("5 wpm", 5);
	ui->wpmBox_5->addItem("10 wpm", 10);
	ui->wpmBox_5->addItem("15 wpm", 15);
	ui->wpmBox_5->addItem("20 wpm", 20);
	ui->wpmBox_5->addItem("30 wpm", 30);
	ui->wpmBox_5->addItem("40 wpm", 40);
	ui->wpmBox_5->addItem("50 wpm", 50);
	ui->wpmBox_5->addItem("60 wpm", 60);
	ui->wpmBox_5->addItem("70 wpm", 70);
	ui->wpmBox_5->addItem("80 wpm", 80);

	ui->dbBox_5->addItem("-30db", -30);
	ui->dbBox_5->addItem("-35db", -35);
	ui->dbBox_5->addItem("-40db", -40);
	ui->dbBox_5->addItem("-45db", -45);
	ui->dbBox_5->addItem("-50db", -50);
	ui->dbBox_5->addItem("-55db", -55);
	ui->dbBox_5->addItem("-60db", -60);

	connect(ui->generateButton,SIGNAL(clicked(bool)),this,SLOT(generateButtonClicked(bool)));
	connect(ui->resetButton,SIGNAL(clicked(bool)),this,SLOT(resetButtonClicked(bool)));

	resetButtonClicked(true);
}

void MainWindow::resetButtonClicked(bool clicked)
{
	Q_UNUSED(clicked);

	ui->timeBox->setCurrentIndex(0);

	ui->enabledBox_1->setChecked(true);
	ui->freqencyEdit_1->setText("1000");
	ui->sourceBox_1->setCurrentIndex(0);
	ui->wpmBox_1->setCurrentIndex(3);
	ui->dbBox_1->setCurrentIndex(2);

	ui->enabledBox_2->setChecked(true);
	ui->freqencyEdit_2->setText("1500");
	ui->sourceBox_2->setCurrentIndex(1);
	ui->wpmBox_2->setCurrentIndex(4);
	ui->dbBox_2->setCurrentIndex(2);

	ui->enabledBox_3->setChecked(true);
	ui->freqencyEdit_3->setText("2000");
	ui->sourceBox_3->setCurrentIndex(2);
	ui->wpmBox_3->setCurrentIndex(5);
	ui->dbBox_3->setCurrentIndex(2);

	ui->enabledBox_4->setChecked(true);
	ui->freqencyEdit_4->setText("2500");
	ui->sourceBox_4->setCurrentIndex(3);
	ui->wpmBox_4->setCurrentIndex(6);
	ui->dbBox_4->setCurrentIndex(2);

	ui->enabledBox_5->setChecked(true);
	ui->freqencyEdit_5->setText("3000");
	ui->sourceBox_5->setCurrentIndex(4);
	ui->wpmBox_5->setCurrentIndex(7);
	ui->dbBox_5->setCurrentIndex(2);
}

void MainWindow::generateButtonClicked(bool clicked)
{
	Q_UNUSED(clicked);
	QString appDirPath = QCoreApplication::applicationDirPath();
	QDir appDir = QDir(appDirPath);
#if defined(Q_OS_MAC)
	if (appDir.dirName() == "MacOS") {
		//We're in the mac package and need to get back to the package to access relative directories
		appDir.cdUp();
		appDir.cdUp();
		appDir.cdUp(); //Root dir where app is located
		appDirPath = appDir.absolutePath();
	}
#endif

	//Gen a unique name for output file
	QString baseName = "MorseIQ_";
	QString recordingPath = appDirPath + "/PebbleRecordings/";
	QFileInfo fInfo;
	for (int i=1; i<1000; i++) {
		m_outFileName = recordingPath + baseName + QString::number(i) + ".wav";
		fInfo.setFile(m_outFileName);
		if (!fInfo.exists())
			break; //Got a unique name
		//When we overrun counter, last filename will be continually overwritten
	}

	//dmCWL=8 from device_interfaces.h
	bool result = m_wavOutFile.OpenWrite(m_outFileName,m_sampleRate,m_sampleRate/2,8,0);
	if (!result) {
		qDebug()<<"Error opening file "<<m_outFileName;
		return;
	}

	double gen1Freq = ui->freqencyEdit_1->text().toDouble();
	double gen1Amp = ui->dbBox_1->currentData().toDouble();
	quint32 gen1Wpm = ui->wpmBox_1->currentData().toUInt();
	quint32 gen1Rise = 5; //Add to UI?
	quint32 gen1Text = ui->sourceBox_1->currentData().toUInt();
	bool gen1Enabled = ui->enabledBox_1->isChecked();
	if (gen1Enabled) {
		m_morseGen1->setParams(gen1Freq,gen1Amp,gen1Wpm,gen1Rise);
		m_morseGen1->setTextOut(m_sampleText[gen1Text]);
	}

	double gen2Freq = ui->freqencyEdit_2->text().toDouble();
	double gen2Amp = ui->dbBox_2->currentData().toDouble();
	quint32 gen2Wpm = ui->wpmBox_2->currentData().toUInt();
	quint32 gen2Rise = 5; //Add to UI?
	quint32 gen2Text = ui->sourceBox_2->currentData().toUInt();
	bool gen2Enabled = ui->enabledBox_2->isChecked();
	if (gen2Enabled) {
		m_morseGen2->setParams(gen2Freq,gen2Amp,gen2Wpm,gen2Rise);
		m_morseGen2->setTextOut(m_sampleText[gen2Text]);
	}

	double gen3Freq = ui->freqencyEdit_3->text().toDouble();
	double gen3Amp = ui->dbBox_3->currentData().toDouble();
	quint32 gen3Wpm = ui->wpmBox_3->currentData().toUInt();
	quint32 gen3Rise = 5; //Add to UI?
	quint32 gen3Text = ui->sourceBox_3->currentData().toUInt();
	bool gen3Enabled = ui->enabledBox_3->isChecked();
	if (gen3Enabled) {
		m_morseGen3->setParams(gen3Freq,gen3Amp,gen3Wpm,gen3Rise);
		m_morseGen3->setTextOut(m_sampleText[gen3Text]);
	}

	double gen4Freq = ui->freqencyEdit_4->text().toDouble();
	double gen4Amp = ui->dbBox_4->currentData().toDouble();
	quint32 gen4Wpm = ui->wpmBox_4->currentData().toUInt();
	quint32 gen4Rise = 5; //Add to UI?
	quint32 gen4Text = ui->sourceBox_4->currentData().toUInt();
	bool gen4Enabled = ui->enabledBox_4->isChecked();
	if (gen4Enabled) {
		m_morseGen4->setParams(gen4Freq,gen4Amp,gen4Wpm,gen4Rise);
		m_morseGen4->setTextOut(m_sampleText[gen4Text]);
	}

	double gen5Freq = ui->freqencyEdit_5->text().toDouble();
	double gen5Amp = ui->dbBox_5->currentData().toDouble();
	quint32 gen5Wpm = ui->wpmBox_5->currentData().toUInt();
	quint32 gen5Rise = 5; //Add to UI?
	quint32 gen5Text = ui->sourceBox_5->currentData().toUInt();
	bool gen5Enabled = ui->enabledBox_5->isChecked();
	if (gen5Enabled) {
		m_morseGen5->setParams(gen5Freq,gen5Amp,gen5Wpm,gen5Rise);
		m_morseGen5->setTextOut(m_sampleText[gen5Text]);
	}

	//How many samples do we need for time setting
	quint32 secTime = ui->timeBox->currentData().toUInt();
	quint32 numSamples = m_sampleRate * secTime;

	double dbNoiseAmp = ui->noiseBox->currentData().toDouble();
	//Because noise is averaged, fudget +30db so it matches with generator values
	//ie -30db gen and -30db noise should be 0snr
	double noiseAmp = DB::dBToAmplitude(dbNoiseAmp+20);

	CPX cpx1,cpx2,cpx3,cpx4,cpx5,cpx6, out;
	for (quint32 i=0; i<numSamples; i++) {
		if (gen1Enabled)
			cpx1 = m_morseGen1->nextOutputSample();
		if (gen2Enabled)
			cpx2 = m_morseGen2->nextOutputSample();
		if (gen3Enabled)
			cpx3 = m_morseGen3->nextOutputSample();
		if (gen4Enabled)
			cpx4 = m_morseGen4->nextOutputSample();
		if (gen5Enabled)
			cpx5 = m_morseGen5->nextOutputSample();
		cpx6 = nextNoiseSample(noiseAmp);
		out = cpx1 + cpx2 + cpx3 + cpx4 + cpx5 + cpx6;
		m_wavOutFile.WriteSamples(&out, 1);
	}
	m_wavOutFile.Close();

#if 0
	m_morseGen2->setTextOut(sample2);
	CPX gen1,gen2,out;
	while (m_morseGen1->hasOutputSamples()) {
		gen1 = m_morseGen1->nextOutputSample();
		gen2 = m_morseGen2->nextOutputSample();
		out = gen1 + gen2;
		m_wavOutFile.WriteSamples(&out, 1);
	}
#endif
#if 0
	for (int i=0; i<strlen(sample1); i++) {
		len = morseGen1->genText(outBuf1, sample1[i]);
		len = morseGen2->genText(outBuf2, sample1[i]);
		//lengths will be different if wpm not the same, how to handle???
		for (int j=0; j<len; j++) {
			outBuf[j] = outBuf1[j] + outBuf2[j];
		}
		wavOutFile.WriteSamples(outBuf,len);
	}
#endif

}

//Derived from nco.cpp
//Algorithm from http://dspguru.com/dsp/howtos/how-to-generate-white-gaussian-noise
//Uses Box-Muller Method, optimized to Polar Method [Ros88] A First Course on Probability
//Also http://www.dspguide.com/ch2/6.htm
//https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform R Knop algorithm
CPX MainWindow::nextNoiseSample(double _dbGain)
{
	double u1;
	double u2;
	double s;
	double rad;
	double x;
	double y;

	//R Knop
	do {
		u1 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX;
		u2 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX;
		//Collapsing steps
		s = u1 * u1 + u2 * u2;
	} while (s >= 1.0 || s == 0.0);
	// 0 < s < 1
	rad = sqrt(-2.0 * log(s) / s);
	x = _dbGain * u1 * rad;
	y = _dbGain * u2 * rad;
	return CPX(x,y);
}
