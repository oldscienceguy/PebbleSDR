#include "deviceinterfacebase.h"
#include <QWidget>
#include "pebblelib_global.h"
#include "db.h"
PebbleLibGlobal *pebbleLibGlobal;

DeviceInterfaceBase::DeviceInterfaceBase()
{
	pebbleLibGlobal = new PebbleLibGlobal();

	m_connected = false;
	//These are common settings for every device, variables are defined in DeviceInterface
	m_startupType = ST_DEFAULTFREQ;
	m_userFrequency = 10000000;
	m_inputDeviceName = QString();
	m_outputDeviceName = QString();
	m_sampleRate = 48000;
	m_deviceSampleRate = m_sampleRate;
	m_userIQGain = 1.0;
	m_iqOrder = IQO_IQ;
	m_iqBalanceGain = 1.0;
	m_iqBalancePhase = 0;
	m_iqBalanceEnable = false;
	m_lastFreq = 10000000;
	m_lastDemodMode = dmAM;
	m_lastSpectrumMode = 0;
	m_deviceNumber = 0;
	m_startupFrequency = 10000000;
	m_lowFrequency = 150000;
	m_highFrequency = 30000000;
	m_startupDemodMode = dmAM;
	m_connected = false;
	m_running = false;
	m_audioInput = NULL;
	processIQData = NULL;
	processBandscopeData = NULL;
	processAudioData = NULL;
	m_audioOutputSampleRate = 11025;
	m_audioInputBuffer = NULL;
	//Set normalizeIQ gain by injecting known signal db into device and matching spectrum display
	m_normalizeIQGain = 1.0; //Will be overridden by specific devices if needed
	m_converterMode = false;
	m_converterOffset = 0;
	m_decimateFactor = 1;
}

//Implement pure virtual destructor from interface, otherwise we don't link
DeviceInterface::~DeviceInterface()
{

}

DeviceInterfaceBase::~DeviceInterfaceBase()
{
	if (m_audioInputBuffer != NULL) {
		delete m_audioInputBuffer;
		m_audioInputBuffer = NULL;
	}
}

bool DeviceInterfaceBase::initialize(CB_ProcessIQData _callback,
									 CB_ProcessBandscopeData _callbackBandscope,
									 CB_ProcessAudioData _callbackAudio,
									 quint16 _framesPerBuffer)
{
	processIQData = _callback;
	processBandscopeData = _callbackBandscope;
	processAudioData = _callbackAudio;
	m_framesPerBuffer = _framesPerBuffer;
	m_connected = false;

	using namespace std::placeholders;
	if (get(DeviceInterface::Key_DeviceType).toInt() == DT_AUDIO_IQ_DEVICE) {
		m_audioInput = Audio::Factory(std::bind(&DeviceInterfaceBase::audioProducer, this, _1, _2), m_framesPerBuffer);
	}

	m_audioInputBuffer = CPX::memalign(m_framesPerBuffer);

	return true;
}

bool DeviceInterfaceBase::connectDevice()
{
	return true;
}

bool DeviceInterfaceBase::disconnectDevice()
{
	return true;
}

void DeviceInterfaceBase::startDevice()
{
	if (get(DeviceInterface::Key_DeviceType).toInt() == DT_AUDIO_IQ_DEVICE) {
		//We handle audio
		m_audioInput->StartInput(m_inputDeviceName, m_sampleRate);
	}
	return;
}

void DeviceInterfaceBase::stopDevice()
{
	if (get(DeviceInterface::Key_DeviceType).toInt() == DT_AUDIO_IQ_DEVICE) {
		if (m_audioInput != NULL)
			m_audioInput->Stop();
	}

	return;
}

void DeviceInterfaceBase::setupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
	return;
}

