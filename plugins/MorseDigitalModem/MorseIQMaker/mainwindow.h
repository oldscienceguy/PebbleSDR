#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "wavfile.h"
#include "morsegen.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow *ui;

	WavFile wavOutFile;
	CPX *outBuf;
	CPX *outBuf1;
	CPX *outBuf2;

	MorseGen *morseGen1;
	MorseGen *morseGen2;
};

#endif // MAINWINDOW_H
