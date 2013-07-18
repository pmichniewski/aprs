#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>
#include <QDebug>
#include <stdint.h>
#include <QAudioFormat>

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
	format.setChannels(1);
	format.setSampleSize(16);
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	m_audioInput = new QAudioInput(format, this);
//	m_audioInput->setNotifyInterval(50);
	m_audioDevice = m_audioInput->start();
	connect(m_audioDevice, SIGNAL(readyRead()), this, SLOT(processAudio()));
	m_audioInput->resume();
}

MainWindow::~MainWindow()
{
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

void MainWindow::processAudio() {
	qDebug() << "data";
}
