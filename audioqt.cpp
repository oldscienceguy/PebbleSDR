//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "audioqt.h"
#include <QDebug>

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
}

int AudioQT::Start(int _inputSampleRate, int _outputSampleRate)
{
    //Don't set sample rate in construct, only here do we know actual rate
    inputSampleRate = _inputSampleRate;
    outputSampleRate = _outputSampleRate;

    //Test ouput only
    qaDevice = QAudioDeviceInfo::defaultOutputDevice();
    //DumpDeviceInfo(qaDevice);

    qaFormat.setSampleRate(outputSampleRate);
    qaFormat.setChannelCount(2);
    qaFormat.setSampleSize(16);
    qaFormat.setCodec("audio/pcm");
    qaFormat.setByteOrder(QAudioFormat::LittleEndian);
    qaFormat.setSampleType(QAudioFormat::SignedInt);

    //qaAudioOutput->setBufferSize(framesPerBuffer*sizeof(CPX));
    //qaAudioOutput->setNotifyInterval(42);

    QAudioDeviceInfo info(qaDevice);
    if (!info.isFormatSupported(qaFormat)) {
        qWarning() << "Default format not supported";
        //qaFormat = info.nearestFormat(qaFormat);
        return -1;
    }

    qaAudioOutput = new QAudioOutput(qaDevice, qaFormat, this);

	dataSource = qaAudioOutput->start();
	//dataSource->open(QIODevice::ReadWrite);
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
	QByteArray data;
	//data.resize(framesPerBuffer*sizeof(CPX));
    for (int i=0;i<outSamples;i++)
	{
		//Float not supported by QT
        qint16 left = Float2Int(out[i].re);
        qint16 right = Float2Int(out[i].im);

		//Everytime we write to QIODevice, it emits a signal
		//This caused problems, so we write everything to a ByteArray and then output
		//float left = out[i*decimate].re;
		//float right = out[i*decimate].im;

		data.append((char*)&left,sizeof(left));
		data.append((char*)&right,sizeof(right));
	}
	bytesWritten = dataSource->write(data);
	//if (bytesWritten != data.length())
	//	true;
}
void AudioQT::ClearCounts()
{

}
QStringList AudioQT::InputDeviceList()
{
	QStringList inputDevices;
	QAudioDeviceInfo device;
	foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
		//DumpDeviceInfo(device);

		inputDevices.append(device.deviceName());
									   //qVariantFromValue(device));
	}

	return inputDevices;
}
QStringList AudioQT::OutputDeviceList()
{
	QStringList outputDevices;
	QAudioDeviceInfo device;
	foreach (device, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
		//DumpDeviceInfo(device);

		outputDevices.append(device.deviceName());
									   //qVariantFromValue(device));
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