bool DeviceInterfaceBase::command(DeviceInterface::StandardCommands _cmd, QVariant _arg)
{
	switch (_cmd) {
		case Cmd_Connect:
			//Transition
			return this->connectDevice();
		case Cmd_Disconnect:
			return this->disconnectDevice();
		case Cmd_Start:
			this->startDevice();
			return true;
		case Cmd_Stop:
			this->stopDevice();
			return true;
		case Cmd_ReadSettings:
			this->readSettings();
			return true;
		case Cmd_WriteSettings:
			this->writeSettings();
			return true;
		case Cmd_DisplayOptionUi:						//Arg is QWidget *parent
			//Use QVariant::fromValue() to pass, and value<type passed>() to get back
			this->setupOptionUi(_arg.value<QWidget*>());
			return true;
		default:
			return false;
	}

}


QVariant DeviceInterfaceBase::get(StandardKeys _key, QVariant _option) {
	Q_UNUSED(_option);
	switch (_key) {
		case Key_PluginName:
			break;
		case Key_PluginDescription:
			break;
		case Key_PluginNumDevices:
			return 1;
			break;
		case Key_DeviceName:
			break;
		case Key_DeviceDescription:
			break;
		case Key_DeviceNumber:
			return m_deviceNumber;
			break;
		case Key_DeviceType:
			return DT_AUDIO_IQ_DEVICE;
			break;
		//Applications sets deviceSampleRate, which is the hardware sample rate
		//Device returns sampleRate, which may be decimated by the device
		//SampleRate is read only and defaults to deviceSampleRate
		case Key_SampleRate:
			return m_deviceSampleRate;
			break;
		case Key_DeviceSampleRate:
			return m_deviceSampleRate;
			break;
		case Key_DeviceSampleRates:
			//We shouldn't know this, depends on audio device connected to receiver
			return QStringList()<<"48000"<<"96000"<<"192000";
			break;
		case Key_DeviceFrequency:
			return m_deviceFrequency;
			break;
		case Key_DeviceHealthValue:
			return 100; //Default is perfect health
		case Key_DeviceHealthString:
			return "Device running normally";
		case Key_InputDeviceName:
			return m_inputDeviceName;
			break;
		case Key_OutputDeviceName:
			return m_outputDeviceName;
			break;
		case Key_HighFrequency:
			//Assume DC to daylight since we have no idea what converter is doing
			if (m_converterMode)
				return 6000000000.0;
			else
				return m_highFrequency;
		case Key_LowFrequency:
			//Assume DC to daylight since we have no idea what converter is doing
			if (m_converterMode)
				return 0;
			else
				return m_lowFrequency;
		case Key_DeviceFreqCorrectionPpm:
			return 0;
			break;
		case Key_IQGain:
			return m_userIQGain;
			break;
		case Key_StartupType:
			return m_startupType;
			break;
		case Key_StartupDemodMode:
			return m_startupDemodMode;
		case Key_StartupSpectrumMode:
			break;
		case Key_StartupFrequency:
			return m_startupFrequency;
		case Key_LastDemodMode:
			return m_lastDemodMode;
			break;
		case Key_LastSpectrumMode:
			return m_lastSpectrumMode;
			break;
		case Key_LastFrequency:
			//If freq is outside of mode we are in return default
			if (m_lastFreq > get(DeviceInterface::Key_HighFrequency).toDouble() || m_lastFreq < get(DeviceInterface::Key_LowFrequency).toDouble())
				return get(DeviceInterface::Key_StartupFrequency).toDouble();
			else
				return m_lastFreq;
			break;
		case Key_UserDemodMode:
			break;
		case Key_UserSpectrumMode:
			break;
		case Key_UserFrequency:
			return m_userFrequency;
			break;
		case Key_IQOrder:
			return m_iqOrder;
			break;
		case Key_IQBalanceEnabled:
			return m_iqBalanceEnable;
			break;
		case Key_IQBalanceGain:
			return m_iqBalanceGain;
			break;
		case Key_IQBalancePhase:
			return m_iqBalancePhase;
			break;
		case Key_AudioOutputSampleRate:
			return m_audioOutputSampleRate;
			break;
		case Key_DeviceDemodMode:		//RW quint16 enum DeviceInterface::DEMODMODE
			return 0;
			break;
		case Key_DeviceOutputGain:		//RW quint16
			return 0;
			break;
		case Key_DeviceFilter:			//RW QString "lowFilter:highFilter"
			return "-2000:2000";
			break;
		case Key_DeviceAGC:				//RW quint16
			return 0;
			break;
		case Key_DeviceANF:				//RW quint16
			return 0;
			break;
		case Key_DeviceNB:				//RW quint16
			return 0;
			break;
		case Key_DeviceSlave:			//RO bool true if device is controled by somthing other than Pebble
			return false;
		case Key_SettingsFile:
			if (m_settings != NULL)
				return m_settings->fileName();
			return false;
		case Key_ConverterMode:
			return m_converterMode;
		case Key_ConverterOffset:
			return m_converterOffset;
		case Key_Setting:
			return m_settings->value(_option.toString());
			break;
		case Key_DecimateFactor:
			return m_decimateFactor;
			break;
		case Key_RemoveDC:
			return m_removeDC;
			break;
		default:
			break;
	}
	return QVariant();
}

