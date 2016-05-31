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

	//dmCWL=8 from device_interfaces.h
	QString outFileName = pebbleRecordingPath + "testmorseiq.wav";
	bool result = wavOutFile.OpenWrite(outFileName,48000,24000,8,0);
	morseGen = new MorseGen(48000, 1000, -50, 20);
	quint32 tcwMs = MorseCode::wpmToTcwMs(20);
	quint32 maxSymbolMs = MorseCode::c_maxTcwPerSymbol * tcwMs;
	quint32 outBufLen = maxSymbolMs / (1000.0/48000.0);
	outBuf = CPX::memalign(outBufLen);
	quint32 len = 0;

#if 1
	for (int i=0; i<256; i++) {
		len = morseGen->genTable(outBuf,i);
		wavOutFile.WriteSamples(outBuf,len);
	}
#else
	for (int i=0; i<10; i++) {
		len = morseGen->genText(outBuf,'a');
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genText(outBuf,'b');
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genText(outBuf,'c');
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genText(outBuf,' ');
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genText(outBuf,'1');
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genWord(outBuf);
		wavOutFile.WriteSamples(outBuf,len);
	}
#endif

}

MainWindow::~MainWindow()
{
	delete ui;
	wavOutFile.Close();
}
