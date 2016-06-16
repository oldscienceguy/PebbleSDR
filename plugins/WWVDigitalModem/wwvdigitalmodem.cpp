//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
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
    0-5ms  Reserved for 1khz second tick
    5ms - 30ms  No tone, guard band for second tick
    0ms -  800ms Reserved for 1000Hz minute pulse once per minute
    30 -  800ms Reserved for 100hz pulse width modulated bit on seconds 1 to 59
          -15db below carrier, then reduced another -15db at end of pulse
            30 - 200ms is data bit zero
            30 - 500ms is data bit one
            30 - 800ms is marker
    800 - 990ms    Reserved?
    990ms - 0ms No tone, pre-second guard band

    Between seconds one and sixteen inclusive past the minute, the current difference between UTC and UT1 is transmitted by doubling some of the once-per-second ticks, transmitting a second tick 100 ms after the first

Each minute encodes nine BCD digits for the time of century plus seven bits for the daylight savings time (DST) indicator, leap warning indicator and DUT1 correction

Collect 60bits a minute for N minutes, separate buffer for each minute
Analyze buffers to interpolate missing data if necessary

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

/*
 A matched filter is one where the frequency response is an exact match to the expected frequency spectrum. (Bores)
 A matched filter is in fact the same as correlating a signal with a copy of itself.

 Another definition
 A matched filter does a cross correlation using a time reversed template (does this mean NCO runs backwards?)

 The input (filter coefficients) to a matched filter is the reversed signal of interest
 For simple pulses, this consists of an NCO generated sine wave of the detected signal length
 The detected signal length therfore determines the filter length
 The output of the filter peaks when the desired signal and the detected signal are in sync
 This peak is detected as a slope change and indicates a detected 'bit'

 */
//This needs to work with any sampleRate, detectedSignal length and detectedSignal frequency
MatchedFilter::MatchedFilter(quint16 _sampleRate, quint16 _detectedSignalMs, quint32 _detectedSignalFrequency)
{
    sampleRate = _sampleRate;
    detectedSignalMs = _detectedSignalMs; //Length of matched signal
    detectedSignalFrequency = _detectedSignalFrequency;
    msPerSample = sampleRate / 1000;

    //8k sample rate, 170ms filter = 1360 sample
    lenFilter = msPerSample * detectedSignalMs; //#samples needed to get length
    delay = new CPX[lenFilter];
    outBuf = new double[lenFilter];
    delayPtr = 0;
    ncoPtr = 0;
    clearCpx(out);
    phase = 0;

    ncoPhase = 0;

    //Generate a fixed filter coefficient table that's in sync with detected frequency, sample rate, and symbol len
    ncoTable = new CPX[lenFilter+1]; //1 to 1 match with sample buffer with 1 extra zero element
    ncoIncrement = TWOPI * (double)detectedSignalFrequency / (double)sampleRate;
    for (int i=0; i<lenFilter; i++) {
        ncoTable[i].real(cos(ncoPhase));
        ncoTable[i].imag(sin(ncoPhase));
        ncoPhase += ncoIncrement;
        //sin (0) to sin(PI) is positive, zero at 0 and PI
        //sin (PI) to sin(TWOPI) is neg, zero at PI and TWOPI
        //When ncoPhase gets to PI we've generated 1/2 cycle at freq
        //When ncoPhase gets to TWOPI or greater we've completed a full cycle
        if (ncoPhase >= TWOPI)
            ncoPhase -= TWOPI;
        else if (ncoPhase < 0)
            ncoPhase += TWOPI;

    }

    slopesToAverage = 3; //Number of consistent slope up/down required to detect peak
    slopeDownCounter = 0;
    slopeUpCounter = 0;
    lastSlope = 0;
    lastMag = 0;
}

