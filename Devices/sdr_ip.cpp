#include "sdr_ip.h"


SDR_IP::SDR_IP(Receiver *_receiver, SDR::SDRDEVICE dev, Settings *_settings)
    :SDR(_receiver, dev, _settings)
{
    //If settings is NULL we're getting called just to get defaults, check in destructor
    settings = _settings;
    if (!settings)
        return;

    m_pSdrInterface = new CSdrInterface();
    //TcpClient emits signals, handled by CSdrInterface, and re-emited to us
    //This is how we get info back
    connect(m_pSdrInterface, SIGNAL(NewStatus(int)), this,  SLOT( OnStatus(int) ) );

    //Initial testing
    m_IPAdr = QHostAddress("192.168.0.222");
    m_Port = 50000;
    m_Status = CSdrInterface::NOT_CONNECTED;
    m_RadioType = CSdrInterface::SDRIP;
    m_pSdrInterface->SetRadioType(m_RadioType);

}

SDR_IP::~SDR_IP()
{
    if (!settings)
        return;

    if (m_pSdrInterface != NULL)
        delete(m_pSdrInterface);
}

//This is where we handle status updates from sdrInterface and underlying TcpClient
void SDR_IP::OnStatus(int status)
{
    m_Status = (CSdrInterface::eStatus)status;
//qDebug()<<"Status"<< m_Status;
    switch(status)
    {
        case CSdrInterface::NOT_CONNECTED:
            qDebug()<<"Status change: Not Connected";
            break;
        case CSdrInterface::CONNECTING:
            qDebug()<<"Status change: Connecting";
            break;
            //if(	m_LastStatus == CSdrInterface::RUNNING)
            //{
                //m_pSdrInterface->StopSdr();
                //ui->framePlot->SetRunningState(false);
            //}
            //ui->statusBar->showMessage(tr("SDR Not Connected"), 0);
            //ui->pushButtonRun->setText(tr("Run"));
            //ui->pushButtonRun->setEnabled(false);
            break;
        case CSdrInterface::CONNECTED:
            qDebug()<<"Status change: Connected";
            break;
            /*
            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                m_pSdrInterface->StopSdr();
                ui->framePlot->SetRunningState(false);
            }
            ui->statusBar->showMessage( m_ActiveDevice + tr(" Connected"), 0);
            if(	(m_LastStatus == CSdrInterface::NOT_CONNECTED) ||
                (m_LastStatus == CSdrInterface::CONNECTING) )
                    m_pSdrInterface->GetSdrInfo();
            ui->pushButtonRun->setText(tr("Run"));
            ui->pushButtonRun->setEnabled(true);
            break;
            */
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
            break;
            /*
            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                m_pSdrInterface->StopSdr();
                ui->framePlot->SetRunningState(false);
            }
            ui->statusBar->showMessage(tr("SDR Not Connected"), 0);
            ui->pushButtonRun->setText(tr("Run"));
            ui->pushButtonRun->setEnabled(false);
            break;
            */
        case CSdrInterface::ADOVR:
            qDebug()<<"Status change: ADOVR";
            break;
            /*
            if(	m_LastStatus == CSdrInterface::RUNNING)
            {
                m_Status = CSdrInterface::RUNNING;
                ui->framePlot->SetADOverload(true);
            }
            break;
            */
        default:
            qDebug()<<"Status change: Unknown";
            break;
    }
    m_LastStatus = m_Status;
}

bool SDR_IP::Connect()
{
    //if( (CSdrInterface::RUNNING == m_Status) || ( CSdrInterface::CONNECTED == m_Status) )
    //    m_pSdrInterface->KeepAlive();
    if( CSdrInterface::NOT_CONNECTED == m_Status )
        m_pSdrInterface->ConnectToServer(m_IPAdr,m_Port);

    return true; //Not sure if connected until we get status update
}

bool SDR_IP::Disconnect()
{
    //m_pSdrInterface->DisconnectFromServer();
    m_pSdrInterface->StopIO(); //Sends SDR stop command and delays slightly before calling DisconnectFromServer
    return true;
}

double SDR_IP::SetFrequency(double fRequested, double fCurrent)
{
    return fRequested;
}

void SDR_IP::ShowOptions()
{
}

void SDR_IP::Start()
{
    if( CSdrInterface::CONNECTED == m_Status)
    {
        //m_CenterFrequency = m_pSdrInterface->SetRxFreq(m_CenterFrequency);
        //m_pSdrInterface->SetDemodFreq(m_CenterFrequency - m_DemodFrequency);
        m_pSdrInterface->StartSdr();
        m_pSdrInterface->m_MissedPackets = 0;

        //ui->framePlot->SetRunningState(true);
        //InitPerformance();
        //m_RdsDecode.DecodeReset(m_USFm);
    }

}

void SDR_IP::Stop()
{
    if(CSdrInterface::RUNNING == m_Status)
    {
        m_pSdrInterface->StopSdr();
        //ui->framePlot->SetRunningState(false);
        //ReadPerformance();
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
