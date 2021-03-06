#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QAudioInput>
#include <fftw3.h>
#include <stdint.h>
#include "aprsdecoder.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	void drawSample(int sample);
	
private:
	Ui::MainWindow *ui;
	QGraphicsPixmapItem *m_waterfallPixmapItem;
	QGraphicsScene *m_waterfallScene;
	QImage *m_waterfallImage;
	QTimer *m_timer;
	QAudioInput *m_audioInput;
	QIODevice *m_audioDevice;
	QByteArray m_buffer;
	qint64 m_bufferPos;
	double *m_fft_in;
	fftw_complex *m_fft_out;
	fftw_plan m_fft_plan;
	QRgb m_magRgb[256];
	aprsDecoder *m_decoder;

public slots:
	void updatePixmap();
	void audioData();
	void audioInterval();
};

#endif // MAINWINDOW_H