bool DeviceInterfaceBase::set(StandardKeys _key, QVariant _value, QVariant _option) {
	Q_UNUSED(_value);
	Q_UNUSED(_option);
	switch (_key) {
		case Key_PluginName:
			Q_UNREACHABLE(); //Read only key
			break;
		case Key_PluginDescription:
			Q_UNREACHABLE();
			break;
		case Key_PluginNumDevices:
			Q_UNREACHABLE();
			break;
		case Key_DeviceName:
			Q_UNREACHABLE();
			break;
		case Key_DeviceDescription:
			Q_UNREACHABLE();
			break;
		case Key_DeviceNumber:
			m_deviceNumber = _value.toInt();
			break;
		case Key_DeviceType:
			Q_UNREACHABLE();
			break;
		//SampleRate is read only and returned by device based on deviceSampleRate
		case Key_SampleRate:
			Q_UNREACHABLE();
			break;
		case Key_DeviceSampleRate:
			m_deviceSampleRate = _value.toUInt();
			break;
		case Key_DeviceSampleRates:
			Q_UNREACHABLE();
			break;
		case Key_DeviceFrequency:
			Q_UNREACHABLE(); //Must be handled by device
			break;
		case Key_InputDeviceName:
			m_inputDeviceName = _value.toString();
			break;
		case Key_OutputDeviceName:
			m_outputDeviceName = _value.toString();
			break;
		case Key_HighFrequency:
			Q_UNREACHABLE();
			break;
		case Key_LowFrequency:
			Q_UNREACHABLE();
			break;
		case Key_DeviceFreqCorrectionPpm:
			break;
		case Key_IQGain:
			m_userIQGain = _value.toDouble();
			break;
		case Key_StartupType:
			m_startupType = (StartupType)_value.toInt();
			break;
		case Key_StartupDemodMode:
			Q_UNREACHABLE();
			break;
		case Key_StartupSpectrumMode:
			Q_UNREACHABLE();
			break;
		case Key_StartupFrequency:
			Q_UNREACHABLE();
			break;
		case Key_LastDemodMode:
			m_lastDemodMode = _value.toInt();
			break;
		case Key_LastSpectrumMode:
			m_lastSpectrumMode = _value.toInt();
			break;
		case Key_LastFrequency:
			m_lastFreq = _value.toDouble();
			break;
		case Key_UserDemodMode:
			Q_UNREACHABLE(); //Future
			break;
		case Key_UserSpectrumMode:
			Q_UNREACHABLE(); //Future
			break;
		case Key_UserFrequency:
			m_userFrequency = _value.toDouble();
			break;
		case Key_IQOrder:
			m_iqOrder = (IQOrder)_value.toInt();
			break;
		case Key_IQBalanceEnabled:
			m_iqBalanceEnable = _value.toBool();
			break;
		case Key_IQBalanceGain:
			m_iqBalanceGain = _value.toDouble();
			break;
		case Key_IQBalancePhase:
			m_iqBalancePhase = _value.toDouble();
			break;
		case Key_AudioOutputSampleRate:
			Q_UNREACHABLE();
			break;
		case Key_DeviceDemodMode:		//RW quint16 enum DeviceInterface::DEMODMODE
			break;
		case Key_DeviceOutputGain:		//RW quint16
			break;
		case Key_DeviceFilter:			//RW QString "lowFilter:highFilter"
			break;
		case Key_DeviceAGC:				//RW quint16
			break;
		case Key_DeviceANF:				//RW quint16
			break;
		case Key_DeviceNB:				//RW quint16
			break;
		case Key_DeviceSlave:			//RO bool true if device is controled by somthing other than Pebble
			Q_UNREACHABLE();
			break;
		case Key_ConverterMode:
			m_converterMode = _value.toBool();
			break;
		case Key_ConverterOffset:
			m_converterOffset = _value.toDouble();
			break;
		case Key_Setting:
			if (m_settings == NULL)
				return false;
			m_settings->setValue(_value.toString(), _option);
			break;
		case Key_DecimateFactor:
			m_decimateFactor = _value.toUInt();
			break;
		case Key_RemoveDC:
			m_removeDC = _value.toBool();
			break;
		default:
			break;
	}
	return false;
}

