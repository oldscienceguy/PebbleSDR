#ifndef WWVDIGITALMODEM_H
#define WWVDIGITALMODEM_H

#include "digital_modem_interfaces.h"
#include "fldigifilters.h"
#include "downconvert.h"
#include "ui/ui_data-wwv.h"

//Experiment extracting from ntp
class MatchedFilter
{
public:
    MatchedFilter(quint16 _sampleRate, quint16 _detectedSignalMs, quint32 _detectedSignalFrequency);

    //Running results
    double mag() {return out.sqrMag();} //Energy in filter

    CPX out;
    double phase;

    void ProcessSample(CPX in);

private:
    //NOTE: NUM_SINES has to be evenly divisible into 360deg in order to use a fixed table
    //If this is a problem, we can always calc sine on the fly as in other NCO's
    static const int NUM_SINES = 80;
    double sintab[NUM_SINES + 1];

    quint16 sampleRate;
    quint16 detectedSignalFrequency; //Signal we're looking for
    float msPerSample;
    quint16 phaseShiftSamples; //Num sample to shift phase by 90 deg

    quint16 ncoPtr; //Steps through sintab to output desired NCO freq
    quint16 delayPtr;	//Circular buffer ptr to delay line
    CPX *delay;
    quint16 msDetectedSignal; //How many ms of data is the filter supposed to match, ie length of tone
    quint16 lenFilter;

    //NCO
    double ncoPhase;
};

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

    void SetSampleRate(int _sampleRate, int _sampleCount);

    //Setup demod mode etc
    void SetDemodMode(DEMODMODE _demodMode);

    //Process samples
    CPX * ProcessBlock(CPX * in);

    //Setup UI in context of parent
    void SetupDataUi(QWidget *parent);

    //Info - return plugin name for menus
    QString GetPluginName();

    //Additional info
    QString GetDescription();

    void Process100Hz(CPX in);
    void Process1000Hz(CPX in);
private:
    Ui::dataWWV *dataUi;

    int numSamples;
    int sampleRate;
    CPX *out;
    CPX *workingBuf;

    CDownConvert modemDownConvert; //Get to modem rate and mix

    static const int bandPassLen = 512; //Review
    C_FIR_filter bandPass;
    static const int lowPassLen = 512;
    C_FIR_filter lowPass;

    int modemSampleRate;

    MatchedFilter *ms170;

};



#endif // WWVDIGITALMODEM_H
