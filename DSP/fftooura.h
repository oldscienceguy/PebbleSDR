#ifndef FFTOOURA_H
#define FFTOOURA_H

class FFTOoura
{
public:
    FFTOoura();
    void cdft(int n, int isgn, double *a, int *ip, double *w);
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void ddct(int n, int isgn, double *a, int *ip, double *w);
    void ddst(int n, int isgn, double *a, int *ip, double *w);
    void dfct(int n, double *a, double *t, int *ip, double *w);
    void dfst(int n, double *a, double *t, int *ip, double *w);

};

#endif // FFTOOURA_H
