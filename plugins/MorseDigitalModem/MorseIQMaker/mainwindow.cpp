#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDir"
MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

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

	QString pebbleRecordingPath = appDirPath + "/PebbleRecordings/";

	quint32 sampleRate = 48000;
	//dmCWL=8 from device_interfaces.h
	QString outFileName = pebbleRecordingPath + "testmorseiq.wav";
	bool result = wavOutFile.OpenWrite(outFileName,sampleRate,sampleRate/2,8,0);
	quint32 tcwMs = MorseCode::wpmToTcwMs(20);
	quint32 maxSymbolMs = MorseCode::c_maxTcwPerSymbol * tcwMs;
	quint32 outBufLen = maxSymbolMs / (1000.0/sampleRate);
	outBuf = CPX::memalign(outBufLen);
	outBuf1 = CPX::memalign(outBufLen);
	outBuf2 = CPX::memalign(outBufLen);
	quint32 len = 0;

	//Sample rate must be same for all gen and same as wav file
	//msRise for faster speeds more critical, testing at 5ms
	morseGen1 = new MorseGen(sampleRate, 1000, -40, 20, 5);
	//Testing higher speeds, 50wpm ok, 60wpm no
	morseGen2 = new MorseGen(sampleRate, 2000, -40, 55, 5);

	const char* sample1 = "The quick brown fox jumped over the lazy dog 1,2,3,4,5,6,7,8,9,0 times";
	const char* sample2 = "Now is the time for all good men to come to the aid of their country";

#if 1
	morseGen1->setTextOut(sample1);
	morseGen2->setTextOut(sample2);
	CPX gen1,gen2,out;
	while (morseGen1->hasOutputSamples()) {
		gen1 = morseGen1->nextOutputSample();
		gen2 = morseGen2->nextOutputSample();
		out = gen1 + gen2;
		wavOutFile.WriteSamples(&out, 1);
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
#if 0
	for (int i=0; i<256; i++) {
		len = morseGen1->genTable(outBuf,i);
		wavOutFile.WriteSamples(outBuf,len);
	}
#endif

}

MainWindow::~MainWindow()
{
	delete ui;
	wavOutFile.Close();
}
