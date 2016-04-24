#ifndef SDROPTIONS_H
#define SDROPTIONS_H
#include <QDialog>
#include "device_interfaces.h"
#include "ui_sdr.h"
#include "audio.h"

class SdrOptions:public QObject
{
	Q_OBJECT
public:
	SdrOptions();
	~SdrOptions();
	void showSdrOptions(DeviceInterface *_di, bool b);

public slots:
	void deviceSelectionChanged(int i);
private slots:
	void inputChanged(int i);
	void outputChanged(int i);
	void startupChanged(int i);
	void startupFrequencyChanged();
	void sampleRateChanged(int i);
	void iqGainChanged(double i);
	void iqOrderChanged(int i);
	void balancePhaseChanged(int v);
	void balanceGainChanged(int v);
	void balanceEnabledChanged(bool b);
	void balanceReset();
	void resetSettings(bool b);
	void closeOptions(bool b);
	void converterModeChanged(bool b);
	void converterOffsetChanged();
	void dcRemovalChanged(bool b);

signals:
	void restart();

private:
	DeviceInterface *m_di;

	QDialog *m_sdrOptionsDialog;
	Ui::SdrOptions *m_sd;


	QStringList m_inputDevices;
	QStringList m_outputDevices;

	void updateOptions();

	bool m_dcRemove;
};

#endif // SDROPTIONS_H
