#include "appdog.h"
#include "qmutex.h"
#include "qapplication.h"
#include "qdatetime.h"
#include "qtimer.h"
#include "qdebug.h"
#include "appsetting.h"
#define TIMEMS qPrintable(QTime::currentTime().toString("HH:mm:ss zzz"))

//#ifdef __arm__
#ifdef Q_OS_UNIX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/watchdog.h>

#define WATCHDOG_IOCTL_BASE 'W'
#define	WDIOC_KEEPALIVE _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define	WDIOC_SETTIMEOUT _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#endif

AppDog *AppDog::self = 0;
AppDog *AppDog::Instance()
{
    if (!self) {
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!self) {
            self = new AppDog;
        }
    }

    return self;
}

AppDog::AppDog(QObject *parent) : QObject(parent)
{
//#ifdef __arm__
#ifdef Q_OS_UNIX
    fd = 0;
    feedInterval = 3;
    timerDog = new QTimer(this);
    connect(timerDog, SIGNAL(timeout()), this, SLOT(feedWatchDog()));
//    timerDog->setInterval(feedInterval *1000);
    timerDog->setInterval(APPSetting::WatchDogInterval);
#endif
}

bool AppDog::start()
{
    //延时启动加密狗,避免程序启动后处理过多卡住界面

    QTimer::singleShot(5 * 1000, this, SLOT(runWatchDog()));

    return true;
}

void AppDog::stop()
{
//#ifdef __arm__
#ifdef Q_OS_UNIX
    timerDog->stop();
    close(fd);
#endif
}

void AppDog::runWatchDog()
{

//#ifdef __arm__
#ifdef Q_OS_UNIX
    fd = open("/dev/watchdog", O_WRONLY);

    if (fd == -1) {
        qDebug() << TIMEMS << "Start AppDog Error";
        return;
    } else {
        timerDog->start();
        qDebug() << TIMEMS << "Start AppDog Ok";
    }

    int timeout = feedInterval * 3;//10
    ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
    feedWatchDog();
#endif
}

void AppDog::feedWatchDog()
{
//#ifdef __arm__
#ifdef Q_OS_UNIX
    int dummy;
    ioctl(fd, WDIOC_KEEPALIVE, &dummy);
//    if(ioctl(fd, WDIOC_KEEPALIVE, &dummy) == -1){
//        qDebug()<<"Watch Dog fail!!!"<<TIMEMS;
//    }else{
//        qDebug()<<"feed Watch Dog!!!"<<TIMEMS;
//    }
#endif
}
