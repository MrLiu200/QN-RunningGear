#ifndef BUS2API_H
#define BUS2API_H

#include <QObject>
#include <QMutex>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusDeviceInfo>
#include <QCanBusFrame>
#include <QDebug>
#include <QFile>
#include "appsetting.h"
#include <QMap>
#include "dbdata.h"
#include "m_databaseapi.h"
#include "udpmulticastapi.h"

#define SPEEDBOARDID 0x03
#define COMMUNICATIONBOARDID 0x09
#define PREIOBOARDID 0x08

enum CommandByte{
    CommandByte_SelfInspection = 0x01,
    CommandByte_Heartbeat,
    CommandByte_Speed,
    CommandByte_Version,
    CommandByte_UpdateRequest,
    CommandByte_SendUpdateFile,
    CommandByte_RebootRequest,
    CommandByte_PowerControl,
    CommandByte_PISLedControl
};

enum UpdateState{
    UpdateState_OK = 0,
    UpdateState_Abnormal,
};

class Bus2API : public QObject
{
    Q_OBJECT
public:
    explicit Bus2API(QObject *parent = nullptr);
    static Bus2API *Instance();
    ~Bus2API();
    bool Open();
private:
    static Bus2API *self;
    std::unique_ptr<QCanBusDevice> m_canDevice;
    bool IsOpen;
    QMap<uint8_t,QString> VersionMap;
    QMap<uint8_t,QString> StateMap;
    QString UpdateFile;
    uint32_t UpdateCRC;
    uint8_t UpdateDevice;
    QTimer *timerPISState;
    QString PISState;
    QFile ETH1FileIndex;


private slots:
    void processReceivedFrames();

    void processErrors(QCanBusDevice::CanBusError error) const;

    void checkData(QByteArray frameData,quint32 frameID);

    //发送数据
    void SendData(const QCanBusFrame &frame);
    //数据包生成
    QByteArray GetSendPackage(uint8_t FrameID, uint8_t order, QByteArray data);

    //上位机指定升级设备
    void BackboardCardSoftwareUpdate(const QString filename,const uint32_t crc,uint8_t devicetype);

    //获取eth1网口状态
    void GetPISNetState();

public:
    //获取自检信息
    void GetSelfInspection(uint8_t FrameID);
    //获取版本信息
    void GetVersion(uint8_t frameID);
    //更新软件请求
    void UpdateAPPRequest(uint8_t frameID);
    //发送更新文件
    void SendUpdateFile(uint8_t frameID,QString filename);
    //重启设备
    void RebootDevice(uint8_t frameID);
    //获取软件版本
    QByteArray GetVersionMap(uint8_t key = 0xFF);
    //更新板卡版本
    void SetVersionMap(uint8_t key, uint16_t value);
    //获取板卡自检状态
    QByteArray GetStateMap(uint8_t key = 0xFF);
    //更新板卡自检状态
    void SetStateMap(uint8_t key, uint8_t value);
    //设置更新文件及crc
    void SetUpdateFile(QString file,uint32_t crc);
    //设置前置IO板卡电源
    void SetPreIOPowerState(bool state);
    //设置PIS网口指示灯
    void SetPISLedState(uint8_t state);
signals:
    void VersionReturn(uint8_t id, uint32_t value);
    void UpdateReturn(uint8_t id, uint8_t state);
    void RebootReturn(uint8_t id, uint8_t state);
};

#endif // BUS2API_H
