#ifndef TASKAPI_H
#define TASKAPI_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include "dbdata.h"
#include "corehelper.h"
#include "appsetting.h"
#include "bus2api.h"
#include "m_databaseapi.h"
#include "datalog.h"
class TaskAPI : public QObject
{
    Q_OBJECT
public:
    static TaskAPI *Instance();
    explicit TaskAPI(QObject *parent = nullptr);

private:
    static TaskAPI *self;
    QTimer *timerCheckCan;
#ifndef DEVICE_SLAVE
    QTimer *timerCheckSlave;
#endif
    QTimer *timerOtherBoard;
    QTimer *timerTask;
    QMap<uint8_t,QList<uint8_t>> errorPrelist;
    void checkListInit();
private slots:
    void CheckCanTask();
    void CheckSlaveTask();
    void CheckOtherBoard();
    void CheckTimerTask();

public slots:
    bool existErrorprelist(uint8_t preid,uint8_t ch);
    void removeErrorprelist(uint8_t preid,uint8_t ch);
    void addErrorprelist(uint8_t preid,uint8_t ch);
    void clearErrorPre();
    void clearErrorPre(uint8_t preid,uint8_t ch);
#ifndef DEVICE_SLAVE
    void clearErrorSlave();
    void clearErrorSlave(QString Wagon);
#endif
    void clearErrorOtherBoard();
    void clearErrorOtherBoard(uint8_t id);
    void Start();
    void Stop();
signals:
    void HostOffline();
    void PreStateChange(uint8_t id, uint8_t ch,DBData::DeviceState state);
    void OtherBoardStateChange(uint8_t id,DBData::DeviceState state);
};

#endif // TASKAPI_H
