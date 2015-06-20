//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "audioqt.h"
#include <QDebug>

AudioQT::AudioQT(cbAudioProducer cb, int fpb):Audio()
{
	AudioProducer = cb;

	framesPerBuffer = fpb;

	this->OutputDeviceList();
	this->InputDeviceList();

    qaAudioOutput = NULL;
    qaAudioInput = NULL;
    cpxOutBuffer = new CPX[framesPerBuffer];
    outStreamBuffer = new float[framesPerBuffer * 2]; //Max we'll ever see
    inStreamBuffer = new float[framesPerBuffer * 2 * 2]; //Max we'll ever see
}

AudioQT::~AudioQT()
{
}

int AudioQT::StartInput(QString inputDeviceName, int _inputSampleRate)
{
    inputSampleRate = _inputSampleRate;
    //qaDevice = QAudioDeviceInfo::defaultInputDevice();
    qaInputDevice = FindInputDeviceByName(inputDeviceName);

    //DumpDeviceInfo(qaInputDevice); //Use this to see supported formats

    qaFormat.setSampleRate(inputSampleRate);
    qaFormat.setChannelCount(2);
    qaFormat.setCodec("audio/pcm");
    qaFormat.setByteOrder(QAudioFormat::LittleEndian);
    //Same as PortAudio PAFloat32
    qaFormat.setSampleSize(32);
    qaFormat.setSampleType(QAudioFormat::Float);

    QAudioDeviceInfo info(qaInputDevice);
    if (!info.isFormatSupported(qaFormat)) {
        qWarning() << "QAudio input format not supported";
        //qaFormat = info.nearestFormat(qaFormat);
        return -1;
    }

    //set notify interval and connect to notify signal, calls us back whenever n ms of data is available
    qaAudioInput = new QAudioInput(qaInputDevice, qaFormat, this);

    //Want to get called back whenever there's a buffer full of data
	int notifyInterval = (float)framesPerBuffer / ((float)inputSampleRate * 1.5)* 1000.0;
    qaAudioInput->setNotifyInterval(notifyInterval);  //Experiment with this, has to be fast enough to keep up with sample rate
    //Set up our call back
    //connect(qaAudioInput,SIGNAL(notify()),this,SLOT(ProcessInputData()));
    //New QT5 connect syntax for compile time checking
    connect(qaAudioInput,&QAudioInput::notify,this,&AudioQT::ProcessInputData);
    //This sets the max # of samples and is in units of sampleformat
    //So 2048 float samples will return 8192 bytes
    qaAudioInput->setBufferSize(framesPerBuffer*2*2); //2x Extra room
	qaAudioInput->setVolume(5); //Default is 1
    //We should start seeing callbacks after start
    inputDataSource = qaAudioInput->start();

    return 0;
}

void AudioQT::ProcessInputData()
{
    qint64 bytesAvailable = qaAudioInput->bytesReady();
    qint64 bytesPerBuffer = framesPerBuffer * sizeof(float) *2;
    if (bytesAvailable < bytesPerBuffer)
        return;
    //qDebug()<<"ProcessInputData "<<bytesAvailable;

	mutex.lock(); //Make sure inSteamBuffer doesn't get trashed if we take too long
	qint64 bytesRead = inputDataSource->read((char*)inStreamBuffer,bytesPerBuffer);
	if (bytesRead != bytesPerBuffer) {
        qDebug()<<"AudioQt bytesRead != bytesPerBuffer";
		mutex.unlock();
		return;
	}
	//If audio input is too high, adjust volume in StartInput
	AudioProducer(inStreamBuffer, bytesRead / sizeof(float));
	mutex.unlock();

}

