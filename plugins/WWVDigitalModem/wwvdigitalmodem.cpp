#include "wwvdigitalmodem.h"
#include "gpl.h"
#include <QDebug>

/*
  References
  http://www.ntp.org/downloads.html see wwv.c refclock_wwv.c and doc for in depth decoding.  From ICOM?
  Uses wwv wwvh chu as refernce clocks for NTP server

The WWV signal format consists of three elements
1. 5-ms, 1000-Hz pulse, which occurs at the beginning of each second
2. 800-ms, 1000-Hz pulse, which occurs at the beginning of each minute
3. Pulse-width modulated 100-Hz subcarrier for the data bits, one bit per second

Each minute encodes nine BCD digits for the time of century plus seven bits for the daylight savings time (DST) indicator, leap warning indicator and DUT1 correction


WWV BCD time code
Bit     Weight	Meaning
:00             No 100 Hz (minute mark)
:01     0       Unused, always 0.
:02     DST1	DST in effect at 00:00Z today
:03     LSW     Leap second at end of month

:04     1       BCD Units digit of year
:05     2       BCD Units digit of year
:06     4       BCD Units digit of year
:07     8   	BCD Units digit of year

:08     0       Unused, always 0
:09     P1      Marker	M

:10     1       BCD Minutes
:11     2       BCD Minutes
:12     4       BCD Minutes
:13     8       BCD Minutes
:14     0       BCD Minutes
:15     10      BCD Minutes
:16     20      BCD Minutes
:17     40      BCD Minutes

:18     0       Unused, always 0
:19     P2      Marker

:20     1       BCD Hours
:21     2       BCD Hours
:22     4       BCD Hours
:23     8       BCD Hours
:24     0       BCD Hours
:25     10      BCD Hours
:26     20      BCD Hours

:27     0       Unused, always 0
:28     0       Unused, always 0
:29     P3      Marker	M

:30     1       BCD Day of year 1=January 1st, 32=February 1st, etc
:31     2       BCD Day of year 1=January 1st, 32=February 1st, etc
:32     4       BCD Day of year 1=January 1st, 32=February 1st, etc
:33     8       BCD Day of year 1=January 1st, 32=February 1st, etc
:34     0       BCD Day of year 1=January 1st, 32=February 1st, etc
:35     10      BCD Day of year 1=January 1st, 32=February 1st, etc
:36     20      BCD Day of year 1=January 1st, 32=February 1st, etc
:37     40      BCD Day of year 1=January 1st, 32=February 1st, etc
:38     80      BCD Day of year 1=January 1st, 32=February 1st, etc

:39     P4      Marker

:40     100     Day of year (cont.)	0
:41     200     0

:42     0       Unused, always 0
:43     0       Unused, always 0
:44     0       Unused, always 0
:45     0       Unused, always 0
:46     0       Unused, always 0
:47     0       Unused, always 0
:48     0       Unused, always 0

:49     P5      Marker	M

:50     +       DUT1 sign (1=positive)

:51     10      BCD Tens digit of year
:52     20      BCD Tens digit of year
:53     40      BCD Tens digit of year
:54     80      BCD Tens digit of year

:55     DST2	DST in effect at 24:00Z today

:56     0.1     DUT1 magnitude (0 to 0.7 s).DUT1 = UT1−UTC
:57     0.2     DUT1 magnitude (0 to 0.7 s).DUT1 = UT1−UTC
:58     0.4     DUT1 magnitude (0 to 0.7 s).DUT1 = UT1−UTC

:59     P0      Marker	M

*/

