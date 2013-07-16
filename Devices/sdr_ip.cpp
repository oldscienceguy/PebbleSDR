#include "sdr_ip.h"
#include "Demod.h"
#include "Settings.h"
#include "Receiver.h"

/*
 * SDR_IP inherits SDR and is the main Pebble device class
 * SDR_IP has a member that points to an instance of CSdrInterface, which handles all the hardware specific functions
 * CSdrInterface inherits CNetio, which handles the low level network protocol and could/should be used with other networked devices
 * CSdrInterface manages CDataProcess which handles all the specific data translation to CPX in a separate thread
 * CSdrInterface manages two threads
 *  CUdp is a thread to handle UDB
 *  CNetIO is another thread to handle TCP data
 *
 * Data flow
 * CSdrInterface (via CNetio) calls CSdrInterface to process each data buffer (see CDataProcess)
 * ProcessUdpData and ProcessTcpData
 *
 * Move CDataProcess into our normal SDR Producer thread?
 * First test, let everything work as in CuteSDR, but re-queue in sdr_ip producer thread so we can use normal consumer logic
 *  will be slower due to reque, but easier to test.
 * So CSdrInterface needs a callback to SDR_IP, pass in constructor
 *
 */

SDR_IP::SDR_IP(Receiver *_receiver, SDR::SDRDEVICE dev, Settings *_settings)
    :SDR(_receiver, dev, _settings)
{
    settings = _settings;

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = new QSettings(path+"/PebbleData/sdr_ip.ini",QSettings::IniFormat);
    ReadSettings();

    optionUi = NULL;

    //If settings is NULL we're getting called just to get defaults and device settings, check in destructor
    if (!settings)
        return;

    sampleRate = settings->sampleRate;

    framesPerBuffer = settings->framesPerBuffer;

    //inBuffer = new CPXBuf(framesPerBuffer);
    //outBuffer = new CPXBuf(framesPerBuffer);

    m_pSdrInterface = new CSdrInterface( this ); //Callback to us for incoming data
    //TcpClient emits signals, handled by CSdrInterface, and re-emited to us
    //This is how we get info back
    connect(m_pSdrInterface, SIGNAL(NewStatus(int)), this,  SLOT( OnStatus(int) ) );
    connect(m_pSdrInterface, SIGNAL(NewInfoData()), this,  SLOT( OnNewInfo() ) );  //ASCP data parsed with device info

    //Initial testing
    //The higher bandwidths can flood a network due to the high sample rate
    //I use a direct network connection to Mac to avoid this
    //Create a new network service using LAN, manual IP of 10.0.1.1 and standard 255.255.255.0 mask
    //Then set SDR-IP or AFEDRI to IP 10.0.1.100 with 10.0.1.1 gateway
    //
    //m_IPAdr = QHostAddress("10.0.1.100"); //Move to settings page
    //m_Port = 50000;
    m_Status = m_LastStatus = CSdrInterface::NOT_CONNECTED;
    m_RadioType = CSdrInterface::SDRIP;
    m_pSdrInterface->SetRadioType(m_RadioType);

    outBuffer = CPXBuf::malloc(framesPerBuffer);

    //producerThread = new SDRProducerThread(this);
    //producerThread->setRefresh(0);
    consumerThread = new SDRConsumerThread(this);
    consumerThread->setRefresh(0);


}

SDR_IP::~SDR_IP()
{
    if (!settings)
        return;

    WriteSettings();

    if (m_pSdrInterface != NULL)
        delete(m_pSdrInterface);
}

void SDR_IP::ReadSettings()
{
    SDR::ReadSettings();
    //sStartup = qSettings->value("Startup",10000000).toDouble();
    //sLow = qSettings->value("Low",100000).toDouble();
    //sHigh = qSettings->value("High",33000000).toDouble();
    //sStartupMode = qSettings->value("StartupMode",dmAM).toInt();
    //sGain = qSettings->value("Gain",1.0).toDouble();
    m_RfGain = qSettings->value("RFGain",0).toInt();
    m_IPAdr = QHostAddress(qSettings->value("IPAddr","10.0.1.100").toString());
    m_Port = qSettings->value("Port",50000).toInt();

}
void SDR_IP::WriteSettings()
{
    SDR::WriteSettings();
    //qSettings->setValue("Startup",sStartup);
    //qSettings->setValue("Low",sLow);
    //qSettings->setValue("High",sHigh);
    //qSettings->setValue("StartupMode",sStartupMode);
    //qSettings->setValue("Gain",sGain);
    //qSettings->setValue("Bandwidth",sBandwidth); //Enum
    qSettings->setValue("RFGain",m_RfGain);
    qSettings->setValue("IPAddr",m_IPAdr.toString());
    qSettings->setValue("Port",m_Port);

    qSettings->sync();
}

