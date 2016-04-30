#ifndef WWVDIGITALMODEM_H
#define WWVDIGITALMODEM_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "digital_modem_interfaces.h"
#include "fldigifilters.h"
#include "downconvert.h"
#include "iir.h"
#include "ui_data-wwv.h"

//Experiment extracting from ntp
class MatchedFilter
{
public:
    MatchedFilter(quint16 _sampleRate, quint16 _detectedSignalMs, quint32 _detectedSignalFrequency);

    //Running results
    double mag() {return out.mag();} //Energy in filter sqrt(sum of squares)

    CPX out;
    double phase;

    bool ProcessSample(CPX in);

    void ProcessSamples(quint16 len, CPX *in, double *output);
private:
    //NOTE: NUM_SINES has to be evenly divisible into 360deg in order to use a fixed table
    //If this is a problem, we can always calc sine on the fly as in other NCO's
    CPX *ncoTable; //Array of sin/cos pairs that generate detectedSignalFrequency NCO
    double *outBuf;
    quint16 sampleRate;
    quint16 detectedSignalFrequency; //Signal we're looking for
    float msPerSample;
	//quint16 phaseShiftSamples; //Num sample to shift phase by 90 deg

    quint16 ncoPtr; //Steps through sintab to output desired NCO freq
    quint16 delayPtr;	//Circular buffer ptr to delay line
    CPX *delay;
    quint16 detectedSignalMs; //How many ms of data is the filter supposed to match, ie length of tone
    quint16 lenFilter;

    //NCO
    double ncoPhase;
    double ncoIncrement;

    //Peak detector
    quint16 slopesToAverage; //Number of slope calculations to averge to make sure we have valid peak
    quint16 slopeUpCounter;
    quint16 slopeDownCounter;
    double lastMag;
    qint16 lastSlope;

};

//Interface has to come after QObject or we get linker errors
class WWVDigitalModem  : public QObject, public DigitalModemInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DigitalModemInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DigitalModemInterface)

public:

    WWVDigitalModem();
    ~WWVDigitalModem();

    void setSampleRate(int _sampleRate, int _sampleCount);

    //Setup demod mode etc
    void setDemodMode(DeviceInterface::DEMODMODE _demodMode);

    //Process samples
    CPX * processBlock(CPX * in);

    //Setup UI in context of parent
    void setupDataUi(QWidget *parent);

    //Info - return plugin name for menus
    QString getPluginName();

    //Additional info
    QString getPluginDescription();

    QObject *asQObject() {return (QObject *)this;}

    void Process100Hz(CPX in);
    void Process1000Hz(CPX in);

signals:
    void testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
    void testbench(int _length, double* _buf, double _sampleRate, int _profile);
    bool addProfile(QString profileName, int profileNumber); //false if profilenumber already exists
    void removeProfile(quint16 profileNumber);

private:
    enum TestBenchProfiles {
        WWVModem = 100
    };

    Ui::dataWWV *dataUi;
    quint32 modemClock;

    int numSamples;
    int sampleRate;
    CPX *out;
    CPX *workingBuf;
    double *ms170Buf;

    CDownConvert modemDownConvert; //Get to modem rate and mix

    static const int bandPassLen = 512; //Review
    C_FIR_filter bandPass;
    static const int lowPassLen = 512;
    CIir lowPass;

    CPX bandPassOut;
    CPX lowPassOut;

    int modemSampleRate;

    MatchedFilter *ms170;
    MatchedFilter *ms5; //5ms 1khz second pulse
    MatchedFilter *ms800; //800ms ikhz minute pulse

};



#endif // WWVDIGITALMODEM_H