//This needs to work with any sampleRate, detectedSignal length and detectedSignal frequency
MatchedFilter::MatchedFilter(quint16 _sampleRate, quint16 _detectedSignalMs, quint32 _detectedSignalFrequency)
{
    sampleRate = _sampleRate;
    msDetectedSignal = _detectedSignalMs; //Length of matched signal
    detectedSignalFrequency = _detectedSignalFrequency;
    msPerSample = sampleRate / 1000;
    phaseShiftSamples = 90 / (360/NUM_SINES); //20 samples for 80 sine table

    //8k sample rate, 170ms filter = 1360 sample
    lenFilter = msPerSample * msDetectedSignal; //#samples needed to get length
    delay = new CPX[lenFilter];
    delayPtr = 0;
    ncoPtr = 0;
    out.clear();
    phase = 0;

    ncoPhase = 0;
    //Init sintab, compare with sintab in refclock_wwv
    double radInc = TWOPI / NUM_SINES; //radian delta, there are TWOPI radians in a circle
    //We actually need NUM_SINES +1 values in table so we wrap correctly
    for (int i=0; i<NUM_SINES; i++) {
        sintab[i] = sin(i * radInc); //sin() takes radians, not degrees
    }
    sintab[NUM_SINES] = 0;
}

//See wwv_rf in refclock_wwv.c
void MatchedFilter::ProcessSample(CPX in)
{

    //Next NCO value
    //Not a complex mix, ....  simple multiplcation
    in.re *= cos(ncoPhase);
    in.im *= sin(ncoPhase);

    ncoPhase += TWOPI * (double)detectedSignalFrequency / (double)sampleRate;
    if (ncoPhase > M_PI)
        ncoPhase -= TWOPI;
    else if (ncoPhase < M_PI)
        ncoPhase += TWOPI;

    //out has the accumulated mxed values for all samples in the ring buffer
    //Throw out oldest value
    out -= delay[delayPtr];
    //And add newest
    out += in; //Mixed value

    //Add current mixed sample to ring buffer and increment circular ptr for next call
    delay[delayPtr] = in;
    delayPtr = (delayPtr + 1) % lenFilter;

#if 0
    double dtemp;
    //i = up->datapt;
    quint16 prevNcoPtr = ncoPtr; //i

    //NCO for the In phase frequency
    //Increment is # of NCO samples to output desired NCO frequency
    //up->datapt = (up->datapt + IN100) % 80;
    ncoPtr = (ncoPtr + (detectedSignalFrequency * NUM_SINES / sampleRate)) % NUM_SINES;

    //dtemp = sintab[i] * data / (MS / 2. * DATCYC);
    dtemp = sintab[prevNcoPtr] * in.re / (msPerSample / 2.0 * msDetectedSignal);

    //up->irig -= ibuf[iptr];
    out.re -= delay[delayPtr].re;
    //up->irig += dtemp;
    out.re += dtemp;

    //ibuf[iptr] = dtemp;
    delay[delayPtr].re = dtemp;


    //NCO for the Quadrature (out of phase) frequency is 90deg or 20 4.5deg samples offset from In phase
    //i = (i + 20) % 80;
    prevNcoPtr = (prevNcoPtr + phaseShiftSamples) % NUM_SINES;

    //dtemp = sintab[i] * data / (MS / 2. * DATCYC);
    dtemp = sintab[prevNcoPtr] * in.im / (msPerSample / 2 * msDetectedSignal);

    //up->qrig -= qbuf[iptr];
    out.im -= delay[delayPtr].im;
    //up->qrig += dtemp;
    out.im += dtemp;

    //qbuf[iptr] = dtemp;
    delay[delayPtr].im = dtemp;


    //iptr = (iptr + 1) % DATSIZ;
    delayPtr = (delayPtr + 1) % lenFilter;
#endif

}

WWVDigitalModem::WWVDigitalModem()
{
    dataUi = NULL;
    out = NULL;
    workingBuf = NULL;
}

void WWVDigitalModem::SetSampleRate(int _sampleRate, int _sampleCount)
{
    sampleRate = _sampleRate;
    numSamples = _sampleCount;
    int actualModemRate = modemDownConvert.SetDataRate(sampleRate,1000);
    modemSampleRate = actualModemRate;

    //Inital filter to get 1k or 1.2k carrier
    bandPass.init_bandpass(bandPassLen,1,800,1400); //600hz wide
    //100hz sub carrier
    lowPass.init_lowpass(lowPassLen,1,150);

    out = new CPX[numSamples];
    workingBuf = new CPX[numSamples];

    ms170 = new MatchedFilter(modemSampleRate, 170, 100);
}

