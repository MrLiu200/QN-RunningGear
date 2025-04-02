#ifndef DBDATA_H
#define DBDATA_H

#include <QObject>
#include <QMutex>
#include <QSqlDatabase>
//转速结构体
struct SPEEDSTRUCT{
    bool Bus0CanSave;
    QVector<uint32_t> Bus0SpeedVector;
    bool Bus1CanSave;
    QVector<uint32_t> Bus1SpeedVector;
};
class DBData : public QObject
{
    Q_OBJECT
public:
    enum DeviceState{
        DeviceState_Online = 0,
        DeviceState_Offline,
        DeviceState_Waring,
        DeviceState_FirstAlarm,
        DeviceState_Secondary,
        DeviceState_Error
    };



    static SPEEDSTRUCT SpeedStruct;
    //前置设备状态，用于判断是否在线以及是否报警
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    //状态：0-在线 1-离线
    static QList<QString> PreStatus;

#ifndef DEVICE_SLAVE
    //从机设备状态,只用于主机。用来判断从机的在线状态
    //数据格式：车厢号|车厢类型|当前状态|最后一个心跳时间
    //状态：0-在线 1-离线
    static QList<QString> SlaveStatus;
#endif
    //背板设备状态，用于判断是否在线
    //数据格式：板卡帧ID|当前状态|最后一次心跳时间
    //状态：0-在线 1-离线
    static QList<QString> OtherBoardStatus;

    //定时任务链表
    static QList<QString> Task_Action;
    static QList<QString> Task_TriggerTime;

    //轴承信息列表
    static QList<QString> BearingInfo_Model;                //型号
    static QList<float> BearingInfo_PitchDiameter;          //中径
    static QList<float> BearingInfo_RollingDiameter;        //滚径
    static QList<int> BearingInfo_RollerNum;                //滚子数
    static QList<float> BearingInfo_ContactAngle;           //接触角

    //查询轴承参数
    static QVector<float> QueryBearingParameters(QString model);

    //设备信息链表
    static int DeviceInfo_Count;
    static QList<int> DeviceInfo_DeviceID;                  //前置ID
    static QList<int> DeviceInfo_DeviceCH;                  //通道
    static QList<QString> DeviceInfo_DeviceName;            //名称
    static QList<QString> DeviceInfo_DeviceType;            //测点类型
    static QList<float> DeviceInfo_Sensitivity;             //测点灵敏度
    static QList<int> DeviceInfo_AxisPosition;              //轴位
    static QList<float> DeviceInfo_ShaftDiameter;           //测点所在轴轴径

    static QList<QString> DeviceInfo_bearing1Name;          //轴承1名称
    static QList<QString> DeviceInfo_bearing1_model;        //轴承1型号
    static QList<QString> DeviceInfo_bearing2Name;          //轴承2名称
    static QList<QString> DeviceInfo_bearing2_model;        //轴承2型号
    static QList<QString> DeviceInfo_bearing3Name;          //轴承3名称
    static QList<QString> DeviceInfo_bearing3_model;        //轴承3型号
    static QList<QString> DeviceInfo_bearing4Name;          //轴承4名称
    static QList<QString> DeviceInfo_bearing4_model;        //轴承4型号

    static QList<QString> DeviceInfo_capstanName;           //主动轮名称
    static QList<int> DeviceInfo_capstanTeethNum;           //主动轮齿数
    static QList<QString> DeviceInfo_DrivenwheelName;       //从动轮名称
    static QList<int> DeviceInfo_DrivenwheelTeethNum;       //从动轮齿数
    static QList<QString> DeviceInfo_version;               //设备版本
    static QList<bool> DeviceInfo_IsEnable;                 //设备使能
    static int GetDeviceIndex(int Preid, int PreCH);

    //振动数据名称链表
    static QList<QString> TcpSendList;                     //Tcp需要发送的文件名称
    static QMutex mutex;                                    //静态锁，用来保护线程安全
    //链表添加内容
    static void TCPAddSendFile(QString filename);
    //链表删除内容
    static void TCPDeleteSendFile(int index);

    //关联设备列表
    static QList<QString> LinkDeviceInfo_Type;              //关联设备类型
    static QList<QString> LinkDeviceInfo_IP;                //关联设备IP
    static QList<int> LinkDeviceInfo_Port;                  //关联设备端口
    static QList<QString> LinkDeviceInfo_WagonNumber;       //关联设备车厢号

    static int vibfilecount;                                //振动数据存储计数
    static int tmpfilecount;                                //温度数据存储计数
    static uint32_t RunSpeed;                               //列车当前速度，实时更新 单位： RPM

    //算法所用参数
    // 轴承。内容:保外、保内、外环、内环、滚单
//    static QList<int> ZhouLimit_Warning;                   //报警阈值预警
    static QList<int> ZhouLimit_First;                     //报警阈值一级
    static QList<int> ZhouLimit_Secondary;                 //报警阈值二级
    //踏面、本轴、邻轴
//    static QList<int> OtherLimit_Warning;                   //其他报警阈值预警
    static QList<int> OtherLimit_First;                     //其他报警阈值预警
    static QList<int> OtherLimit_Secondary;                 //其他报警阈值预警

    static qint8 CurrentAlarmLevel;                        //当前报警等级，若小于此报警等级，则不更新指示灯
    static bool PreIsAlarm;                                 //前置是否已经报警，用于指示灯
//    static bool IOPowerEnable;                              //前置IO板卡电源标志位,上电后默认是打开的



signals:

};

#endif // DBDATA_H