//This is where we handle status updates from sdrInterface and underlying TcpClient
//This acts as a state machine as well as updates status for possible use by UI
/*
 * State change possibilities
 *                                  /---New Status---\
 * (LastStatus-V)   NOT_CONNECTED   CONNECTING  CONNECTED       RUNNING     ERR         ADOVR
 * NOT_CONNECTED    NOP             NOP         Start?          NOP         NOP         NOP
 * CONNECTING
 * CONNECTED
 * RUNNING
 * ERR
 * ADOVR
 */

void SDR_IP::OnStatus(int status)
{
    m_Status = (CSdrInterface::eStatus)status;
//qDebug()<<"Status"<< m_Status;
    switch(status)
    {
        case CSdrInterface::NOT_CONNECTED:
            qDebug()<<"Status change: Not Connected";
            //Fall through
        case CSdrInterface::CONNECTING:
            qDebug()<<"Status change: Connecting";

            //If we were running, this means we lost connection
            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                //m_pSdrInterface->StopSdr();
                //ui->framePlot->SetRunningState(false);
            }
            //ui->statusBar->showMessage(tr("SDR Not Connected"), 0);
            //ui->pushButtonRun->setText(tr("Run"));
            //ui->pushButtonRun->setEnabled(false);
            break;

        case CSdrInterface::CONNECTED:
            qDebug()<<"Status change: Connected";
            //If we were RUNNING, but get a new Connected state, we must have lost connection - restart
            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                //m_pSdrInterface->StopSdr();
                //ui->framePlot->SetRunningState(false);
            }
            //ui->statusBar->showMessage( m_ActiveDevice + tr(" Connected"), 0);
            //If this is first time CONNECTED - Change from NOT_CONNECTED or CONNECTING, Get info from SDR
            if(	(m_LastStatus == CSdrInterface::NOT_CONNECTED) ||
                (m_LastStatus == CSdrInterface::CONNECTING) )
                    m_pSdrInterface->GetSdrInfo();
            //ui->pushButtonRun->setText(tr("Run"));
            //ui->pushButtonRun->setEnabled(true);
            break;

        case CSdrInterface::RUNNING:
            qDebug()<<"Status change: Running";
            break;
            /*
            m_Str.setNum(m_pSdrInterface->GetRateError());
            m_Str.append(tr(" ppm  Missed Pkts="));
            m_Str2.setNum(m_pSdrInterface->m_MissedPackets);
            m_Str.append(m_Str2);
            ui->statusBar->showMessage(m_ActiveDevice + tr(" Running   ") + m_Str, 0);
            ui->pushButtonRun->setText(tr("Stop"));
            ui->pushButtonRun->setEnabled(true);
            break;
            */

        case CSdrInterface::ERR:
            qDebug()<<"Status change: Err";

            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                m_pSdrInterface->StopSdr();
                //ui->framePlot->SetRunningState(false);
            }
            //ui->statusBar->showMessage(tr("SDR Not Connected"), 0);
            //ui->pushButtonRun->setText(tr("Run"));
            //ui->pushButtonRun->setEnabled(false);
            break;

        case CSdrInterface::ADOVR:
            qDebug()<<"Status change: ADOVR";
            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                m_Status = CSdrInterface::RUNNING;
                //ui->framePlot->SetADOverload(true);
            }
            break;

        default:
            qDebug()<<"Status change: Unknown";
            break;
    }
    m_LastStatus = m_Status;
}

void SDR_IP::OnNewInfo()
{
    qDebug()<<"Info received from device";
    qDebug()<<"Device name: "<<m_pSdrInterface->m_DeviceName;
    qDebug()<<"Serial #: "<<m_pSdrInterface->m_SerialNum;
    qDebug()<<"Boot Rev: "<<m_pSdrInterface->m_BootRev;
    qDebug()<<"App Rev: "<<m_pSdrInterface->m_AppRev;
    qDebug()<<"HW Rev: "<<m_pSdrInterface->m_HardwareRev;

}

bool SDR_IP::Connect()
{
    //
    if( (CSdrInterface::RUNNING == m_Status) || ( CSdrInterface::CONNECTED == m_Status) )
        m_pSdrInterface->KeepAlive();
    else if( CSdrInterface::NOT_CONNECTED == m_Status ) {
        //Wait for connection
        return m_pSdrInterface->ConnectToServer(m_IPAdr,m_Port,true);
    }
    return true;
}

bool SDR_IP::Disconnect()
{
    m_pSdrInterface->StopIO(); //Sends SDR stop command and delays slightly before calling DisconnectFromServer
    return true;
}

double SDR_IP::SetFrequency(double fRequested, double fCurrent)
{
    //We get double, but SetRxFreq want quint64, check cast
    m_CenterFrequency = m_pSdrInterface->SetRxFreq(fRequested);
    return m_CenterFrequency;
}

void SDR_IP::ShowOptions()
{
}

