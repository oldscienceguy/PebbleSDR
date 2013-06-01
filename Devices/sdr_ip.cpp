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
    //If settings is NULL we're getting called just to get defaults, check in destructor
    settings = _settings;
    if (!settings)
        return;

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = new QSettings(path+"/PebbleData/sdr_ip.ini",QSettings::IniFormat);
    //ReadSettings();

    sampleRate = settings->sampleRate;
    framesPerBuffer = settings->framesPerBuffer;

    //inBuffer = new CPXBuf(framesPerBuffer);
    //outBuffer = new CPXBuf(framesPerBuffer);

    m_pSdrInterface = new CSdrInterface( this ); //Callback to us for incoming data
    //TcpClient emits signals, handled by CSdrInterface, and re-emited to us
    //This is how we get info back
    connect(m_pSdrInterface, SIGNAL(NewStatus(int)), this,  SLOT( OnStatus(int) ) );

    //Initial testing
    m_IPAdr = QHostAddress("192.168.0.222");
    m_Port = 50000;
    m_Status = m_LastStatus = CSdrInterface::NOT_CONNECTED;
    m_RadioType = CSdrInterface::SDRIP;
    m_pSdrInterface->SetRadioType(m_RadioType);

    producerBuffer = new unsigned char *[numDataBufs];
    for (int i=0; i<numDataBufs; i++)
        producerBuffer[i] = new unsigned char [framesPerBuffer];

    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;

    producerThread = new SDRProducerThread(this);
    producerThread->setRefresh(0);
    consumerThread = new SDRConsumerThread(this);
    consumerThread->setRefresh(0);

}

SDR_IP::~SDR_IP()
{
    if (!settings)
        return;

    if (m_pSdrInterface != NULL)
        delete(m_pSdrInterface);
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
                m_pSdrInterface->StopSdr();
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
                m_pSdrInterface->StopSdr();
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
    //Start out with all producer buffers available
    if (semNumFreeBuffers != NULL)
        delete semNumFreeBuffers;
    semNumFreeBuffers = new QSemaphore(numDataBufs);

    if (semNumFilledBuffers != NULL)
        delete semNumFilledBuffers;
    //Init with zero available
    semNumFilledBuffers = new QSemaphore(0);

    nextProducerDataBuf = nextConsumerDataBuf = 0;

    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    consumerThread->start();
    producerThread->start();
    isThreadRunning = true;

    if( CSdrInterface::CONNECTED == m_Status)
    {
        m_pSdrInterface->StartSdr();
        m_pSdrInterface->m_MissedPackets = 0;
    }

}

void SDR_IP::Stop()
{
    if(CSdrInterface::RUNNING == m_Status)
    {
        m_pSdrInterface->StopSdr();
    }
    if (isThreadRunning) {
        producerThread->stop();
        consumerThread->stop();
        isThreadRunning = false;
    }
}

double SDR_IP::GetStartupFrequency()
{
    return 10000000;
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
    return 500000;
}

double SDR_IP::GetGain()
{
    return 1;
}

QString SDR_IP::GetDeviceName()
{
    return "SDR_IP";
}

//These are called from Producer/Consumer threads to do the work
void SDR_IP::RunProducerThread()
{
    int bytesRead;

    //This will block if we don't have any free data buffers to use, pending consumer thread releasing
    semNumFreeBuffers->acquire();
}

void SDR_IP::StopProducerThread()
{

}

void SDR_IP::RunConsumerThread()
{
    double fpSampleRe;
    double fpSampleIm;

    //Wait for data to be available from producer
    semNumFilledBuffers->acquire();
}

void SDR_IP::StopConsumerThread()
{
    //Problem - RunConsumerThread may be in process when we're asked to stopp
    //We have to wait for it to complete, then return.  Bad dependency - should not have tight connection like this
}
