//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "noiseblanker.h"

NoiseBlanker::NoiseBlanker(quint32 _sampleRate, quint32 _bufferSize):ProcessStep(_sampleRate,_bufferSize)
{
	nbEnabled = false;
	nb2Enabled = false;
	nbDelay = new DelayLine(8,2); //2 sample delay line
	nbAverageMag = 1;
	nb2AverageMag = 1;
	nbSpike = 7;
	nbSpikeCount =0;
	nbThreshold = 3.3;

}

NoiseBlanker::~NoiseBlanker(void)
{
}
void NoiseBlanker::setNbEnabled(bool b)
{
	if (b) {
		nbSpikeCount = 0;
		nbAverageMag = 0;
		clearCpx(nbAverageCPX);
	}
	nbEnabled = b;
}
void NoiseBlanker::setNb2Enabled(bool b)
{
	if (b) {
		nb2AverageMag = 0;
		clearCpx(nb2AverageCPX);
	}
	nb2Enabled = b;
}
/* 
NB1 Handles spikes, like lightning
Algorithm from dttsp and general books
Compute average mag and look for spikes > N times avg.
When found, blank output for 7 samples
Todo: Add variable threshold
*/
CPX * NoiseBlanker::ProcessBlock(CPX *in)
{
	int size = numSamples;
	if (!nbEnabled) {
		return in;
	}
	float mag = 0.0;
	for (int i =0; i < size; i++)
	{
		mag = magCpx(in[i]);
		//Insert current sample at head of delay line
		nbDelay->NewSample(in[i]);

		//Calc simple weighted average of signal magniture
		nbAverageMag = (0.999 * nbAverageMag) + (0.001 * mag);

		//If this sample > noise threshold and we're not already in blanking interval,
		//blank next N samples
		if (nbSpikeCount == 0 && mag > (nbAverageMag * nbThreshold))
			nbSpikeCount = nbSpike;
		if (nbSpikeCount > 0) {
			//Blank noise
			out[i].real(0.0);
			out[i].imag(0.0);
			nbSpikeCount --;
		} else {
			//Default delay is 2 samples, handled by DelayLine class.
			out[i] = nbDelay->NextDelay(0);
		}
	}
	return out;
}
//NB2 reduces average noise
//From dttsp, doesn't blank but inserts average whenever threshold is exceeded
CPX * NoiseBlanker::ProcessBlock2(CPX *in)
{
	int size = numSamples;
	if (!nb2Enabled) {
		return in;
		}
	float mag = 0.0;
	for (int i = 0; i < size; i++)
	{
		mag = magCpx(in[i]);
		//Weighted average 75/25
		nb2AverageCPX = scaleCpx(nb2AverageCPX, 0.75) + scaleCpx(in[i],0.25);
		nb2AverageMag = 0.999 * nb2AverageMag + 0.001 * mag;
		if (mag > (nbThreshold * nb2AverageMag))
			out[i] = nb2AverageCPX;
		else
			out[i] = in[i]; //This step not in dttsp, don't see how it would work
	}
	return out;
}
