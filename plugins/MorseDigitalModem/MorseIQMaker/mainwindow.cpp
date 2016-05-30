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
	outBuf = CPX::memalign(32000);
	morseGen = new MorseGen(48000, 1000, -50, 20);
	quint32 len = 0;
	for (int i=0; i<1024; i++) {
		len = morseGen->genDot(outBuf);
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genElement(outBuf);
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genDash(outBuf);
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genChar(outBuf);
		wavOutFile.WriteSamples(outBuf,len);
		len = morseGen->genWord(outBuf);
		wavOutFile.WriteSamples(outBuf,len);
	}
}

MainWindow::~MainWindow()
{
	delete ui;
	wavOutFile.Close();
}