void SDR_IP::Start()
{
    producerOverflowCount = 0;

    InitProducerConsumer(50, framesPerBuffer * sizeof(CPX));

    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    consumerThread->start();
    //producerThread->start();
    isThreadRunning = true;

    m_pSdrInterface->m_MissedPackets = 0;
    m_pSdrInterface->SetSdrBandwidth(m_pSdrInterface->LookUpBandwidth(sampleRate));
    m_RfGain = 0; //index to table
    m_pSdrInterface->SetSdrRfGain( m_RfGain );
    m_pSdrInterface->StartSdr();

}

void SDR_IP::Stop()
{
    if(CSdrInterface::RUNNING == m_Status)
    {
        m_pSdrInterface->StopSdr();
    }
    if (isThreadRunning) {
        //producerThread->stop();
        consumerThread->stop();
        isThreadRunning = false;
    }
}

double SDR_IP::GetStartupFrequency()
{
    return 800000;
}

int SDR_IP::GetStartupMode()
{
    return 0;
}

double SDR_IP::GetHighLimit()
{
    return 30000000;
}

double SDR_IP::GetLowLimit()
{
    return 100000; //AFEDRI SDR-Net
}

double SDR_IP::GetGain()
{
    return 1;
}

QString SDR_IP::GetDeviceName()
{
    return "SDR_IP";
}

int SDR_IP::GetSampleRate()
{
    return sampleRate;
    //Get actual sample rate from device
    //return m_pSdrInterface->LookUpSampleRate(sampleRate);
}

int* SDR_IP::GetSampleRates(int &len)
{
    len = 4;
    //Ugly, but couldn't find easy way to init with {1,2,3} array initializer
    //Assume SDR_IP for now, add other devices later
    //These must be the same values as in sdrinterface.c
    sampleRates[0] = 62500; //50000;
    sampleRates[1] = 250000; //200000;
    sampleRates[2] = 500000; //500000;
    sampleRates[3] = 2000000; //1800000;
    return sampleRates;

}

//Called from CUdp thread when it's processed a packet
//We need to be as fast as possible and return to CUdp
//So no blocking, even when we overflow
//sdrinterface ensures we always get called with numFramesPerBuffer (2048) CPX
void SDR_IP::PutInProducerQ(CPX *cpxBuf, quint32 numCpx)
{
    if (numCpx != framesPerBuffer)
        return; //Defensive

    if (!IsFreeBufferAvailable())
        return;

    //This won't block because we checked above
    AcquireFreeBuffer();

    //ProducerBuffer is byte array, cast as CPX
    CPXBuf::copy(((CPX **)producerBuffer)[nextProducerDataBuf],cpxBuf,numCpx);
    //Circular buffer of dataBufs
    IncrementProducerBuffer();

    //Increment the number of data buffers that are filled so consumer thread can access
    ReleaseFilledBuffer();
}

void SDR_IP::RunConsumerThread()
{
    //Wait for data to be available from producer
    AcquireFilledBuffer();

    if (receiver != NULL)
        receiver->ProcessBlock(((CPX **)producerBuffer)[nextConsumerDataBuf],outBuffer,framesPerBuffer);

    IncrementConsumerBuffer();
    ReleaseFreeBuffer();
}

void SDR_IP::StopConsumerThread()
{
    //Problem - RunConsumerThread may be in process when we're asked to stopp
    //We have to wait for it to complete, then return.  Bad dependency - should not have tight connection like this
}

//Installs our option widget in setup dialog
void SDR_IP::SetupOptionUi(QWidget *parent)
{
    //This sets up the sdr specific widget
    if (optionUi != NULL)
        delete optionUi;
    optionUi = new Ui::SDRIP();
    optionUi->setupUi(parent);
    parent->setVisible(true);

    //Read options file and display
    optionUi->ipAddress->setText(m_IPAdr.toString());
    //Set mask after we set text, otherwise won't parse correctly
    optionUi->ipAddress->setInputMask("000.000.000.000");
    optionUi->port->setText(QString::number(m_Port));

    //connect(o->pushButton,SIGNAL(clicked(bool)),this,SLOT(Test2(bool)));
    //Debugging tip using qApp as known slot, this works
    //connect(o->pushButton,SIGNAL(clicked(bool)),qApp,SLOT(aboutQt()));
    //New QT5 syntax using pointer to member functions
    //connect(o->pushButton,&QPushButton::clicked, this, &SDR_IP::Test2);
    //Lambda function example: needs compiler flag -std=c++0x
    //connect(o->pushButton,&QPushButton::clicked,[=](bool b){qDebug()<<"test";});

}

void SDR_IP::WriteOptionUi()
{
    m_IPAdr = QHostAddress(optionUi->ipAddress->text());
    m_Port = optionUi->port->text().toInt();
    WriteSettings();
}

