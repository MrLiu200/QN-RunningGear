#ifndef APPSETTING_H
#define APPSETTING_H

#include <QSettings>
#include <QFile>
class APPSetting
{
public:
    static bool IsHardDiskReady;                    //外部硬盘是否已经准备就绪
    static QString configFile;                      //配置文件路径
    static QString dbfile;                          //数据库名称
    static QString version;                         //软件版本号 格式：主版本号(大变更,可能不兼容).次版本号(增删功能).补丁(修复bug)
    static QString WagonType;                       //车厢类型
    static QString projectName;                     //项目名称

    //车辆配置
    static int LineNumber;                          //线路号
    static QString CarNumber;                       //车号
    static QString WagonNumber;                     //车厢号
    static double WheelDiameter;                    //车轮直径 单位：米
    static double SpeedCoefficient;                 //转速系数

    //CAN总线配置
    static QString CanPlugin;                       //can插件名称
    static QString can0_Mode;                       //CAN0 模式 分为can和canfd
    static QString can0_Name;                       //can0 模块名称
    static QString can0_bitrate;                    //can0 仲裁域波特率
    static QString can0_bitrateSample;              //can0 仲裁域采样率
    static QString can0_dbitrate;                   //can0 数据域波特率
    static QString can0_dbitrateSample;             //can0 数据域采样率

    static QString can1_Mode;                       //CAN1 模式 分为can和canfd
    static QString can1_Name;                       //can1 模块名称
    static QString can1_bitrate;                    //can1 仲裁域波特率
    static QString can1_bitrateSample;              //can1 仲裁域采样率
    static QString can1_dbitrate;                   //can1 数据域波特率
    static QString can1_dbitrateSample;             //can1 数据域采样率

    static QString can2_Mode;                       //CAN2 模式 分为can和canfd
    static QString can2_Name;                       //can2 模块名称
    static QString can2_bitrate;                    //can2 仲裁域波特率
    static QString can2_bitrateSample;              //can2 仲裁域采样率
    static QString can2_dbitrate;                   //can2 数据域波特率
    static QString can2_dbitrateSample;             //can2 数据域采样率

    static int CANVibrateDataLen;                   //CAN总线上振动数据长度
    static int VibrateSamplingPoints;               //振动数据采样点数
    static int VibrateSamplingFrequency;            //振动数据采样频率

    //本地网络配置
    static QString LocalIP_eth0;                         //本地IP
    static QString LocalMask_eth0;                       //掩码
    static QString LocalGateway_eth0;                    //本机网关
    static QString LocalDNS_eth0;                        //dns
    static QString LocalIP_eth1;                         //eth1 IP
    static QString LocalMask_eth1;                       //eth1 掩码
    static QString LocalGateway_eth1;                    //eth1 网关
    static QString LocalDNS_eth1;                        //eth1 dns

    //TCP网络配置
    static QString TcpServerIP;                     //远程服务器ip
    static int TcpServerPort;                       //远程服务器端口
    static int TcpListenPort;                       //本地Tcp监听端口
#ifdef DEVICE_SLAVE
    //从机专属配置
    static QString TcpServerIPStandby;              //备用主机IP
    static int TcpServerPortStandby;                //备用主机port
#endif

    //udp 网络配置
    static QString UDPMulticastIP;                  //UDP组播IP
    static int UDPListenPort;                       //UDP监听端口
    static int HeartbeatInterval;                   //发送心跳间隔
    static int SynchronousInterval;                 //发送转速间隔

    //上位机 网络配置
    static int HMIListenPort;                       //上位机监听端口

    /*预警默认值配置*/
    static int baowaiWarning;        //保外预警
    static int baoneiWarning;        //保内预警
    static int waihuanWarning;       //外环预警
    static int neihuanWarning;       //内环预警
    static int gundanWarning;        //滚单预警
    static int gunshuangWarning;     //滚双预警
    static int treadWarning;         //踏面预警
    static int benzhouchilunWarning; //本轴齿轮预警
    static int linzhouchilunWarning; //邻轴齿轮预警

    /*一级报警默认值配置*/
    static int baowaiFirstLevelAlarm;   //保外一级报警
    static int baoneiFirstLevelAlarm;   //保内一级报警
    static int waihuanFirstLevelAlarm;  //外环一级报警
    static int neihuanFirstLevelAlarm;  //内环一级报警
    static int gundanFirstLevelAlarm;   //滚单一级报警
    static int gunshuangFirstLevelAlarm;//滚双一级报警
    static int treadFirstLevelAlarm;    //踏面一级报警
    static int benzhouchilunFirstLevelAlarm;  //本轴齿轮一级报警
    static int linzhouchilunFirstLevelAlarm;  //邻轴齿轮一级报警

