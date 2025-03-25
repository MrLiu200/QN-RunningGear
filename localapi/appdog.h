#ifndef APPDOG_H
#define APPDOG_H

#include <QObject>

class QTimer;

class AppDog : public QObject
{
	Q_OBJECT
public:
    static AppDog *Instance();
    explicit AppDog(QObject *parent = 0);

    bool start();
    void stop();

private:
	static AppDog *self;

	int fd;
	int feedInterval;
	QTimer *timerDog;

signals:

public slots:
	void runWatchDog();
	void feedWatchDog();
};

#endif // APPDOG_H
