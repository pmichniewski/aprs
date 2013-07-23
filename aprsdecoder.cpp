#include "aprsdecoder.h"
#include <math.h>

aprsDecoder::aprsDecoder(int sampleRate) {
	m_resampBuf = new int16_t[sampleRate / decodeRate]; // buffer for holding samples left from previous resampling
	m_sampleBuf = new int16_t[corrTaps]; // buffer for holding samples for sine convolution
	m_corrBuf = new int16_t[filterTaps]; // buffer for holding samples for lowpass filter convolution

	for (int i = 0; i < 40; i++) {
		m_s12[i] = 128 * sin(2*M_PI*1200*i/48000);
		m_c12[i] = 128 * cos(2*M_PI*1200*i/48000);
		m_s22[i] = 128 * sin(2*M_PI*2200*i/48000);
		m_c22[i] = 128 * cos(2*M_PI*2200*i/48000);
	}
}

void aprsDecoder::feedData(int16_t *data, int count) {
	int csum12;
	int ssum12;
	int csum22;
	int ssum22;

//	for (int n = 0; n < count; n++) {
//		csum12 = 0;
//		ssum12 = 0;
//		csum22 = 0;
//		ssum22 = 0;
//		for (int i = 0; i < 40; i++) {
//			int snum = pos - 39 + i;
//			if (snum < 0) snum += size; // last 40 samples
//			if (snum >= size) snum -= size;
//			csum12 += data[snum] * c12[i];
//			ssum12 += data[snum] * s12[i];
//			csum22 += data[snum] * c22[i];
//			ssum22 += data[snum] * s22[i];
//		}
//		csum12 /= 256;
//		ssum12 /= 256;
//		csum22 /= 256;
//		ssum22 /= 256;

//		double result = csum12 * csum12 + ssum12 * ssum12 - csum22 * csum22 - ssum22 * ssum22;
//		if (abs(result) > 100) {
//			if (result > 0) {
//				output += '0';
//			} else {
//				output += '1';
//			}
//		}
//
//		pos++;
//	}
//	if (output.length() > 0) qDebug() << output;

}