    /*二级报警默认值配置*/
    static int baowaiSecondaryAlarm;   //保外二级报警
    static int baoneiSecondaryAlarm;   //保内二级报警
    static int waihuanSecondaryAlarm;  //外环二级报警
    static int neihuanSecondaryAlarm;  //内环二级报警
    static int gundanSecondaryAlarm;   //滚单二级报警
    static int gunshuangSecondaryAlarm;//滚双二级报警
    static int treadSecondaryAlarm;    //踏面一级报警
    static int benzhouchilunSecondaryAlarm;  //本轴齿轮二级报警
    static int linzhouchilunSecondaryAlarm;  //邻轴齿轮二级报警

    /*其它报警值配置*/
    static int AbsoluteTemperatureThreshold;    //绝对温度报警标准值
    static int RelativeTemperatureThreshold;    //相对温度报警标准值
    static int AbsoluteTemperatureWarning;      //温度预警值
    static int MemorySpaceRatio;                //内存空间比例标准

    /*量纲RMS一级报警配置(按测点类型)*/
    static double Axlebox_Rms_FirstLevelAlarm;  //轴箱测点rms值一级报警
    static double Gearbox_Rms_FirstLevelAlarm;  //齿轮箱测点rms值一级报警
    static double Motor_Rms_FirstLevelAlarm;    //电机测点rms值一级报警

    /*量纲RMS二级报警配置(按测点类型)*/
    static double Axlebox_Rms_SecondaryAlarm;   //轴箱测点rms值二级报警
    static double Gearbox_Rms_SecondaryAlarm;   //齿轮箱测点rms值二级报警
    static double Motor_Rms_SecondaryAlarm;     //电机测点rms值二级报警

    /*量纲PP一级报警配置(按测点类型)*/
    static double Axlebox_PP_FirstLevelAlarm;   //轴箱测点峰峰值一级报警
    static double Gearbox_PP_FirstLevelAlarm;   //齿轮箱测点峰峰值一级报警
    static double Motor_PP_FirstLevelAlarm;     //电机测点峰峰值一级报警

    /*量纲PP二级报警配置(按测点类型)*/
    static double Axlebox_PP_SecondaryAlarm;    //轴箱测点峰峰值二级报警
    static double Gearbox_PP_SecondaryAlarm;    //齿轮箱测点峰峰值二级报警
    static double Motor_PP_SecondaryAlarm;      //电机测点峰峰值二级报警

    //各种开关
    static bool UseDimensionalAlarm;            //是否启用量纲报警
    static bool UseVibrateWarning;              //是否启用振动预警
    static bool UseVibrateFirstLevelAlarm;      //是否启用振动一级报警
    static bool UseVibrateSecondaryAlarm;       //是否启用振动二级报警
    static bool UseTemperatureAlarm;            //是否启用温度报警
    static bool UseOfflineAlarm;                //是否启用设备掉线报警
    static bool UseExternalSpeed;               //是否启用外部转速

    //日志相关配置
    static int TmpTaskSpeedHourMax;                //温度巡检任务时速最大值
    static int LowPowerTaskSpeedHourMax;           //低功耗任务时速最大值
    static bool UseFileInterval;                //使用间隔存储
    static int VIBSaveInterval;                 //振动数据保存间隔
    static int TMPSaveInterval;                 //温度数据保存间隔
    static QString PackagingDate;               //自动打包日期

    //定时任务超时配置
    static int TimeoutInterval_Pre;           //前置处理器超时
    static int TimeoutInterval_Slave;         //从机心跳超时间隔
    static int TimeoutInterval_OtherBrade;    //背板其他板卡心跳超时间隔
    static int TimeoutInterval_UDPUpdateFile;    //UDP接收更新文件超时间隔

    //FTP设置
    static QString FtpDirName;

    //led灯设置
    static QString Led_Run;                     //运行指示灯
    static QString Led_PreError;                //前置错误指示灯
    static QString Led_alarm1;                  //一级报警指示灯
    static QString Led_alarm2;                  //二级报警指示灯

    //前置IO板卡供电设置
    static bool IOPowerEnable;                  //IO板卡电源是否使能

    //定时器间隔配置
    static int CheckCanInterval;                //定时检查前置状态间隔
    static int CheckSlaceInterval;              //定时检查从机状态间隔
    static int CheckOtherBoardInterval;         //定时检查背板状态间隔
    static int CheckOtherTaskInterval;          //定时检查其他任务间隔
    static int CheckPISStateInterval;           //定时检查PIS网口间隔
    static int CanBusTaskInterval;              //Can总线任务间隔
    static int CanBusUpdateTaskInterval;        //Can总线更新任务查询间隔
    static int TcpSendFileInterval;             //Tcp发送文件间隔
    static int WatchDogInterval;                //喂狗间隔

    static void ReadConfig();
    static void WriteConfig();
    static void newConfig();
    static bool checkConfig();
    static void UpdateNetworkconfig();

};

#endif // APPSETTING_H
