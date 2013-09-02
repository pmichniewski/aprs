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

	m_data = new uint8_t[buffer_size];
	m_dataLength = 0;

	m_time = 0;
	m_lastTime = 0;
	m_lastDiff = 0;

	m_currentState = ERROR;
	m_nextState = ERROR;
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

	int numDecimated = (count - m_decimSkip)/decimRate; // number of decimated samples
	if ((count - m_decimSkip)%decimRate > 0) {
		numDecimated++;
	}

//	QTextStream out(f);

	double sampleVal = 0;
	for (int i = 0; i < numDecimated; i++) {
		// decimation
		m_sampleBuf[m_samplePos++] = data[m_decimSkip + i*decimRate];
		m_samplePos %= sinTaps;

		// correlation
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

		//correlation difference
		double diff = (csum12 * csum12 + ssum12 * ssum12) - (csum22 * csum22 + ssum22 * ssum22);
//		out << (data[m_decimSkip + i*decimRate]/32767.0) << ", " << sqrt(csum12 * csum12 + ssum12 * ssum12) - sqrt(csum22 * csum22 + ssum22 * ssum22) << "\n";
		m_corrBuf[m_corrPos++] = diff;
		if (m_corrPos >= filterTaps)
			m_corrPos = 0;

		// low-pass filtering of correlation difference
		filteredDiff = 0;
		for (int j = 0; j < filterTaps; j++) {
			int snum = (m_corrPos + j) % filterTaps;
			filteredDiff = filteredDiff + m_corrBuf[snum] * filter[j];
		}
//		out << (data[m_decimSkip + i*decimRate]/32767.0) << ", " << filteredDiff << "\n";

		// decoding
		if ((filteredDiff > 0 && m_lastDiff < 0) || // zero-crossing detected
				(filteredDiff < 0 && m_lastDiff > 0) ||
				(filteredDiff == 0)) {
			if (m_time < 4 || m_time > 60) { // 0..3 or 61+ error detected
				m_nextState = ERROR;
			} else if (m_time > 52 && m_time < 61) { // 53..60 FLAG received
				if (m_dataLength > 0) { // remove last bit from start of FLAG byte
					m_dataLength--;
				}
				m_nextState = RECEIVING;
				QString msg;
				if (m_dataLength >= 24) { // at least 3 bytes (1 data + 2 CRC)
					for (int i = 0; i < m_dataLength/8; i++) {
						msg+= QString::number(m_data[i], 16) + " ";
					}
//					qDebug() << m_dataLength << ": " << msg;
					parsePacket();
				}
				memset(m_data, 0, buffer_size);
				m_dataLength = 0;
			} else {
				switch (m_currentState) {
				case ERROR:
					m_nextState = ERROR;
					break;
				case RECEIVING:
					if (m_time < 13) { // 5..12 received "0"
						addBits(0x00, 1);
					} else if (m_time < 21) { // 13..20 received "10"
						addBits(0x02, 2);
					} else if (m_time < 29) { // 21..28 received "110"
						addBits(0x06, 3);
					} else if (m_time < 37) { // 29..36 received "1110"
						addBits(0x0e, 4);
					} else if (m_time < 45) { // 37..44 received "11110"
						addBits(0x1e, 5);
					} else if (m_time < 53) { // 45.. 52 received "11111"
						addBits(0x1f, 5);
					}
					break;
				}
			}

			m_lastTime = m_time;
			m_time = 0;
		}

		m_currentState = m_nextState;
		m_lastDiff = filteredDiff;
		m_time++;
	}

	m_decimSkip = (5 - count % 5 + m_decimSkip) % 5; // how many samples to skip from the next buffer
}

void aprsDecoder::addBits(uint8_t bits, int count) {
	for (int i = 0; i < count; i++) {
		m_data[(m_dataLength + i) / 8] |= ((bits >> (count - i - 1)) & 1) << ((m_dataLength + i)%8);
	}
	m_dataLength += count;
}

void aprsDecoder::parsePacket() {
	uint16_t crc_result = 0xffff;
	for (int i = 0; i < m_dataLength/8 - 2; i++) {
		crc_result = crc_result >> 8 ^ crc_tab[(crc_result ^ m_data[i]) & 0xff];
	}
	crc_result = ~crc_result;
	uint16_t packet_crc = (m_data[m_dataLength/8 - 2]) | (m_data[m_dataLength/8 - 1] << 8);
	if (crc_result == packet_crc) {
		char dest_call[7];
		char src_call[7];
		char repeater_call[7];
		uint8_t dest_ssid;
		uint8_t src_ssid;
		uint8_t repeater_ssid;
		bool last;
		bool repeated;
		QString msg;
		QTextStream stream(&msg);

		last = decodeAddress(1, src_call, src_ssid, repeated);
		stream << src_call;
		if (src_ssid > 0)
			stream << "-" << src_ssid;
		if (repeated)
			stream << "*";
		stream << ">";

		last = decodeAddress(0, dest_call, dest_ssid, repeated);
		stream << dest_call;
		if (dest_ssid > 0)
			stream << "-" << dest_ssid;
		if (repeated)
			stream << "*";
		if (!last)
			stream << ",";

		int repeater_num = 2;
		while (!last) {
			last = decodeAddress(repeater_num++, repeater_call, repeater_ssid, repeated);
			stream << repeater_call;
			if (dest_ssid > 0)
				stream << "-" << repeater_ssid;
			if (repeated)
				stream << "*";
			if (!last)
				stream << ",";
		}
		qDebug() << msg;
	}
}

bool aprsDecoder::decodeAddress(int addrNum, char *callsign, uint8_t &ssid, bool &repeated) {
	for (int i = 0; i < 6; i++) {
		callsign[i] = m_data[addrNum * 7 + i] >> 1;
		if (callsign[i] == 0x20) callsign[i] = 0;
	}
	callsign[6] = 0;
	ssid = (m_data[addrNum * 7 + 6] & 0x1e) >> 1;
	repeated = (m_data[addrNum * 7 + 6] & 0x80) == 0x80;
	if ((m_data[addrNum * 7 + 6] & 0x01) == 0x01) {
		return true;
	} else {
		return false;
	}
}
