#ifndef UDPSLOTAPI_H
#define UDPSLOTAPI_H

#include <QObject>
#include "udpmulticastapi.h"
class UDPSlotAPI : public QObject
{
    Q_OBJECT
public:
    static UDPSlotAPI *Instance();
    explicit UDPSlotAPI(QObject *parent = nullptr);
    void Init();
private:
    static UDPSlotAPI *self;

private slots:
    void UDPAddLog(const QString &carNumber,const QString &WagonNumber,const int &DeviceID, const int &DeviceCh, const QString &DeviceName,
                   const QString &LogType, const int &AlarmGrade, const QString &TriggerTime, const QString &LogContent = QString());
    void UDPAddLog1(const QString &carNumber, const QString &WagonNumber, const QString &LogType, const QString &TriggerTime, const QString &LogContent);
    void UDPAddEigenvalue(QString Wagon, qint8 id, qint8 ch, quint32 speed, double AmbientTem, double PointTem, QString time,
                          QVector<float> Dimensional,QVector<float> Demodulated);
    void UDPAddBearing(QString modelname, float PitchDiameter, float RollingDiameter, int RollerNum, float ContactAngle);
    void UDPDeleteBearing(QString BearingName);
    void UDPAddDevice(int DeviceId, int DeviceCH, QString DeviceName, QString DeviceType, float DeviceSensitivity, float ShaftDiameter,
                      QString Bearing1Typedef, QString Bearing1_model,QString Bearing2Typedef, QString Bearing2_model, QString Bearing3Typedef,
                      QString Bearing3_model, QString Bearing4Typedef, QString Bearing4_model,QString capstanName, int capstanTeethNum,
                      QString DrivenwheelName, int DrivenwheelTeethNum, QString version, bool Enable);
    void UDPDeleteDevice(int DeviceId, int DeviceCH);
    void UDPAddTask(QString Action, QString TriggerTime);
    void UDPDeleteTasks(QString action);
    void UDPAddLinkDevice(QString Type, QString IP, int Port, QString WagonNumber);
    void UDPDeleteLinkDevice(QString IP);
    void RunSQLStr(QString str);
    void UDPDeleteDimensional(QString Deadlinetime);
    void UDPClearAlarm(qint8 id, qint8 ch);
    void UDPReboot();
    void reboot();
    void UDPTestAddPRE(QString str);
signals:

};

#endif // UDPSLOTAPI_H