//Settings shared by all devices
//Set defaults first, then call ReadSettings to handle initial ini file creation
void DeviceInterfaceBase::readSettings()
{
	//Set up defaults, specific devices can override by setting inputDeviceName and outputDeviceName before calling ReadSettings()
	if (m_outputDeviceName.isEmpty()) {
#ifdef USE_QT_AUDIO
		m_outputDeviceName = "Built-in Output";
#endif
#ifdef USE_PORT_AUDIO
		outputDeviceName = "Core Audio:Built-in Output";
#endif

	}
	if (m_inputDeviceName.isEmpty()) {
#ifdef USE_QT_AUDIO
		m_inputDeviceName = "Built-in Microphone";
#endif
#ifdef USE_PORT_AUDIO
		inputDeviceName = "CoreAudio:Built-in Microphone";
#endif

	}
	//These are common settings for every device, variables are defined in DeviceInterface
	m_startupType = (StartupType)m_settings->value("StartupType", m_startupType).toInt();
	m_userFrequency = m_settings->value("UserFrequency", m_userFrequency).toDouble();
	//Allow the device to specify a fixed inputDeviceName, see rfspacedevice for example
	m_inputDeviceName = m_settings->value("InputDeviceName", m_inputDeviceName).toString();
	m_outputDeviceName = m_settings->value("OutputDeviceName", m_outputDeviceName).toString();
	//sampleRate is returned by device and not saved
	//sampleRate = qSettings->value("SampleRate", sampleRate).toUInt();
	m_deviceSampleRate = m_settings->value("DeviceSampleRate", m_deviceSampleRate).toUInt();
	//Default sampleRate to deviceSampleRate for compatibility, device will override if necessary
	m_sampleRate = m_deviceSampleRate;
	m_userIQGain = m_settings->value("IQGain",m_userIQGain).toDouble();
	m_iqOrder = (IQOrder)m_settings->value("IQOrder", m_iqOrder).toInt();
	m_iqBalanceGain = m_settings->value("IQBalanceGain",m_iqBalanceGain).toDouble();
	m_iqBalancePhase = m_settings->value("IQBalancePhase",m_iqBalancePhase).toDouble();
	m_iqBalanceEnable = m_settings->value("IQBalanceEnable",m_iqBalanceEnable).toBool();
	m_lastFreq = m_settings->value("LastFreq", m_lastFreq).toDouble();
	m_lastDemodMode = m_settings->value("LastDemodMode",m_lastDemodMode).toInt();
	m_lastSpectrumMode = m_settings->value("LastSpectrumMode",m_lastSpectrumMode).toInt();
	m_deviceNumber = m_settings->value("DeviceNumber",m_deviceNumber).toInt();
	m_startupFrequency = m_settings->value("StartupFrequency",m_startupFrequency).toDouble();
	m_lowFrequency = m_settings->value("LowFrequency",m_lowFrequency).toDouble();
	m_highFrequency = m_settings->value("HighFrequency",m_highFrequency).toDouble();
	m_startupDemodMode = m_settings->value("StartupDemodMode",m_startupDemodMode).toInt();
	m_converterMode = m_settings->value("ConverterMode", false).toBool();
	m_converterOffset = m_settings->value("ConverterOffset", 0).toDouble();
	m_decimateFactor = m_settings->value("DecimateFactor",m_decimateFactor).toUInt();
	m_removeDC = m_settings->value("RemoveDC",false).toBool();
}

