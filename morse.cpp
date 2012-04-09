#include "morse.h"
#include "QPainter"
#include "QDebug"
#include "QEvent"

/*
  CW Notes for reference
  Tcw is basic unit of measurement in ms
  Dash = 3 Tcw, Dot = 1Tcw, Element Space = 1 Tcw, Character Space = 3 Tcw, Word Space = 7 Tcw (min)
  Standard for measuring WPM is to use the word "PARIS", which is exactly 50Tcw
  So Tcw (ms) = 60 (sec) / (WPM * 50)
  and WPM = 1200/Tcw(ms)
  Tcw for 12wpm = 60 / (12 * 50) = 60 / 600 = .100 sec  100ms
  Tcw for 30wpm = 60 / 1500 = .040 40ms
  Tcw for 120wpm = 60 / (120 * 50) = 60 / 6000 = .01 sec  10ms

  The 'binWidth' in the Goertzel algorithm determines how many samples should be processed before returning a result.
  The smaller the binWidth, the more samples we need to process.  See goertzel.cpp for detail
  There is a potential trade off between binWidth and Tcw resolution
  We need to make sure the 'timePerBin' (#samples per bin * timePerSample) syncs up with Tcw requirements

  Each Goertzel 'bin' should be at least Tcw * 0.25, so we get at least 4 bins per Tcw
  15wpm example
  Sampling at 8khz, each sample is 1/8000 = .000125sec
  for 20ms bin (25% of 80ms) we need to accumulate 160 samples, which also gives us a binWidth of 50hz, ie too sharp

  30wpm example
  for 10ms bin (25% of 40ms) we need to accumulate 80 samples, which also gives us a binWidth of 100hz, ie very sharp

  60wpm example
  for 5ms bin (25% of 20ms) we need to accumulate 40 samples, which also gives us a binWidth of 200hz, ie not as sharp
  Therefore, as WPM goes up, goertzel bandwidth (binWidth) goes up
  If we set for 60wpm, we can also handle 15 and 30 by accumulating 16 and 8 results instead of 4;

  A 5ms bin will handle any WPM up to 60, we just need to keep track of how many bins we need per Tcw
  For any binWidth, the maximum WPM is calculated as
  maxWpm = 1200 / (timePerBin * 4)

  If we tried to use Goertzel directly on a 96k sample rate, we would need to process 9600 samples to get a 100ms block!!!
  So to make Goertzel practical, we will need to decimate to a lower rate, like 8k

*/

Morse::Morse(int sr, int fc) : SignalProcessing(sr,fc)
{
    //Setup Goertzel
    cwGoertzel = new Goertzel();
    //Process Goertzel at 8k sample rate
    goertzelSampleRate = 8000;
    goertzelFreq = 700;

    //Start with fixed WPM for now
    fixedWPM = true;
    maxWPM = 60;
    wpm = 30;
    //binwidth for maxWPM = 60 is 200hz
    cwGoertzel->SetFreqHz(goertzelFreq,goertzelSampleRate,200);

    //Number of goertzel results we need for a reliable tcw
    numResultsPerTcw = WpmToTcw(wpm) / cwGoertzel->timePerBin;

    decimateFactor = sampleRate / goertzelSampleRate;
    powerBufSize = numSamples / decimateFactor;
    toneBufSize = numResultsPerTcw * 2; //2x larger than needed
    powerBuf = new float[powerBufSize];
    toneBuf = new bool[toneBufSize];
    toneBufCounter = 0;

    outString ="";

}

Morse::~Morse()
{
    if (cwGoertzel != NULL) delete cwGoertzel;
    if (powerBuf != NULL) free (powerBuf);
    if (toneBuf != NULL) free (toneBuf);

}

//Returns tcw in ms for any given WPM
int Morse::WpmToTcw(int w)
{
    int tcw = 60.0 / (w * 50) * 1000;
    return tcw;
}

void Morse::ConnectToUI(QFrame *meter, QTextEdit *edit)
{
    uiMeter = meter;
    uiTextEdit = edit;
    //We want to handle all events for this UI object, like paint
    meter->installEventFilter(this);

}

//return true to 'eat' event
bool Morse::eventFilter(QObject *o, QEvent *e)
{
    if (o == uiMeter) {
        QFrame *m = (QFrame *)o;
        if (e->type() == QEvent::Paint) {
            QPainter painter(m);
            //rect() just returns 0,0,width,height with no offsets or transforms
            QRect pa = m->rect();

            if (outTone) {
                //painter.setPen(Qt::blue);
                //painter.drawText(0,0,"M");
                painter.fillRect(pa, Qt::blue);
                uiTextEdit->insertPlainText(outString);
            }
            else {
                //painter.setPen(Qt::red);
                //painter.drawText(pa, "S");
                painter.fillRect(pa, Qt::white);
                uiTextEdit->insertPlainText(outString);
            }
            return false;
        }

    }
    return false;
}

