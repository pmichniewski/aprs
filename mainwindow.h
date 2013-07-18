#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QAudioInput>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private:
	Ui::MainWindow *ui;
	QGraphicsPixmapItem *m_waterfallPixmapItem;
	QGraphicsScene *m_waterfallScene;
	QImage *m_waterfallImage;
	QTimer *m_timer;
	QAudioInput *m_audioInput;
	QIODevice *m_audioDevice;

public slots:
	void updatePixmap();
	void processAudio();
};

#endif // MAINWINDOW_H
