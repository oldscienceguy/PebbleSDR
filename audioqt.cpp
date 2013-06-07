//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "audioqt.h"
#include <QDebug>
#include "Settings.h"

AudioQT::AudioQT(Receiver *r,int sr, int fpb, Settings *s)
{
	framesPerBuffer = fpb;
	inputSampleRate = sr;
	outputSampleRate = sr;
	settings = s;
	receiver=r;

	this->OutputDeviceList();
	this->InputDeviceList();

    qaAudioOutput = NULL;
    outStreamBuffer = new float[framesPerBuffer * 2]; //Max we'll ever see
}

int AudioQT::Start(int _inputSampleRate, int _outputSampleRate)
{
    //Don't set sample rate in construct, only here do we know actual rate
    inputSampleRate = _inputSampleRate;
    outputSampleRate = _outputSampleRate;

    //Test ouput only
    //qaDevice = QAudioDeviceInfo::defaultOutputDevice();
    qaDevice = FindOutputDeviceByName(settings->outputDeviceName);

    //DumpDeviceInfo(qaDevice); //Use this to see supported formats

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

    QAudioDeviceInfo info(qaDevice);
    if (!info.isFormatSupported(qaFormat)) {
        qWarning() << "Default format not supported";
        //qaFormat = info.nearestFormat(qaFormat);
        return -1;
    }

    qaAudioOutput = new QAudioOutput(qaDevice, qaFormat, this);

	dataSource = qaAudioOutput->start();
	return 0;
}
int AudioQT::Stop()
{
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
void AudioQT::SendToOutput(CPX *out, int outSamples)
{
    if (!qaAudioOutput)
        return;

	//We may have to skip samples to reduce rate to match audio out, decimate set when we
	//opened stream
	qint64 bytesWritten = 0;
    //qint16 left,right;
    for (int i=0, j=0;i<outSamples;i++, j+=2)
	{
        //If we use Int 16
        //left = Float2Int(out[i].re);
        //right = Float2Int(out[i].im);

        //No conversion necessary from cpx
        outStreamBuffer[j] = out[i].re;
        outStreamBuffer[j+1] = out[i].im;

	}
    bytesWritten = dataSource->write((char*)outStreamBuffer,outSamples*sizeof(float)*2);
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
        if (device.deviceName() == name)
            return device;
    }
    return QAudioDeviceInfo::defaultInputDevice();

}
QAudioDeviceInfo AudioQT::FindOutputDeviceByName(QString name)
{
    QAudioDeviceInfo device;
    foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (device.deviceName() == name)
            return device;
    }
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