void MatchedFilter::ProcessSamples(quint16 len, CPX *in, double *output)
{
    for (int i=0; i<len; i++) {
        //Filter coefficient table (pulse waveform) is in sync with delay table, so we can use delayPtr for both
#if 1
		in[i].real(in[i].real() * ncoTable[delayPtr].real() / (msPerSample / 2 * detectedSignalMs));
		in[i].imag(in[i].imag() * ncoTable[delayPtr].im / (msPerSample / 2 * detectedSignalMs));
#else
        in[i] *= ncoTable[delayPtr];
#endif

        //out has the accumulated sum of all samples in the ring buffer * NCO
        //Throw out oldest value
        out -= delay[delayPtr];
        //And add newest
        out += in[i]; //Mixed value

		output[i] = magCpx(out);

        //Replace the oldest mixed sample in ring buffer with current and increment circular ptr for next call
        delay[delayPtr] = in[i];
        delayPtr = (delayPtr + 1) % lenFilter;

    }
}

//See wwv_rf in refclock_wwv.c
//Returns true if peak detected
bool MatchedFilter::ProcessSample(CPX in)
{
    //Filter coefficient table (pulse waveform) is in sync with delay table, so we can use delayPtr for both
	in.real(in.real() * ncoTable[delayPtr].real());// / (msPerSample / 2 * detectedSignalMs);
	in.imag(in.imag() * ncoTable[delayPtr].im);// / (msPerSample / 2 * detectedSignalMs);

#if 0
    //Next NCO value
    //Not convolution (mixing) so don't use CPX * operator
	in.real(in.real() * cos(ncoPhase)); // / (msPerSample / 2 * detectedSignalMs);
	in.imag(in.imag() * sin(ncoPhase)); // / (msPerSample / 2 * detectedSignalMs);

    ncoPhase += TWOPI * (double)detectedSignalFrequency / (double)sampleRate;
    //sin (0) to sin(PI) is positive, zero at 0 and PI
    //sin (PI) to sin(TWOPI) is neg, zero at PI and TWOPI
    //When ncoPhase gets to PI we've generated 1/2 cycle at freq
    //When ncoPhase gets to TWOPI or greater we've completed a full cycle
    if (ncoPhase >= TWOPI)
        ncoPhase -= TWOPI;
    else if (ncoPhase < 0)
        ncoPhase += TWOPI;
#endif
#if 0
    if (ncoPhase > M_PI)
        ncoPhase -= TWOPI;
    else if (ncoPhase < M_PI)
        ncoPhase += TWOPI;
#endif




    //out has the accumulated sum of all samples in the ring buffer * NCO
    //Throw out oldest value
    out -= delay[delayPtr];
    //And add newest
    out += in; //Mixed value

    //outBuf[delayPtr] = out;

    //Replace the oldest mixed sample in ring buffer with current and increment circular ptr for next call
    delay[delayPtr] = in;
    delayPtr = (delayPtr + 1) % lenFilter;

    //Slope detection
	double mag = magCpx(out);
    if (mag > lastMag) {
        //Slope up
        slopeUpCounter++;
        lastMag = mag;
        slopeDownCounter = 0;
        return false;
    } else if (mag < lastMag) {
        //Slope down
        slopeDownCounter++;
        if (slopeUpCounter >= slopesToAverage) {
            //Possible peak
            if (slopeDownCounter >= slopesToAverage) {
                //Consistent slope up and down, this is peak
                slopeUpCounter = slopeDownCounter = 0; //Start looking for next peak
                return true; //Peak is slopesToAverage samples prior
            } else {
                //Keep looking for more down slopes
                return false;
            }
        } else {
            //Not enough up slopes
            slopeUpCounter = slopeDownCounter = 0; //Start looking for next peak
            return false;
        }
    } else {
        //Flat, reset?
        slopeUpCounter = slopeDownCounter = 0; //Start looking for next peak
        return false;
    }

#if 0
    double dtemp;
    //i = up->datapt;
    quint16 prevNcoPtr = ncoPtr; //i

    //NCO for the In phase frequency
    //Increment is # of NCO samples to output desired NCO frequency
    //up->datapt = (up->datapt + IN100) % 80;
    ncoPtr = (ncoPtr + (detectedSignalFrequency * NUM_SINES / sampleRate)) % NUM_SINES;

    //dtemp = sintab[i] * data / (MS / 2. * DATCYC);
    dtemp = sintab[prevNcoPtr] * in.real() / (msPerSample / 2.0 * msDetectedSignal);

    //up->irig -= ibuf[iptr];
	out.real(out.real() - delay[delayPtr].re);
    //up->irig += dtemp;
	out.real(out.real() + dtemp);

    //ibuf[iptr] = dtemp;
    delay[delayPtr].real(dtemp);


    //NCO for the Quadrature (out of phase) frequency is 90deg or 20 4.5deg samples offset from In phase
    //i = (i + 20) % 80;
    prevNcoPtr = (prevNcoPtr + phaseShiftSamples) % NUM_SINES;

    //dtemp = sintab[i] * data / (MS / 2. * DATCYC);
    dtemp = sintab[prevNcoPtr] * in.im / (msPerSample / 2 * msDetectedSignal);

    //up->qrig -= qbuf[iptr];
    out.im -= delay[delayPtr].im;
    //up->qrig += dtemp;
	out.imag(out.imag() += dtemp);

    //qbuf[iptr] = dtemp;
    delay[delayPtr].imag(dtemp);


    //iptr = (iptr + 1) % DATSIZ;
    delayPtr = (delayPtr + 1) % lenFilter;
#endif

}

