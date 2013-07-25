#include "aprsdecoder.h"
#include <math.h>
#include <QDebug>
#include "mainwindow.h"

aprsDecoder::aprsDecoder(int sampleRate) {
	m_sampleBuf = new int16_t[sinTaps]; // buffer for holding samples for sine convolution
	m_corrBuf = new double[filterTaps]; // buffer for holding samples for lowpass filter convolution

	for (int i = 0; i < sinTaps; i++) {
		m_sampleBuf[i] = 0;
	}
	m_samplePos = 0;

	for (int i = 0; i < filterTaps; i++) {
		m_corrBuf[i] = 0;
	}
	m_corrPos = 0;

	m_decimSkip = 0;
	m_sampleRate = sampleRate;
	m_s12 = new double[sinTaps];
	m_c12 = new double[sinTaps];
	m_s22 = new double[sinTaps];
	m_c22 = new double[sinTaps];

	f = new QFile("output.csv");
	f->open(QIODevice::WriteOnly | QIODevice::Text);
	for (int i = 0; i < sinTaps; i++) {
		m_s12[sinTaps - i - 1] = sin(2*M_PI*1200*i/decodeRate); // flipped impulse response
		m_c12[sinTaps - i - 1] = cos(2*M_PI*1200*i/decodeRate);
		m_s22[sinTaps - i - 1] = sin(2*M_PI*2200*i/decodeRate);
		m_c22[sinTaps - i - 1] = cos(2*M_PI*2200*i/decodeRate);
	}

	m_time = 0;
	m_lastTime = 0;
	m_lastDiff = 0;
	m_data.clear();
}

aprsDecoder::~aprsDecoder() {
	delete m_s12;
	delete m_c12;
	delete m_s22;
	delete m_c22;
	f->close();
	delete f;
}

void aprsDecoder::feedData(int16_t *data, int count) {
	bool m_hasData = false;
	for (int i = 0; i < count; i++) {
		if (data[i] != 0) {
			m_hasData = true;
		}
	}

	if (!m_hasData) {
		return;
	}

	double csum12;
	double ssum12;
	double csum22;
	double ssum22;
	double filteredDiff;

	int decimRate = m_sampleRate / decodeRate;

	int numDecimated = (count - m_decimSkip)/decimRate;
	if ((count - m_decimSkip)%decimRate > 0) {
		numDecimated++;
	}
	QTextStream out(f);
	double sampleVal = 0;

	for (int i = 0; i < numDecimated; i++) {
		m_sampleBuf[m_samplePos++] = data[m_decimSkip + i*decimRate];
		m_samplePos %= sinTaps;
		csum12 = 0;
		ssum12 = 0;
		csum22 = 0;
		ssum22 = 0;
		for (int j = 0; j < sinTaps; j++) {
			int snum = (m_samplePos + j)%sinTaps; //m_samplePos + j;
			sampleVal = (m_sampleBuf[snum]/32767.0);
			csum12 += sampleVal * m_c12[j];
			ssum12 += sampleVal * m_s12[j];
			csum22 += sampleVal * m_c22[j];
			ssum22 += sampleVal * m_s22[j];
		}
//		csum12 /= 128;
//		ssum12 /= 128;
//		csum22 /= 128;
//		ssum22 /= 128;
		double diff = (csum12 * csum12 + ssum12 * ssum12) - (csum22 * csum22 + ssum22 * ssum22);
//		out << (data[m_decimSkip + i*decimRate]/32767.0) << ", " << sqrt(csum12 * csum12 + ssum12 * ssum12) - sqrt(csum22 * csum22 + ssum22 * ssum22) << "\n";
		m_corrBuf[m_corrPos++] = diff;
		if (m_corrPos >= filterTaps)
			m_corrPos = 0;

		filteredDiff = 0;

		for (int j = 0; j < filterTaps; j++) {
			int snum = (m_corrPos + j) % filterTaps;
			filteredDiff = filteredDiff + m_corrBuf[snum] * filter[j];
		}
		out << (data[m_decimSkip + i*decimRate]/32767.0) << ", " << filteredDiff << "\n";

		if ((filteredDiff > 0 && m_lastDiff < 0) ||
				(filteredDiff < 0 && m_lastDiff > 0) ||
				(filteredDiff == 0)) {
			if (m_time < 4 || m_time > 60) { // error
				m_data.clear();
			} else {
				if (m_time < 13) { // received "0"
					if (m_lastTime > 52 && m_lastTime < 61) { // FLAG received
						if (m_data.length() > 0)
							qDebug() << m_data;
						m_data.clear();
					} else {
						m_data.append("0");
					}
				} else if (m_time < 21) { // received "10"
					m_data.append("10");
				} else if (m_time < 29) { // received "110"
					m_data.append("110");
				} else if (m_time < 37) { // received "1110"
					m_data.append("1110");
				} else if (m_time < 45) { // received "11110"
					m_data.append("11110");
				} else if (m_time < 53) { // received "11111"
					m_data.append("11111");
				}
			}
			m_lastTime = m_time;
			m_time = 0;
		}

		m_lastDiff = filteredDiff;
		m_time++;
	}

	m_decimSkip = (5 - count % 5 + m_decimSkip) % 5;
}