CPX * Morse::ProcessBlock(CPX * in)
{
    bool res;
    float power,sample;
    int markCount,spaceCount;

    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
    for (int i=0, j=0; i<numSamples; i=i+decimateFactor, j++) {
        sample = in[i].re;
        res = cwGoertzel->FPNextSample(sample);
        if (res) {
            //We have accumulated enough samples to get output of Goertzel
            //Store last N tones
            if (toneBufCounter < toneBufSize) {
                toneBuf[toneBufCounter] = cwGoertzel->binaryOutput;
                toneBufCounter++;
            }

            if (toneBufCounter > numResultsPerTcw) {
                //We need at least numResultsPerTcw results
                //if (toneBuf[0] && toneBuf[1] && toneBuf[2] && toneBuf[3]) {
                markCount = spaceCount = 0;
                for (int i=0; i<numResultsPerTcw; i++) {
                    if (toneBuf[i])
                        markCount++;
                    else
                        spaceCount++;
                }
                if (markCount == numResultsPerTcw) {
                    //All results are mark
                    //1 Tcw Is it dash or dot
                    outString = "M";
                    outTone = true;
                    toneBufCounter = 0;
                    uiMeter->repaint();  //Triggers an immediate repaint
                } else if (spaceCount == numResultsPerTcw) {
                    //All results are space
                    //1 Tcw, is it letter or word space
                    outString = "S";
                    outTone = false;
                    toneBufCounter = 0;
                    uiMeter->repaint();  //Triggers an immediate repaint
                } else {
                    //Not all the same, shift and try again
                    for (int i=0; i<numResultsPerTcw - 1; i++) {
                        toneBuf[i] = toneBuf[i+1];
                    }
                    toneBufCounter --;
                }
            }

            //power = cwGoertzel->GetNextPowerResult();
            //powerBuf[j] = power;
            //qDebug("G sample / power = %.12f / %.12f",sample,power);
            //qDebug("G MS / average / power = %c / %.12f / %.12f",cwGoertzel->binaryOutput?'M':'S', cwGoertzel->avgPower, power);

            //Do we have a new element?
        }
    }

    return in;
}

struct morseChar {
    quint16 binaryValue; //'0'=did '1'=dash
    unsigned char letter;
    const char* elements;
};

morseChar charLookup[] = {
    {0x00,'E',"."},
    {0x01,'T',"-"},
};