void DeviceInterfaceBase::writeSettings()
{
	m_settings->setValue("StartupType",m_startupType);
	m_settings->setValue("UserFrequency",m_userFrequency);
	m_settings->setValue("InputDeviceName", m_inputDeviceName);
	m_settings->setValue("OutputDeviceName", m_outputDeviceName);
	//sampleRate is returned by device and not saved
	//qSettings->setValue("SampleRate",sampleRate);
	m_settings->setValue("DeviceSampleRate",m_deviceSampleRate);
	m_settings->setValue("IQGain",m_userIQGain);
	m_settings->setValue("IQOrder", m_iqOrder);
	m_settings->setValue("IQBalanceGain", m_iqBalanceGain);
	m_settings->setValue("IQBalancePhase", m_iqBalancePhase);
	m_settings->setValue("IQBalanceEnable", m_iqBalanceEnable);
	m_settings->setValue("LastFreq",m_lastFreq);
	m_settings->setValue("LastDemodMode",m_lastDemodMode);
	m_settings->setValue("LastSpectrumMode",m_lastSpectrumMode);
	m_settings->setValue("DeviceNumber",m_deviceNumber);
	m_settings->setValue("StartupFrequency",m_startupFrequency);
	m_settings->setValue("LowFrequency",m_lowFrequency);
	m_settings->setValue("HighFrequency",m_highFrequency);
	m_settings->setValue("StartupDemodMode",m_startupDemodMode);
	m_settings->setValue("ConverterMode",m_converterMode);
	m_settings->setValue("ConverterOffset",m_converterOffset);
	m_settings->setValue("DecimateFactor",m_decimateFactor);
	m_settings->setValue("RemoveDC",m_removeDC);
}


//Should be a common function
void DeviceInterfaceBase::initSettings(QString fname)
{
	//Use ini files to avoid any registry problems or install/uninstall
	//Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
	//Scope::SystemScope puts file c:\ProgramData\n1ddy

	m_settings = new QSettings(pebbleLibGlobal->appDirPath + "/PebbleData/" + fname +".ini",QSettings::IniFormat);

}


//-1.0 to + 1.0
void DeviceInterfaceBase::normalizeIQ(CPX *cpx, float I, float Q)
{
	double tmp;
	//Normalize and apply gain
	cpx->re = I * m_userIQGain * m_normalizeIQGain;
	cpx->im = Q * m_userIQGain * m_normalizeIQGain;

	//Configure IQ order if not default
	switch(m_iqOrder) {
		case DeviceInterface::IQO_IQ:
			//No change, this is the default order
			break;
		case DeviceInterface::IQO_QI:
			tmp = cpx->re;
			cpx->re = cpx->im;
			cpx->im = tmp;
			break;
		case DeviceInterface::IQO_IONLY:
			cpx->im = cpx->re;
			break;
		case DeviceInterface::IQO_QONLY:
			cpx->re = cpx->im;
			break;
	}
}

