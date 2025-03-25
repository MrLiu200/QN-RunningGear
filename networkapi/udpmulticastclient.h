/*
 * 类名:udp组播客户端
 * 作者:Mr.Liu
 * 时间:2024-05-24
 * 说明:通过udp组播实现服务器与多台客户端的交互，包括但不限于日常数据交互、软件升级
 * 弊端: 由于该项目无需实现udp发送超大文件，故此类不支持超大文件的传输
*/

#ifndef UDPMULTICASTCLIENT_H
#define UDPMULTICASTCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QProcess>
#include <QTimer>
#include <QFile>
#define UDP_PACKET_HEAD1 0xA6
#define UDP_PACKET_HEAD2 0xA6
#define UDP_PACKET_END1  0x6A
#define UDP_PACKET_END2  0x6A

class UDPMulticastClient : public QObject
{
    Q_OBJECT
public:
    static UDPMulticastClient *Instance();
    explicit UDPMulticastClient(QObject *parent = nullptr);
private:
    static UDPMulticastClient *self;
    QUdpSocket *udpServer;
    QHostAddress m_ip;
    bool IsOpen;
    QTimer *timerHeartbeat;
    QTimer *timerUpdate;
    int currentPage;
    int AllPage;
    QFile UpdateFile;

private slots:
    void readData();
    void CheckData(const QByteArray array);
    //更新系统时间
    void updateTime(QString DataContent);
    //自检
    void SelfInspection(QString DataContent);
    //发送数据
    void senddata(const QByteArray data);
    //生成发送数据包
    QByteArray GetPackage(uint8_t order, QString data);

public slots:
    bool start();
    bool stop();
    //发送自检数据
    void SendSelfInspection(QString Data);
    //发送温度巡检结果
    void SendTemInspection(QString Data);
    //发送综合巡检结果
    void SendComprehensiveInspection(QString Data);
    //发送心跳
    void SendHeartbeat();

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

    //错误命令
    void HMIError(uint8_t error_order);


signals:
    void PreSoftwareUpdate(const QString filename,const uint32_t crc);//前置处理器更新
    void BackboardCardSoftwareUpdate(const QString filename,const uint32_t crc,uint8_t devicetype);//背板板卡更新

signals:
    void Signal_SelfInspection(uint8_t pre_id);               //自检信号 ,id == 0 表示所有设备均需自检，否则为指定前置自检
};

#endif // UDPMULTICASTCLIENT_H
