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
const int16_t filter[] = {
  -1395,
  -2406,
  437,
  9210,
  19992,
  24946,
  19992,
  9210,
  437,
  -2406,
  -1395
};

class aprsDecoder
{
public:
	aprsDecoder(int sampleRate);
	~aprsDecoder();
	void feedData(int16_t *data, int count);

protected:
	int m_sampleRate;
	int *m_s12;
	int *m_s22;
	int *m_c12;
	int *m_c22;
	int16_t *m_sampleBuf;
	int m_samplePos;
	int m_decimSkip; // number of samples to skip while decimating next buffer
	int16_t *m_corrBuf;
	int m_corrPos;
	int m_time;
	int m_lastDiff;
	QString m_data;
	QFile *f;
};

#endif // APRSDECODER_H