// - 32767 to + 32767 samples
void DeviceInterfaceBase::normalizeIQ(CPX *cpx, qint16 I, qint16 Q)
{
	double tmp;
	//Normalize and apply gain
	cpx->re = (I / 32768.0) * m_userIQGain * m_normalizeIQGain;
	cpx->im = (Q / 32768.0) * m_userIQGain * m_normalizeIQGain;

	//Configure IQ order if not default
	switch(m_iqOrder) {
		case DeviceInterface::IQO_IQ:
			//No change, this is the default order
			break;
		case DeviceInterface::IQO_QI:
			tmp = cpx->re;
			cpx->re = cpx->im;
			cpx->im = tmp;
			break;
		case DeviceInterface::IQO_IONLY:
			cpx->im = cpx->re;
			break;
		case DeviceInterface::IQO_QONLY:
			cpx->re = cpx->im;
			break;
	}

}

// 0 to 255 samples in Offset Binary mode, ie RTL2832
// 255 = +127 (full scale - lsb)
// 128 = 0
// 0 = -128 (full scale)
void DeviceInterfaceBase::normalizeIQ(CPX *cpx, quint8 I, quint8 Q)
{
	double tmp;
	//Normalize and apply gain
	cpx->re = ((I - 128) / 128.0) * m_userIQGain * m_normalizeIQGain;
	cpx->im = ((Q - 128) / 128.0) * m_userIQGain * m_normalizeIQGain;

	//Configure IQ order if not default
	switch(m_iqOrder) {
		case DeviceInterface::IQO_IQ:
			//No change, this is the default order
			break;
		case DeviceInterface::IQO_QI:
			tmp = cpx->re;
			cpx->re = cpx->im;
			cpx->im = tmp;
			break;
		case DeviceInterface::IQO_IONLY:
			cpx->im = cpx->re;
			break;
		case DeviceInterface::IQO_QONLY:
			cpx->re = cpx->im;
			break;
	}
}

// -128 to +127 samples in 2's complement format
// 255 = -1
// 128 = -128 (full scale)
// 127 = 127 (full scale)
// 0 = 0
void DeviceInterfaceBase::normalizeIQ(CPX *cpx, qint8 I, qint8 Q)
{
	double tmp;
	//Normalize and apply gain
	cpx->re = (I / 128.0) * m_userIQGain * m_normalizeIQGain;
	cpx->im = (Q / 128.0) * m_userIQGain * m_normalizeIQGain;

	//Configure IQ order if not default
	switch(m_iqOrder) {
		case DeviceInterface::IQO_IQ:
			//No change, this is the default order
			break;
		case DeviceInterface::IQO_QI:
			tmp = cpx->re;
			cpx->re = cpx->im;
			cpx->im = tmp;
			break;
		case DeviceInterface::IQO_IONLY:
			cpx->im = cpx->re;
			break;
		case DeviceInterface::IQO_QONLY:
			cpx->re = cpx->im;
			break;
	}

}

void DeviceInterfaceBase::normalizeIQ(CPX* _out, CPX8* _in, quint32 _numSamples, bool _reverse)
{
	//Runs at full device sample rate, optimize loop invariants like scale, object copies and switches
	double scale = 1/128.0 * m_userIQGain * m_normalizeIQGain;
	//Only check iqOrder once per loop instead of every sample
	IQOrder tmpOrder = m_iqOrder;
	if (m_iqOrder == IQO_IQ && _reverse)
		tmpOrder = IQO_QI;
	else if (m_iqOrder == IQO_QI && _reverse)
		tmpOrder = IQO_IQ;
	switch(tmpOrder) {
		case DeviceInterface::IQO_IQ:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _in[i].im * scale;
			}
			break;
		case DeviceInterface::IQO_QI:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].im * scale;
				_out[i].im = _in[i].re * scale;
			}
			break;
		case DeviceInterface::IQO_IONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _out[i].re;
			}
			break;
		case DeviceInterface::IQO_QONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].im = _in[i].im * scale;
				_out[i].re = _out[i].im;
			}
			break;
	}
}

