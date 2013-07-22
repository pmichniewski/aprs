#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>
#include <QDebug>
#include <stdint.h>
#include <QAudioFormat>

//const qint64 BufferDurationUs = 1 * 1000000; // 1s
const int NotifyIntervalMs = 50;
const uint8_t pal[9][3] =
{{0, 0, 0},
{0, 6, 136},
{0, 19, 198},
{0, 32, 239},
{172, 167, 105},
{194, 198, 49},
{225, 228, 107},
{255, 255, 0},
{255, 51, 0}};

int s12[40];
int s22[40];
int c12[40];
int c22[40];

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	m_waterfallScene = new QGraphicsScene(this);
	ui->graphicsView->setScene(m_waterfallScene);

	m_waterfallPixmapItem = new QGraphicsPixmapItem();
	m_waterfallScene->addItem(m_waterfallPixmapItem);
	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_waterfallScene->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));

	m_waterfallImage = new QImage(1500, 200, QImage::Format_ARGB32);

	QAudioFormat format;
	format.setSampleRate(48000);
	format.setChannelCount(1);
	format.setSampleSize(16);
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	QAudioFormat preferred = QAudioDeviceInfo::defaultInputDevice().nearestFormat(format);
	qDebug() << preferred.channelCount() << " " << preferred.sampleRate() << " " << preferred.sampleSize() << " " << preferred.sampleType();
	qDebug() << "Default device: " << QAudioDeviceInfo::defaultInputDevice().deviceName();

	foreach(QAudioDeviceInfo device, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
		qDebug() << "device: " << device.deviceName();
	}

	m_audioInput = new QAudioInput(preferred, this);
	m_audioInput->setNotifyInterval(NotifyIntervalMs);
	m_audioDevice = m_audioInput->start();
	connect(m_audioDevice, SIGNAL(readyRead()), this, SLOT(audioData()));
	connect(m_audioInput, SIGNAL(notify()), this, SLOT(audioInterval()));
	m_audioInput->resume();

	m_buffer.clear();
	m_bufferPos = 0;

	const qint64 bufferLength = 2*format.sampleRate(); // 1s of audio buffer
	m_buffer.resize(bufferLength);
	m_buffer.fill(0);

	m_fft_in = (double*) fftw_malloc(sizeof(double) * 48000);
	m_fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 48000);
	m_fft_plan = fftw_plan_dft_r2c_1d(48000, m_fft_in, m_fft_out, FFTW_ESTIMATE);

	int r,g,b;
	for (int n = 0; n < 8; n++) {
			 for (int i = 0; i < 32; i++) {
				r = pal[n][0] + (int)(1.0 * i * (pal[n+1][0] - pal[n][0]) / 32.0);
				g = pal[n][1] + (int)(1.0 * i * (pal[n+1][1] - pal[n][1]) / 32.0);
				b = pal[n][2] + (int)(1.0 * i * (pal[n+1][2] - pal[n][2]) / 32.0);
				m_magRgb[i + 32*n] = qRgb(r, g, b);
			 }
	 }

	m_lastPos = 0;
	for (int i = 0; i < 40; i++) {
		s12[i] = 128 * sin(2*M_PI*1200*i/48000);
		c12[i] = 128 * cos(2*M_PI*1200*i/48000);
		s22[i] = 128 * sin(2*M_PI*2200*i/48000);
		c22[i] = 128 * cos(2*M_PI*2200*i/48000);
	}
}

MainWindow::~MainWindow()
{
	fftw_destroy_plan(m_fft_plan);
	fftw_free(m_fft_in);
	fftw_free(m_fft_out);
	delete ui;
}