WWVDigitalModem::~WWVDigitalModem()
{
}

void WWVDigitalModem::SetDemodMode(DEMODMODE _demodMode)
{
}

CPX *WWVDigitalModem::ProcessBlock(CPX *in)
{
    CPX bandPassOut;
    CPX lowPassOut;
    //Downconverter first mixes in place, ie changes in!  So we have to work with a copy
    CPXBuf::copy(workingBuf,in,numSamples);

    //Downconvert and mix
    modemDownConvert.SetFrequency(0); //No additional mixing
    int numModemSamples = modemDownConvert.ProcessData(numSamples, workingBuf, out);

    for (int i = 0; i < numModemSamples; i++) {
        //Add gain here if needed

        //The 1000/1200-Hz pulses and 100-Hz subcarrier are first separated using
        //a 600-Hz bandpass filter centered on 1100 Hz and a 150-Hz lowpass filter
        //This attenuates the 1000/1200-Hz sync signals, as well as the 440-Hz and 600-Hz tones
        //and most of the noise and voice modulation components.

        //The subcarrier is transmitted 10 dB down from the carrier. The DGAIN parameter can be adjusted for this
        //and to compensate for the radio audio response at 100 Hz.
        if (lowPass.run(bandPassOut,lowPassOut)) {
            //LowPass is valid
            Process100Hz(lowPassOut);
        }
        if (bandPass.run(out[i],bandPassOut)) {
            //BandPass is stable and we have result
                Process1000Hz(bandPassOut);
        }

        //The minute synch pulse is extracted using an 800-ms synchronous matched filter and pulse grooming
        //logic which discriminates between WWV and WWVH signals and noise

        //The second synch pulse is extracted using a 5-ms FIR matched filter and 8000-stage comb filter.
    }
    return in;
}

//Extracts 100hz data from signal
void WWVDigitalModem::Process100Hz(CPX in)
{
    /*
     * The 100-Hz data signal is demodulated using a pair of
     * quadrature multipliers, matched filters and a phase lock
     * loop. The I and Q quadrature data signals are produced by
     * multiplying the filtered signal by 100-Hz sine and cosine
     * signals, respectively. The signals are processed by 170-ms
     * synchronous matched filters to produce the amplitude and
     * phase signals used by the demodulator. The signals are scaled
     * to produce unit energy at the maximum value.
     */

    ms170->ProcessSample(in);
    qDebug()<<ms170->mag();

}

void WWVDigitalModem::Process1000Hz(CPX in)
{
    /*
     * Baseband sync demodulation. The 1000/1200 sync signals are
     * extracted using a 600-Hz IIR bandpass filter. This removes
     * the 100-Hz data subcarrier, as well as the 440-Hz and 600-Hz
     * tones and most of the noise and voice modulation components.
     *
     * Matlab 4th-order IIR elliptic, 800-1400 Hz bandpass, 0.2 dB
     * passband ripple, -50 dB stopband ripple, phase delay 0.91 ms.
     */

}

void WWVDigitalModem::SetupDataUi(QWidget *parent)
{
    if (parent == NULL) {
        //We want to delete
        if (dataUi != NULL) {
            delete dataUi;
        }
        dataUi = NULL;
        return;
    } else if (dataUi == NULL) {
        //Create new one
        dataUi = new Ui::dataWWV();
        dataUi->setupUi(parent);
    }
}

QString WWVDigitalModem::GetPluginName()
{
    return "WWV";
}

QString WWVDigitalModem::GetDescription()
{
    return "WWV Digital Modem";
}

/*
 State Machine
 - Initial lock on 800ms 1k (1.2kWWVH) minute signal
 - Lock on 5ms 1k second signal
 - Collect 9 digits
 - When 3 sets of 9 digits have been collected and match, time is sync'd (can take 15-40 min in noisy conditions)

*/
