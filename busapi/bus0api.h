#ifndef BUS0API_H
#define BUS0API_H

#include <QObject>
#include <QMutex>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusDeviceInfo>
#include <QCanBusFrame>
#include <QDebug>
#include "appsetting.h"
#include "m_databaseapi.h"
#include "corehelper.h"
#include "dbdata.h"
#include "algorithm_v2.h"
#include "datalog.h"
#include "taskapi.h"

enum TASKSTATE{
    idle = 0,               //空闲
    Collecting,             //正在采集
    WaitingToObtain,        //等待获取
    Obtaining,              //获取中
    WaitingSwitch           //等待切换
};

#define PRE_PACKET_DATA_OFFSET 3
typedef struct PRE_SELF_INSPECTION{//自检信息结构体
    uint8_t id;                                     //前置处理器的ID
    uint8_t PreStatus;                              //前置处理器状态
    uint8_t CH_Count;                               //通道数量
    QVector<uint8_t> CH_Status;                     //通道状态(因通道可增加，这里做成可调整的长度)
    uint16_t version;                               //软件版本
}PRE_SELF_INSPECTION;

typedef struct PRE_TEM_VALUE{//温度信息结构体
    uint8_t id;                                     //前置处理器ID
    double AmbientTem;                              //环境温度
    uint8_t CH_Count;                               //通道数量
    QVector<double> PointTem;                       //测点温度
    QDateTime TriggerTime;                          //触发时间
}PRE_TEM_VALUE;

typedef struct PRE_VIBRATION_DATA{//综合巡检信息结构体
    uint8_t id;                                     //前置处理器ID
    uint8_t ch;                                     //数据所在通道
    QString name;                                   //测点名称
    quint8 AxisPosition;                            //所在轴位
    uint32_t speedAve;                              //平均转速
    quint8 RMSAlarmGrade;                           //rms值报警状态
    quint8 PPAlarmGrade;                            //pp值报警状态
    QDateTime TriggerTime;                          //触发时间
    double AmbientTemValue;                         //环境温度
    double PointTemValue;                           //测点温度
    QString AlarmResult;                            //诊断结果 保外|保内|外环|内环|滚单|大齿轮|小齿轮|踏面
    QString DimensionStr;                           //量纲值字符串
    QString DemodulatedStr;                         //包络量纲字符串
    QVector<uint32_t> SpeedVector;                  //采集过程中的转速
}PRE_VIBRATION_DATA;

typedef struct TEMPERATURESTRUCT{//温度链表
    double value;
    qint8 ch;
    qint8 id;
}TEMPERATURESTRUCT;

typedef struct TASK_CHANNEL_LIST{//在线的通道列表
    uint8_t Pre_id;//前置处理器ID
    TASKSTATE status;//0-空闲 1-正在采集  2-等待读取 3-正在读取 4-读取完成
    uint8_t working_ch;//0-没有在工作,其他数值代表通道号
    int working_index;//正在工作通道的索引
    QList<uint8_t> channellist;//通道号列表
}TASK_CHANNEL_LIST;

typedef struct TASK_PRE_LIST{//在线的前置列表
    QList<TASK_CHANNEL_LIST> prelist;
    uint8_t working_pre;//0-没有在工作，其他数值代表前置ID
    int working_index;//工作中的前置索引
}TASK_PRE_LIST;

class Bus0API : public QObject
{
    Q_OBJECT
public:
    static Bus0API *Instance();
    explicit Bus0API(QObject *parent = nullptr);

    bool Open();
private:
    static Bus0API *self;
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
    QList<int> Bus0PreList;


private slots:
    //处理接收到的帧
    void processReceivedFrames();
    //帧错误
    void processErrors(QCanBusDevice::CanBusError error) const;
    //解析数据
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
    void UpdateDeviceState(uint8_t preid, uint8_t prech,bool IsFault = false);
    //检查量纲是否需要报警
    void CheckDimensionState(quint8 id, quint8 ch, quint8 axis, int type, float rmsvalue, float ppvalue);
    //检查RMS的报警状态，返回值：0-未报警，1：一级报警，2：二级报警
    quint8 CheckRMSState(int type, float rmsvalue);
    //检查PP的报警状态，返回值：0-未报警，1：一级报警，2：二级报警
    quint8 CheckPPState(int type,float ppvalue);

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
    //重置所有任务状态
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

#endif // BUS0API_H