void MainWindow::updatePixmap() {
	QRgb* pixels = (QRgb*)m_waterfallImage->bits();
	int w = m_waterfallImage->width();
	int h = m_waterfallImage->height();

	for (int y = 0; y < h - 1; y++) {
		for (int x = 0; x < w; x++) {
			pixels[(y * w) + x] = pixels[((y + 1) * w) + x];
		}
	}
	for (int x = 0; x < w; x++) {
		uint8_t r = 256.0 * rand() / RAND_MAX;
		uint8_t g = 256.0 * rand() / RAND_MAX;
		uint8_t b = 256.0 * rand() / RAND_MAX;
		pixels[((h - 1) * w) + x] = 0xff000000 | r << 16 | g << 8 | b;
	}

	m_waterfallPixmapItem->setPixmap(QPixmap::fromImage(*m_waterfallImage));
}

void MainWindow::audioData() {
	if (m_bufferPos >= m_buffer.size()) {
		m_bufferPos -= m_buffer.size(); // wrap circular buffer
	}
	const qint64 bytesReady = m_audioInput->bytesReady();
	const qint64 bytesSpace = m_buffer.size() - m_bufferPos;
	const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

	const qint64 bytesRead = m_audioDevice->read(m_buffer.data() + m_bufferPos, bytesToRead);
	m_bufferPos += bytesRead;
}

void MainWindow::audioInterval() {
//	qDebug() << "interval";
	const int16_t* data = (const int16_t*)m_buffer.data();
	int pos = m_bufferPos / 2; // samples are 16-bit Doh!
	int size = m_buffer.size() / 2; // same here!

	for (int i = 0; i < size; i++) {
		if (pos >= size) {
			pos -= size;
		}
		m_fft_in[i] = data[pos++] * (0.54 - 0.46 * cos (2*M_PI*i/47999));
	}

	fftw_execute(m_fft_plan);

	QRgb* pixels = (QRgb*)m_waterfallImage->bits();
	int w = m_waterfallImage->width();
	int h = m_waterfallImage->height();

	for (int y = 10; y < h - 1; y++) {
		for (int x = 0; x < w; x++) {
			pixels[(y * w) + x] = pixels[((y + 1) * w) + x];
		}
	}

	for (int y = 0; y < 10; y++) {
		for (int f = 0; f < 3000; f++) {
			if (f % 100 == 0) {
				pixels[(y * w) + f/2] = 0xffffffff;
			}
		}
	}

	double mag;
	int pixel;
	for (int x = 0; x < w; x++) {
		mag = sqrt(m_fft_out[x*2][0]*m_fft_out[x*2][0] + m_fft_out[x*2][1] * m_fft_out[x*2][1]) / 24000.0 / 256.0;
		pixel = (int)(400*log10(mag));
		if (pixel > 255) pixel = 255;
		if (pixel < 0) pixel = 0;
		pixels[((h - 1) * w) + x] = 0xff000000 | m_magRgb[pixel];
	}

	m_waterfallPixmapItem->setPixmap(QPixmap::fromImage(*m_waterfallImage));

	int count = pos - m_lastPos;
	if (count < 0) count += size;

	QString output = "";

	int csum12;
	int ssum12;
	int csum22;
	int ssum22;
	for (int n = 0; n < count; n++) {
		csum12 = 0;
		ssum12 = 0;
		csum22 = 0;
		ssum22 = 0;
		for (int i = 0; i < 40; i++) {
			int snum = pos - 39 + i;
			if (snum < 0) snum += size; // last 40 samples
			if (snum >= size) snum -= size;
			csum12 += data[snum] * c12[i];
			ssum12 += data[snum] * s12[i];
			csum22 += data[snum] * c22[i];
			ssum22 += data[snum] * s22[i];
		}
		csum12 /= 256;
		ssum12 /= 256;
		csum22 /= 256;
		ssum22 /= 256;

		double result = csum12 * csum12 + ssum12 * ssum12 - csum22 * csum22 - ssum22 * ssum22;
		if (abs(result) > 100) {
			if (result > 0) {
				output += '0';
			} else {
				output += '1';
			}
		}
\
		pos++;
	}
	if (output.length() > 0) qDebug() << output;
	m_lastPos = pos;
}