/*
  For historic purposes: Some source code fragments from my SuperRatt AppleII CW code circa 1983

*TABLE OF CW CODES IN ASCII ORDER FROM $20 TO $3F
*1ST HIGH BIT IN # SIGNALS START OF CODE
*0=DIT 1=DAH
*
CWTBL    HEX   00C5002A45253600 ;SP,!,",#,$,%,&,'
         HEX   6D6D152873315532 ;(,),*,+,{,},-,.,/
         HEX   3F2F272321203038 ;0,1,2,3,4,5,6,7
         HEX   3C3E786A0000004C ;8,9,:,;,<,=,>,?
         HEX   0005181A0C02120E ;@,A,B,C,D,E,F,G
         HEX   1004170D1407060F ;H,I,J,K,L,M,N,O
         HEX   161D0A080309110B ;P,Q,R,S,T,U,V,W
         HEX   191B1C6D326D0000 ;X,Y,Z,{,\,},^,NA
*(#)=END OF MESSAGE ($)=END OF QSO (%)=END OF WORK (&)=GO AHEAD
*(*)=END OF LINE (+)=WAIT (!)=BREAK

* CW BYTE IN
* ENTRY:
* EXIT:
         AST   80
*
*
CBYTEIN  LDA   CWFLG      ;WAS LAST CHAR A WORD?
         BEQ   CWIN0      ;NO-CONTINUE
         LDA   #$A0       ;YES-PRINT SPACE
         DEC   CWFLG      ;RESET FLAG
         RTS
*
CWIN0    LDA   #1         ;INIT CHAR LOC
         STA   CWCHAR
         LDA   CWDITLEN   ;GET DITLEN IN MS
         NOP              ;IN CASE LSR NEEDED LATER
         LSR
         STA   LOC0       ;DELAY=1/20TH DIT LEN
         JMP   CWIN2      ;START
*
*ENTER HERE ONLY IF KEYPRESSED
CWIN1    CMP   #$99       ;CTRL Y?
         BNE   CWIN1A     ;NO
         LDA   #1
         STA   CWCHAR
         LDA   #40        ;1 LOOP=1/20TH DIT, SO 40 LOOPS=2 DITS
         STA   CWDIT
         LDA   #0
         STA   KEYLOC
         JMP   CWIN0
*
CWIN1A   RTS
*
CWIN2    JSR   CBITIN     ;KEY UP OR DOWN?
         BCC   KEYDOWN
*
KEYUP    LDA   #0         ;INIT COUNTER
         STA   CWUP
KEYUP1   INC   CWUP       ;ADD TIME TO COUNT
         BNE   KEYUP2
         LDA   CWCHAR     ;KEY UP TOO LONG
         CMP   #1         ;ANY CHAR TO DECODE
         BEQ   CWIN2      ;NO-CONTINUE
         INC   CWFLG      ;YES SET WORD FLAG
         JMP   CWIN6      ;AND GO TRY DECODE
*
KEYUP2   JSR   DELAY
         JSR   KEYTEST
         LDA   KEYLOC
         BEQ   KEYUP2A
         JMP   CWIN1      ;GO PROCESS CTRL KEY
KEYUP2A  JSR   CBITIN     ;STILL UP?
         BCS   KEYUP1     ;YES
*KEY WENT DOWN
* SEE IF UPTIME < DIT AVERAGE (BIT INTERVAL)
         LDA   CWUP       ;GET KEY UP TIME
         CMP   CWDIT      ;>BIT INTERVAL?
         BCC   CWIN2      ;NO,GET MORE BITS
* IF > 4*CWDIT THEN WORD BREAK
         LSR
         LSR
         CMP   CWDIT
         BCC   KEYUP3     ;LETTER
         INC   CWFLG      ;YES-SET WORD FLAG
KEYUP3   JMP   CWIN6      ;DECODE CHARACTER
*
* KEY DOWN TIMING ROUTINE
*
KEYDOWN  LDA   #0         ;INIT COUNTERS
         STA   CWDOWN
*
KEYDOWN1 INC   CWDOWN
         BNE   KEYDOWN2
         DEC   CWDOWN     ;KEEP AT $FF UNTILL KEY UP
*
KEYDOWN2 JSR   DELAY
         JSR   KEYTEST
         LDA   KEYLOC
         BEQ   KD2B
         JMP   CWIN1
KD2B     JSR   CBITIN     ;DOWN?
         BCC   KEYDOWN1   ;YES
*
* KEY WENT UP
*DIT OR DASH?
*
         LDA   CWDOWN     ;GET DOWN TIME
         CMP   #15        ;1DIT AT STD SPEED=20LOOPS
         BCS   DOT:DASH   ;NO
         JMP   CWIN2      ;YES-START OVER
DOT:DASH CMP   CWDIT      ;> AVERAGE DIT?
         BCS   CWDASH     ;YES-DASH
*
*RECEIVED DIT
*
         ASL   CWCHAR     ;ROTATE 0 INTO CHARACTER
         JMP   NEWAVG
*
*RECEIVED DASH
*
CWDASH   ROL   CWCHAR     ;PUT 1 INTO CHARACTER
         TAY
         JSR   $E301      ;FLOAT Y
         JSR   $EB63      ;MOVE FAC INTO ARG
         LDY   #3
         JSR   $E301
         JSR   $EA69      ;DIVIDE ARG BY FAC
         JSR   $E6FB      ;AND PUT IN X
         TXA
*
NEWAVG   ASL              ;DIT TIME *2
         BCC   NEWAVG3    ;NOT > 255
         LDX   #$AA       ;TOO MUCH
         JMP   CWIN8
*
NEWAVG3  STA   CWDOWN     ;STORE IT
         LDY   CWDIT      ;GET LAST AVG
         JSR   $E301
         JSR   $EB63      ;MOVE TO ARG
         LDY   #5
         JSR   $E301
         JSR   $E982      ;MULTIPLY
         JSR   $EB63      ;MOVE RESULT TO ARG
         LDY   CWDOWN
         JSR   $E301      ;FLOAT IT
         JSR   $E7C1      ;ADD TO MULT
         JSR   $EB63      ;MOVE TO FAC
         LDY   #6         ;DIVIDE BY 6
         JSR   $E301
         JSR   $EA69      ;DIVIDE
         JSR   $E6FB      ;RESULT IN X
         STX   CWDIT
NEWAVG2  JMP   CWIN2      ;CONTINUE
*
* DECODE CW CHARACTER
*
CWIN6    LDX   #$60       ;#ASCII CHARACTERS
         LDA   CWCHAR
CWIN7    CMP   CWTBL-$20,X ;MATCH?
         BEQ   CWIN8      ;YEP!
         DEX
         BNE   CWIN7      ;CONTINUE SEARCH
         LDX   #$AA       ;LOAD * FOR NO MATCH
CWIN8    TXA              ;PUT CHAR IN ACCUM
         JMP   BYT3       ;EXIT THROUGH BYTEIN
*
*
         AST   80

*/
