#ifndef FFTOOURA_H
#define FFTOOURA_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "fft.h"

class PEBBLELIBSHARED_EXPORT FFTOoura : public FFT
{
public:
    FFTOoura();
    ~FFTOoura();

	void FFTParams(quint32 _size, double _dBCompensation, double sampleRate);
    void FFTForward(CPX * in, CPX * out, int size);
    void FFTInverse(CPX * in, CPX * out, int size);
    void FFTSpectrum(CPX *in, double *out, int size);

protected:
    //Complex DFT isgn indicates forward or inverse
    void cdft(int n, int isgn, double *a, int *ip, double *w);

    //Other versions not used in Pebble
    void rdft(int n, int isgn, double *a, int *ip, double *w); //Real DFT
    void ddct(int n, int isgn, double *a, int *ip, double *w); //Discrete Cosine Transform
    void ddst(int n, int isgn, double *a, int *ip, double *w); //Discrete Sine Transform
    void dfct(int n, double *a, double *t, int *ip, double *w);  //Real Symmetric DFT
    void dfst(int n, double *a, double *t, int *ip, double *w); //Real Anti-symmetric DFT

private:
    double *offtSinCosTable;
    int *offtWorkArea;

    //Original ooura worker functions converted to member functions
    //Never call these except via public or protected member functions above
    void makewt(int nw, int *ip, double *w);
    void makect(int nc, int *ip, double *c);
    void bitrv2(int n, int *ip, double *a);
    void bitrv2conj(int n, int *ip, double *a);
    void cftfsub(int n, double *a, double *w);
    void cftbsub(int n, double *a, double *w);
    void cft1st(int n, double *a, double *w);
    void cftmdl(int n, int l, double *a, double *w);
    void rftfsub(int n, double *a, int nc, double *c);
    void rftbsub(int n, double *a, int nc, double *c);
    void dctsub(int n, double *a, int nc, double *c);
    void dstsub(int n, double *a, int nc, double *c);

};

#endif // FFTOOURA_H