WWVDigitalModem::WWVDigitalModem() : QObject()
{
    dataUi = NULL;
    out = NULL;
    workingBuf = NULL;
    modemClock = 0;
}

void WWVDigitalModem::setSampleRate(int _sampleRate, int _sampleCount)
{
    sampleRate = _sampleRate;
    numSamples = _sampleCount;
    int actualModemRate = modemDownConvert.SetDataRate(sampleRate,1000);
    modemSampleRate = actualModemRate;

    //Inital filter to get 1k or 1.2k carrier
    //Filter freq has to be normalized against sample rate
    double fHi, fLo;
    fHi = 1400.0/modemSampleRate;
    fLo = 800.0/modemSampleRate;
    bandPass.init_bandpass(bandPassLen,1,fLo,fHi); //600hz wide
    //100hz sub carrier
    fLo = 150.0/modemSampleRate;
    //lowPass.init_lowpass(lowPassLen,1,fLo); //FLDigi version doesn't work as well

    //How does Q affect filter
    lowPass.InitLP(150,1.0,modemSampleRate);

    out = new CPX[numSamples];
    workingBuf = new CPX[numSamples];
    ms170Buf = new double[numSamples];

    ms170 = new MatchedFilter(modemSampleRate, 170, 100);
    ms5 = new MatchedFilter(modemSampleRate, 5, 1000);
    ms800 = new MatchedFilter(modemSampleRate, 800, 1000);
}

//Implement pure virtual destructor from interface, otherwise we don't link
DigitalModemInterface::~DigitalModemInterface()
{

}

WWVDigitalModem::~WWVDigitalModem()
{
}

void WWVDigitalModem::setDemodMode(DeviceInterface::DemodMode _demodMode)
{
	Q_UNUSED(_demodMode);
}

