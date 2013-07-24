#ifndef APRSDECODER_H
#define APRSDECODER_H

#include <stdint.h>
#include <QString>
#include <QFile>

const int sinTaps = 8;
const int filterTaps = 11;
const int decodeRate = 9600;
const int bps = 1200;

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 9600 Hz

fixed point precision: 16 bits

* 0 Hz - 1300 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = n/a

* 2500 Hz - 4800 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = n/a

*/
const double filter[] = {
-0.02129290352029659,
-0.03671659908131041,
0.006661063138798663,
0.1405373281860916,
0.30506115855474275,
0.3806400324285967,
0.30506115855474275,
0.1405373281860916,
0.006661063138798663,
-0.03671659908131041,
-0.02129290352029659
};

class aprsDecoder
{
public:
	aprsDecoder(int sampleRate);
	~aprsDecoder();
	void feedData(int16_t *data, int count);

protected:
	int m_sampleRate;
	double *m_s12;
	double *m_s22;
	double *m_c12;
	double *m_c22;
	int16_t *m_sampleBuf;
	int m_samplePos;
	int m_decimSkip; // number of samples to skip while decimating next buffer
	double *m_corrBuf;
	int m_corrPos;
	int m_time;
	int m_lastDiff;
	QString m_data;
	QFile *f;
};

#endif // APRSDECODER_H