int AudioQT::StartOutput(QString outputDeviceName, int _outputSampleRate)
{
    //Don't set sample rate in construct, only here do we know actual rate
    outputSampleRate = _outputSampleRate;

    //qaDevice = QAudioDeviceInfo::defaultOutputDevice();
    qaOutputDevice = FindOutputDeviceByName(outputDeviceName);

    //DumpDeviceInfo(qaOutputDevice); //Use this to see supported formats

    qaFormat.setSampleRate(outputSampleRate);
    qaFormat.setChannelCount(2);
    qaFormat.setCodec("audio/pcm");
    qaFormat.setByteOrder(QAudioFormat::LittleEndian);
    //Testedwith Int/16 and Float32
    //Same as PortAudio PAFloat32
    qaFormat.setSampleSize(32);
    qaFormat.setSampleType(QAudioFormat::Float);

    //qaAudioOutput->setBufferSize(framesPerBuffer*sizeof(CPX)); //Optional??
    //qaAudioOutput->setNotifyInterval(42); //Only if we want callbacks

    QAudioDeviceInfo info(qaOutputDevice);
    if (!info.isFormatSupported(qaFormat)) {
        qWarning() << "QAudio output format not supported";
        //qaFormat = info.nearestFormat(qaFormat);
        return -1;
    }

    qaAudioOutput = new QAudioOutput(qaOutputDevice, qaFormat, this);

    outputDataSource = qaAudioOutput->start();
	return 0;
}
int AudioQT::Stop()
{
    if (qaAudioOutput)
        qaAudioOutput->stop();
    if (qaAudioInput)
        qaAudioInput->stop();

	return 0;
}
int AudioQT::Flush()
{
	return 0;
}
int AudioQT::Pause()
{
	return 0;
}
int AudioQT::Restart()
{
	return 0;
}
void AudioQT::SendToOutput(CPX *out, int outSamples, float gain, bool mute)
{
    if (!qaAudioOutput)
        return;

	//We may have to skip samples to reduce rate to match audio out, decimate set when we
	//opened stream
	qint64 bytesWritten = 0;
    //qint16 left,right;
    for (int i=0, j=0;i<outSamples;i++, j+=2)
	{
        if (mute)
            out[i].re = out[i].im = 0;

        if (gain != 1)
            out[i] *= gain;

        //If we use Int 16
        //left = Float2Int(out[i].re);
        //right = Float2Int(out[i].im);

        //No conversion necessary from cpx
        outStreamBuffer[j] = out[i].re;
        outStreamBuffer[j+1] = out[i].im;

	}
    bytesWritten = outputDataSource->write((char*)outStreamBuffer,outSamples*sizeof(float)*2);
    //if (bytesWritten != data.length())
	//	true;
}
void AudioQT::ClearCounts()
{

}
QAudioDeviceInfo AudioQT::FindInputDeviceByName(QString name)
{
    QAudioDeviceInfo device;

    foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        //qDebug()<<name<<"/"<<device.deviceName();
        if (device.deviceName() == name)
            return device;
    }
    qDebug()<<"QTAudio input device not found, using default";
    return QAudioDeviceInfo::defaultInputDevice();

}
QAudioDeviceInfo AudioQT::FindOutputDeviceByName(QString name)
{
    QAudioDeviceInfo device;
    foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (device.deviceName() == name)
            return device;
    }
    qDebug()<<"QTAudio output device not found, using default";
    return QAudioDeviceInfo::defaultOutputDevice();

}

QStringList AudioQT::InputDeviceList()
{
	QStringList inputDevices;
	QAudioDeviceInfo device;
    QString buf;
    int devId = 1;
    foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
		//DumpDeviceInfo(device);
        //Emulate same format as portAudio ID:Name
        buf = QString("%1:%2").arg(devId,2).arg(device.deviceName());
        inputDevices.append(buf);
        devId++;
    }

	return inputDevices;
}
QStringList AudioQT::OutputDeviceList()
{
	QStringList outputDevices;
	QAudioDeviceInfo device;
    QString buf;
    int devId = 1;
	foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
		//DumpDeviceInfo(device);
        //Emulate same format as portAudio ID:Name
        buf = QString("%1:%2").arg(devId,2).arg(device.deviceName());
        outputDevices.append(buf);
        devId++;
	}

	return outputDevices;
}


//Dumps everything we know about device
//Todo:  See if we need to install any optional QT audio plugins to get 24 bit, 192k, or float support
void AudioQT::DumpDeviceInfo(QAudioDeviceInfo device)
{
#if(1)
	//Look at QTGlobal for all debug options

	qDebug()<<device.deviceName();
	qDebug()<<"Supported Codecs";
	foreach (QString s,device.supportedCodecs())
		qDebug()<<s;
	//Why don't we see float support on any windows devices?
	qDebug()<<"Supported Sample Types: signed(1), unsigned(2), float(3)";
	foreach (QAudioFormat::SampleType s,device.supportedSampleTypes())
		qDebug()<<s;
	//No 192k devices in list?
	qDebug()<<"Supported Sample Rates";
	foreach (int s,device.supportedSampleRates())
		qDebug()<<s;
	//Not showing any 24bit support?
	qDebug()<<"Supported Sample Sizes";
	foreach (int s,device.supportedSampleSizes())
		qDebug()<<s;
    qDebug()<<"Supported Channels";
    foreach (int s,device.supportedChannelCounts())
		qDebug()<<s;
	qDebug()<<"Supported Byte Orders: BigEndian(0), LittleEndian(1)";
	foreach (QAudioFormat::Endian s,device.supportedByteOrders())
		qDebug()<<s;
	qDebug()<<"Supported Channel Counts";
	foreach (int s,device.supportedChannelCounts())
		qDebug()<<s;
	//Obsolete, use Supported Sample Rate
	//qDebug()<<"Supported Frequencies";
	//foreach (int s,device.supportedFrequencies())
	//	qDebug()<<s;
#endif
}