CPX *WWVDigitalModem::processBlock(CPX *in)
{
    if (workingBuf == NULL)
        return in; //Not initialized yet

    //Downconverter first mixes in place, ie changes in!  So we have to work with a copy
    copyCPX(workingBuf,in,numSamples);

    //Downconvert and mix
    modemDownConvert.SetFrequency(0); //No additional mixing
    int numModemSamples = modemDownConvert.ProcessData(numSamples, workingBuf, out);

    //emit Testbench(numModemSamples, out, modemSampleRate, WWVModem);

    lowPass.ProcessFilter(numModemSamples,out,workingBuf);
    //emit Testbench(numModemSamples, workingBuf, modemSampleRate, WWVModem);

    ms170->ProcessSamples(numModemSamples,workingBuf, ms170Buf);
    emit testbench(numModemSamples, ms170Buf, modemSampleRate, WWVModem);

#if 0

    for (int i = 0; i < numModemSamples; i++) {

        modemClock++;
        Process100Hz(workingBuf[i]);

        //Add gain here if needed

        //The 1000/1200-Hz pulses and 100-Hz subcarrier are first separated using
        //a 600-Hz bandpass filter centered on 1100 Hz and a 150-Hz lowpass filter
        //This attenuates the 1000/1200-Hz sync signals, as well as the 440-Hz and 600-Hz tones
        //and most of the noise and voice modulation components.

        //The subcarrier is transmitted 10 dB down from the carrier. The DGAIN parameter can be adjusted for this
        //and to compensate for the radio audio response at 100 Hz.
        if (lowPass.run(out[i],lowPassOut)) {
            //LowPass is valid
            emit Testbench(1, &lowPassOut, modemSampleRate, WWVModem);

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
#endif
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

    //100Hz is transmitted 10db below carrier, adjust gain. 10db is a factor of 10 gain (see db conversion)
    if (ms170->ProcessSample(in * 10)) {
        qDebug()<<modemClock; //Peak
    }

}

void WWVDigitalModem::Process1000Hz(CPX in)
{
	Q_UNUSED(in);
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

void WWVDigitalModem::setupDataUi(QWidget *parent)
{
    if (parent == NULL) {
        //We want to delete
        if (dataUi != NULL) {
            emit removeProfile(WWVModem);
            delete dataUi;
        }
        dataUi = NULL;
        return;
    } else if (dataUi == NULL) {
        //Create new one
        dataUi = new Ui::dataWWV();
        dataUi->setupUi(parent);
        //Register testbench
        emit addProfile("WWV Modem", WWVModem);

    }
}

QString WWVDigitalModem::getPluginName()
{
    return "WWV";
}

QString WWVDigitalModem::getPluginDescription()
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

#if 0
Formatted output
/*
 * timecode - assemble timecode string and length
 *
 * Prettytime format - similar to Spectracom
 *
 * sq yy ddd hh:mm:ss ld dut lset agc iden sig errs freq avgt
 *
 * s	sync indicator ('?' or ' ')
 * q	error bits (hex 0-F)
 * yyyy	year of century
 * ddd	day of year
 * hh	hour of day
 * mm	minute of hour
 * ss	second of minute)
 * l	leap second warning (' ' or 'L')
 * d	DST state ('S', 'D', 'I', or 'O')
 * dut	DUT sign and magnitude (0.1 s)
 * lset	minutes since last clock update
 * agc	audio gain (0-255)
 * iden	reference identifier (station and frequency)
 * sig	signal quality (0-100)
 * errs	bit errors in last minute
 * freq	frequency offset (PPM)
 * avgt	averaging time (s)
 */
static int
timecode(
    struct wwvunit *up,	/* driver structure pointer */
    char *ptr		/* target string */
    )
{
    struct sync *sp;
    int year, day, hour, minute, second, dut;
    char synchar, leapchar, dst;
    char cptr[50];


    /*
     * Common fixed-format fields
     */
    synchar = (up->status & INSYNC) ? ' ' : '?';
    year = up->decvec[YR].digit + up->decvec[YR + 1].digit * 10 +
        2000;
    day = up->decvec[DA].digit + up->decvec[DA + 1].digit * 10 +
        up->decvec[DA + 2].digit * 100;
    hour = up->decvec[HR].digit + up->decvec[HR + 1].digit * 10;
    minute = up->decvec[MN].digit + up->decvec[MN + 1].digit * 10;
    second = 0;
    leapchar = (up->misc & SECWAR) ? 'L' : ' ';
    dst = dstcod[(up->misc >> 4) & 0x3];
    dut = up->misc & 0x7;
    if (!(up->misc & DUTS))
        dut = -dut;
    sprintf(ptr, "%c%1X", synchar, up->alarm);
    sprintf(cptr, " %4d %03d %02d:%02d:%02d %c%c %+d",
        year, day, hour, minute, second, leapchar, dst, dut);
    strcat(ptr, cptr);

    /*
     * Specific variable-format fields
     */
    sp = up->sptr;
    sprintf(cptr, " %d %d %s %.0f %d %.1f %d", up->watch,
        up->mitig[up->dchan].gain, sp->refid, sp->metric,
        up->errcnt, up->freq / SECOND * 1e6, up->avgint);
    strcat(ptr, cptr);
    return (strlen(ptr));
}


#endif
