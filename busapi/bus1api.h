#ifndef BUS1API_H
#define BUS1API_H

#include <QObject>
#include <QMutex>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusDeviceInfo>
#include <QCanBusFrame>
#include <QDebug>
#include "appsetting.h"
#include "m_databaseapi.h"
#include "bus0api.h"

struct PRE_SELF_INSPECTION;
struct PRE_TEM_VALUE;
struct PRE_VIBRATION_DATA;
struct TEMPERATURESTRUCT;
struct TASK_CHANNEL_LIST;
struct TASK_PRE_LIST;

class Bus1API : public QObject
{
    Q_OBJECT
public:
    static Bus1API *Instance();
    explicit Bus1API(QObject *parent = nullptr);

    bool Open();
private:
    static Bus1API *self;
    std::unique_ptr<QCanBusDevice> m_canDevice;
    bool IsOpen;
    quint32 currentpreID;
    quint32 currentpreCH;
    uint32_t currentStartSpeed;
    uint32_t currentEndSpeed;
    QByteArray VibdataArr;
    qint32 VIBDataLen;
    QList<TEMPERATURESTRUCT> temperaturelist;
    TASK_PRE_LIST task_pre_list;
    int savefilecount;
    QList<int> Bus1PreList;

private slots:
    void processReceivedFrames();

    void processErrors(QCanBusDevice::CanBusError error) const;

    void checkData(QByteArray frameData,quint32 frameID);

private:
    //获取发送的数据包
    QByteArray GetSendPackage(uint8_t pre_id, uint8_t order, QByteArray data);
    //发送数据
    void SendData(const QCanBusFrame &frame);
    //查找温度链表内容
    double findTemperatures(uint8_t preid, uint8_t prech);
    //添加温度链表内容
    void addTemperatures(uint8_t preid, uint8_t prech, double value);
    //添加任务列表内容，仅限自检时，一旦开始任务就不可添加
    void addTaskprelist(uint8_t preid, uint8_t channelsNumber, QVector<uint8_t> CH_Status);
    //更新板卡状态
    void UpdateDeviceState(uint8_t preid, uint8_t prech, bool IsFault = false);
    //检查量纲是否需要报警
    void CheckDimensionState(quint8 id, quint8 ch, int type, float rmsvalue, float ppvalue);

public:
    //设备自检命令
    void CommandSelfInspection(uint8_t PreID);
    //温度采集任务
    void CommandTemInspection(uint8_t PreID);
    //振动采集任务
    void CommandVibInspection(uint8_t PreID, uint8_t PreCH);
    //读取振动数据
    void CommandReadVibData(uint8_t PreID, uint8_t PreCH);
    //更新任务
    void CommandPreUpdate(uint8_t PreID, uint32_t crc, int len);
    //发送更新文件
    void CommandSendFile(QString filename);
    //重启任务
    void CommandReboot(uint8_t PreID);
    //获取当前任务状态 返回值：正在工作的id、ch、state
    QByteArray GetTaskState();
    //切换下一个可工作板卡 状态为空闲(0)或等待读取(2)或接收完成(4)，返回值： 正在工作的id、ch、state
    QByteArray switchNextTask();
    //切换下一个通道，在该通道采集完成后使用
    void switchNextChannel(uint8_t preid);
    //设置当前任务状态
    void SetTaskState(uint8_t preid,TASKSTATE state);
    //查询列表是否包含此ID
    bool TaskIsContain(uint8_t preid);
    //获取所有前置设备列表
    QList<uint8_t> GetPreDeviceList();
    //获取端口打开状态
    bool BusIsOpen();
    //待机模式,更新所有板卡状态
    void LowPowerMode();
    //重置当前任务状态
    void ResetTaskState();
    //查询当前通道是否正常
    void QueryCHState(uint8_t id, uint8_t ch);

signals:
    //自检信息
    void SelfInspection(PRE_SELF_INSPECTION *preselfinspection);
    //温度巡检
    void TemInspection(PRE_TEM_VALUE *tem_value);
    //综合巡检
    void ReturnVIBData(PRE_VIBRATION_DATA * vibration_data);
    //前置空闲状态
    void PreState(uint8_t preid,bool Isbusy);
    //更新的结果
    void UpdateResults(uint8_t preid,bool IsFail);
    //重启的状态
    void RebootState(uint8_t preid,bool IsAbnormal);
    //单通道状态检测
    void ReturnChannelStatus(uint8_t preid, uint8_t ch, bool IsAbnormal);

};

#endif // BUS1API_H
