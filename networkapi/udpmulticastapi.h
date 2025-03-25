#ifndef UDPMULTICASTAPI_H
#define UDPMULTICASTAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include "bus2api.h"
#include "dbdata.h"

#define UDP_PACKET_HEAD1 0xA6
#define UDP_PACKET_HEAD2 0xA6
#define UDP_PACKET_END1  0x6A
#define UDP_PACKET_END2  0x6A
#include <QThread>
#include <QMutex>



#define UDP_DATA_OFFSET 4

class UDPMulticastAPI : public QThread
{
    Q_OBJECT
public:
    static UDPMulticastAPI *Instance();
    explicit UDPMulticastAPI(QObject *parent = nullptr);
    ~UDPMulticastAPI();
private:
    static UDPMulticastAPI *self;
    QUdpSocket *udpServer;
    QHostAddress m_ip;
    bool IsOpen;
    QTimer *timerHeartbeat;
    QTimer *timerUpdate;
    QTimer *timerSynchronous;
    int currentPage;
    int AllPage;
    QFile UpdateFile;
    bool HostOffline;
#if 1
private:
    bool Stopped;
    QMutex m_mutex;
    QByteArrayList DataList;
    void WriteDataToList(const QByteArray data);
    qint64 GetDataListSize();
    QByteArray ReadFirstDataFromList();

protected:
    void run() override;
#endif
public:
    bool startJoin();
    bool Stop(void);

private slots:
    void readData();
    void CheckData(const QByteArray data);

public:
    //发送自检数据
    void SendSelfInspection(QString Data);

    //发送温度巡检结果
    void SendTemInspection(QString Data);

    //发送综合巡检结果
    void SendComprehensiveInspection(QString Data);

    //更新失败
    void UpdateFailed(uint8_t preid);

    //板卡状态发生变化(前置处理器、转速板卡、通信板卡、前置IO板卡等)
    void SendBoardStatus(QString info);

private:
    //发送数据
    void senddata(const QByteArray data);

    //生成发送数据包
    QByteArray GetPackage(uint8_t order, QString data);

    //更新系统时间
    void updateTime(QString DataContent);

    //自检
    void SelfInspection(QString DataContent);

    //收到自检内容
    void ReceiveSelfInspection(QString DataContent);

    //收到温度巡检
    void TemInspection(QString DataContent);

    //收到综合巡检
    void ComprehensiveInspection(QString DataContent);

    //收到心跳数据包
    void HeartbeatEvent(QString DataContent);

    //下发校时
    void Time_Aligned(void);

    //下发自检任务
    void StartSelfInspection(QString WagonName = "all",QString preID = "0");

    //发送心跳数据包
    void SendHeartbeat();

    //下发转速信息
    void SendSynchronous();

    //更新转速信息
    void UpdateSynchronous(QString DataContent);

    //记录板卡状态信息
    void UpdateBoardStatus(QString DataContent);

    //上位机调试部分指令

    //更新指令
    void UpdateSoftware(QString info);

    //设备重启
    void DeviceReboot(QString info);

    //更新轴承信息
    void UpdateBearing(QString info);

    //更新前置处理器信息
    void UpdatePre(QString info);

    //更新定时任务
    void UpdateTimerTask(QString info);

    //更新车辆信息
    void UpdateCarInfo(QString info);

    //更新网络信息
    void UpdateNetWorkConfing(QString info);

    //更新主/从机关联信息
    void UpdateLinkDevice(QString info);

    //更新数据保存间隔
    void UpdateSaveInterval(QString info);

    //更新报警阈值信息
    void UpdateAlarmLimit(QString info);

    //删除指定文件
    void UpdateZIPFile(QString info);

    //删除指定日期前的量纲
    void DeleteDimensionalValue(QString info);

    //删除指定日期前的日志
    void DeleteLog(QString info);

    //消除警报LED
    void ClearAlarmLedState(QString info);

    //获取前置处理器状态
    void GetPreStateList(QString info);

    //获取轴承信息
    void GetBearingInfo(QString info);

    //获取定时任务信息
    void GetTimerTask(QString info);

    //获取车辆信息
    void GetCarConfig(QString info);

    //获取网络信息
    void GetNetworkConfig(QString info);

    //获取关联主从机信息
    void GetLinkDeviceInfo(QString info);

    //获取数据保存间隔
    void GetSaveInterval(QString info);

    //获取背板状态
    void GetOtherBoardState(QString info);

    //获取报警阀值
    void GetAlarmLimit(QString info);

    //获取软件版本
    void GetAppVersion(QString info);

    //错误命令
    void HMIError(uint8_t error_order,QString Content = QString());

public slots:
    void PreStateChange(uint8_t id, uint8_t ch, DBData::DeviceState state);
    void OtherBoardStateChange(uint8_t id, DBData::DeviceState state);
    void SetTimerState(bool enable);
signals:
    void PreSoftwareUpdate(const QString filename,const uint32_t crc,uint8_t deviceID);//前置处理器更新
    void BackboardCardSoftwareUpdate(const QString filename,const uint32_t crc,uint8_t devicetype);//背板板卡更新
    void SlaveOnlineStatusChange(uint8_t carnum,uint8_t status);//从机在线状态发生改变
    void Signal_SelfInspection(uint8_t pre_id);               //自检信号 ,id == 0 表示所有设备均需自检，否则为指定前置自检
    void HostGoOnline();
    void UDPAddLog(const QString &carNumber,const QString &WagonNumber,const int &DeviceID, const int &DeviceCh, const QString &DeviceName,
                   const QString &LogType, const int &AlarmGrade, const QString &TriggerTime, const QString &LogContent = QString());
    void UDPAddLog1(const QString &carNumber, const QString &WagonNumber, const QString &LogType, const QString &TriggerTime, const QString &LogContent);
    void UDPAddEigenvalue(QString Wagon, qint8 id, qint8 ch, uint32_t speed, double AmbientTem,double PointTem,QString time,
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
    void UDPSetTimerState(bool enable);
    void TestAddPRE(QString str);
};

#endif // UDPMULTICASTAPI_H