void DeviceInterfaceBase::normalizeIQ(CPX* _out, CPXU8* _in, quint32 _numSamples, bool _reverse)
{
	//Runs at full device sample rate, optimize loop invariants like scale, object copies and switches
	double scale = 1/128.0 * m_userIQGain * m_normalizeIQGain;
	//(I - 128) / 128.0) * userIQGain * normalizeIQGain
	//Only check iqOrder once per loop instead of every sample
	IQOrder tmpOrder = m_iqOrder;
	if (m_iqOrder == IQO_IQ && _reverse)
		tmpOrder = IQO_QI;
	else if (m_iqOrder == IQO_QI && _reverse)
		tmpOrder = IQO_IQ;

	switch(tmpOrder) {
		case DeviceInterface::IQO_IQ:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = (_in[i].re - 128.0) * scale;
				_out[i].im = (_in[i].im - 128.0) * scale;
			}
			break;
		case DeviceInterface::IQO_QI:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = (_in[i].im - 128.0) * scale;
				_out[i].im = (_in[i].re - 128.0) * scale;
			}
			break;
		case DeviceInterface::IQO_IONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = (_in[i].re - 128.0) * scale;
				_out[i].im = _out[i].re;
			}
			break;
		case DeviceInterface::IQO_QONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].im = (_in[i].im - 128.0) * scale;
				_out[i].re = _out[i].im;
			}
			break;
	}
}

void DeviceInterfaceBase::normalizeIQ(CPX *_out, CPX16 *_in, quint32 _numSamples, bool _reverse)
{
	//Runs at full device sample rate, optimize loop invariants like scale, object copies and switches
	double scale = 1/32768.0 * m_userIQGain * m_normalizeIQGain;
	//Only check iqOrder once per loop instead of every sample
	IQOrder tmpOrder = m_iqOrder;
	if (m_iqOrder == IQO_IQ && _reverse)
		tmpOrder = IQO_QI;
	else if (m_iqOrder == IQO_QI && _reverse)
		tmpOrder = IQO_IQ;
	switch(tmpOrder) {
		case DeviceInterface::IQO_IQ:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _in[i].im * scale;
			}
			break;
		case DeviceInterface::IQO_QI:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].im * scale;
				_out[i].im = _in[i].re * scale;
			}
			break;
		case DeviceInterface::IQO_IONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _out[i].re;
			}
			break;
		case DeviceInterface::IQO_QONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].im = _in[i].im * scale;
				_out[i].re = _out[i].im;
			}
			break;
	}
}

void DeviceInterfaceBase::normalizeIQ(CPX *_out, CPX *_in, quint32 _numSamples, bool _reverse)
{
	//Runs at full device sample rate, optimize loop invariants like scale, object copies and switches
	double scale = m_userIQGain * m_normalizeIQGain;
	//Only check iqOrder once per loop instead of every sample
	IQOrder tmpOrder = m_iqOrder;
	if (m_iqOrder == IQO_IQ && _reverse)
		tmpOrder = IQO_QI;
	else if (m_iqOrder == IQO_QI && _reverse)
		tmpOrder = IQO_IQ;
	switch(tmpOrder) {
		case DeviceInterface::IQO_IQ:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _in[i].im * scale;
			}
			break;
		case DeviceInterface::IQO_QI:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].im * scale;
				_out[i].im = _in[i].re * scale;
			}
			break;
		case DeviceInterface::IQO_IONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _out[i].re;
			}
			break;
		case DeviceInterface::IQO_QONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].im = _in[i].im * scale;
				_out[i].re = _out[i].im;
			}
			break;
	}
}

void DeviceInterfaceBase::normalizeIQ(CPX *_out, CPXFLOAT *_in, quint32 _numSamples, bool _reverse)
{
	//Runs at full device sample rate, optimize loop invariants like scale, object copies and switches
	double scale = m_userIQGain * m_normalizeIQGain;
	//Only check iqOrder once per loop instead of every sample
	IQOrder tmpOrder = m_iqOrder;
	if (m_iqOrder == IQO_IQ && _reverse)
		tmpOrder = IQO_QI;
	else if (m_iqOrder == IQO_QI && _reverse)
		tmpOrder = IQO_IQ;
	switch(tmpOrder) {
		case DeviceInterface::IQO_IQ:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _in[i].im * scale;
			}
			break;
		case DeviceInterface::IQO_QI:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].im * scale;
				_out[i].im = _in[i].re * scale;
			}
			break;
		case DeviceInterface::IQO_IONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _in[i].re * scale;
				_out[i].im = _out[i].re;
			}
			break;
		case DeviceInterface::IQO_QONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].im = _in[i].im * scale;
				_out[i].re = _out[i].im;
			}
			break;
	}

}

void DeviceInterfaceBase::normalizeIQ(CPX *_out, short *_inI, short *_inQ, quint32 _numSamples, bool _reverse)
{
	//Runs at full device sample rate, optimize loop invariants like scale, object copies and switches
	double scale = 1/32768.0 * m_userIQGain * m_normalizeIQGain;
	//Only check iqOrder once per loop instead of every sample
	IQOrder tmpOrder = m_iqOrder;
	if (m_iqOrder == IQO_IQ && _reverse)
		tmpOrder = IQO_QI;
	else if (m_iqOrder == IQO_QI && _reverse)
		tmpOrder = IQO_IQ;
	switch(tmpOrder) {
		case DeviceInterface::IQO_IQ:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _inI[i] * scale;
				_out[i].im = _inQ[i] * scale;
			}
			break;
		case DeviceInterface::IQO_QI:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _inQ[i] * scale;
				_out[i].im = _inI[i] * scale;
			}
			break;
		case DeviceInterface::IQO_IONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].re = _inI[i] * scale;
				_out[i].im = _out[i].re;
			}
			break;
		case DeviceInterface::IQO_QONLY:
			for (quint32 i=0; i < _numSamples; i++) {
				_out[i].im = _inQ[i] * scale;
				_out[i].re = _out[i].im;
			}
			break;
	}
}

//These per sample methods are deprecated and can be removed
void DeviceInterfaceBase::normalizeIQ(CPX *cpx, CPX iq)
{
	double tmp;
	//Normalize and apply gain
	cpx->re = iq.re * m_userIQGain * m_normalizeIQGain;
	cpx->im = iq.im * m_userIQGain * m_normalizeIQGain;

	//Configure IQ order if not default
	switch(m_iqOrder) {
		case DeviceInterface::IQO_IQ:
			//No change, this is the default order
			break;
		case DeviceInterface::IQO_QI:
			tmp = cpx->re;
			cpx->re = cpx->im;
			cpx->im = tmp;
			break;
		case DeviceInterface::IQO_IONLY:
			cpx->im = cpx->re;
			break;
		case DeviceInterface::IQO_QONLY:
			cpx->re = cpx->im;
			break;
	}

}

//Called by audio devices when new samples are available
//Samples are paired I and Q
//numSamples is total number of samples I + Q
void DeviceInterfaceBase::audioProducer(float *samples, quint16 numSamples)
{
	//NormalizeIQGain is determined by the audio library we're using, not the SDR
	//Set to -60db at 10mhz .0005v signal generator input
#ifdef USE_QT_AUDIO
	m_normalizeIQGain = DB::dBToAmplitude(-18.00);
#endif
#ifdef USE_PORT_AUDIO
	normalizeIQGain = DB::dBToAmplitude(-4.00);
#endif
	if (numSamples / 2 != m_framesPerBuffer) {
		qDebug()<<"Invalid number of audio samples";
		return;
	}
	for (int i=0, j=0; i<m_framesPerBuffer; i++, j+=2) {
		normalizeIQ( &m_audioInputBuffer[i], samples[j], samples[j+1]);
	}
	processIQData(m_audioInputBuffer, m_framesPerBuffer);
}
