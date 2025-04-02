#include "udpmulticastapi.h"
#include <QMutex>
#include <QNetworkDatagram>
#include <QFile>
#include <QSqlError>
#include "corehelper.h"
#include "appsetting.h"
#include "m_databaseapi.h"
#include "logapi/datalog.h"
#include "taskapi.h"
#include <QNetworkInterface>
UDPMulticastAPI *UDPMulticastAPI::self = 0;
UDPMulticastAPI *UDPMulticastAPI::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if(!self){
            self = new UDPMulticastAPI;
        }
    }
    return self;
}

UDPMulticastAPI::UDPMulticastAPI(QObject *parent) : QThread(parent)
{
    IsOpen = false;
    HostOffline = false;
    m_ip = QHostAddress(APPSetting::UDPMulticastIP);
    udpServer = new QUdpSocket;
    Stopped = false;


    connect(udpServer,&QUdpSocket::readyRead,this,&UDPMulticastAPI::readData);

    timerHeartbeat = new QTimer;
    timerHeartbeat->setInterval(APPSetting::HeartbeatInterval);
    connect(timerHeartbeat,&QTimer::timeout,this,&UDPMulticastAPI::SendHeartbeat);

    timerSynchronous = new QTimer;
    timerSynchronous->setInterval(APPSetting::SynchronousInterval);
    connect(timerSynchronous,&QTimer::timeout,this,&UDPMulticastAPI::SendSynchronous);
    timerUpdate = new QTimer;
    timerUpdate->setInterval(APPSetting::TimeoutInterval_UDPUpdateFile);
    connect(timerUpdate,&QTimer::timeout,this,[=]{
        currentPage = 0;
        AllPage = 0;
        if(UpdateFile.isOpen()){
            UpdateFile.close();
        }
        timerUpdate->stop();
        HMIError(0x11);
    });

    connect(TaskAPI::Instance(),&TaskAPI::HostOffline,[=]{
        this->HostOffline = true;
    });
    connect(TaskAPI::Instance(),&TaskAPI::PreStateChange,this,&UDPMulticastAPI::PreStateChange);
    connect(TaskAPI::Instance(),&TaskAPI::OtherBoardStateChange,this,&UDPMulticastAPI::OtherBoardStateChange);
    connect(this,&UDPMulticastAPI::UDPSetTimerState,this,&UDPMulticastAPI::SetTimerState);

    currentPage = 0;
    AllPage = 0;
}

UDPMulticastAPI::~UDPMulticastAPI()
{
    Stop();
}
#if 1
void UDPMulticastAPI::WriteDataToList(const QByteArray data)
{
    QMutexLocker lock(&m_mutex);
    DataList.append(data);
}

qint64 UDPMulticastAPI::GetDataListSize()
{
    QMutexLocker lock(&m_mutex);
    return DataList.size();
}

QByteArray UDPMulticastAPI::ReadFirstDataFromList()
{
    QMutexLocker lock(&m_mutex);
    QByteArray returnArray;
    if(!DataList.isEmpty()){
        returnArray = DataList.takeFirst();
    }
    return returnArray;
}

void UDPMulticastAPI::run()
{
    while (!Stopped) {
        if(GetDataListSize() > 0){
            QByteArray buffer = ReadFirstDataFromList();
            CheckData(buffer);
        }
    }
}
#endif
bool UDPMulticastAPI::startJoin()
{
#if 0
     bool ret = udpServer->bind(QHostAddress::AnyIPv4,APPSetting::UDPListenPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
#else
    QHostAddress udpIPV4(APPSetting::LocalIP_eth0);
    QNetworkInterface netInterface = QNetworkInterface::interfaceFromName("eth0");
    bool ret = udpServer->bind(QHostAddress::AnyIPv4,APPSetting::UDPListenPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
#endif

    if(ret){
        udpServer->setMulticastInterface(netInterface);
        ret = udpServer->joinMulticastGroup(m_ip,netInterface);
        if(ret){
            udpServer->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
            IsOpen = true;
            //发送一次心跳
            SendHeartbeat();
#ifndef DEVICE_SLAVE
            //启动成功后，主机需要发送一次校时
            Time_Aligned();
            timerSynchronous->start();
#endif
            timerHeartbeat->start();
            QThread::start();
        }
    }
    return ret;
}

bool UDPMulticastAPI::Stop()
{
    if(timerHeartbeat->isActive()){
        timerHeartbeat->stop();
    }
    if(timerSynchronous->isActive()){
        timerSynchronous->stop();
    }
    bool ret = udpServer->leaveMulticastGroup(m_ip);
    if(ret){
        udpServer->abort();
        IsOpen = false;
    }
    Stopped = false;
    wait();
    quit();
    return ret;

}

void UDPMulticastAPI::senddata(const QByteArray data)
{
    if(IsOpen){
        qint64 len = udpServer->writeDatagram(data,m_ip,APPSetting::UDPListenPort);
        //        qDebug()<< "udp send len = "<< len << "data = " << CoreHelper::byteArrayToHexStr(data);
    }
}

void UDPMulticastAPI::readData()
{
    QNetworkDatagram datagrm = udpServer->receiveDatagram();
    QByteArray data = datagrm.data();
    WriteDataToList(data);

}

void UDPMulticastAPI::CheckData(const QByteArray data)
{
    int len = data.length();
    if(len == 0){
        return;
    }
//    qDebug()<< "udp Receive " << CoreHelper::byteArrayToHexStr(data);
    uint8_t frame[len];

    for(int i=0; i<len; i++){
        frame[i] = data[i];
    }

    //判断帧头帧尾若非主从机通信数据则不处理
    if((frame[0] == UDP_PACKET_HEAD1) && (frame[1] == UDP_PACKET_HEAD2) && (frame[len-1] == UDP_PACKET_END1) &&(frame[len-2] == UDP_PACKET_END2)){
        //校验和判断
        uint8_t oldcrc = frame[len-3];
        uint8_t newcrc = CoreHelper::CheckSum(frame,2,len-3);
        if(oldcrc == newcrc){
            uint8_t order = frame[2];
            uint16_t datalen = frame[3]<<8 | frame[4];
            QString DataContent = QString::fromUtf8(data.mid(5,datalen));
            switch (order) {
            case 0x01://校时
                updateTime(DataContent);
                break;
            case 0x02://自检
                SelfInspection(DataContent);
                break;
#ifndef DEVICE_SLAVE
            case 0x03://接收到自检结果
                ReceiveSelfInspection(DataContent);
                break;
            case 0x04://收到温度巡检结果
                TemInspection(DataContent);
                break;
            case 0x05://收到综合巡检结果
                ComprehensiveInspection(DataContent);
                break;
            case 0x06://收到心跳数据包
                HeartbeatEvent(DataContent);
                break;
#else
            case 0x07://收到更新转速信息
                UpdateSynchronous(DataContent);
                break;
#endif
#ifndef DEVICE_SLAVE
            case 0x08://设备状态发生改变
                UpdateBoardStatus(DataContent);
                break;
#endif
            case 0x11://更新指令
                UpdateSoftware(DataContent);
                break;
            case 0x12://设备重启
                DeviceReboot(DataContent);
                break;
            case 0x13://更新轴承信息
                UpdateBearing(DataContent);
                break;
            case 0x14://更新前置处理器
                UpdatePre(DataContent);
                break;
            case 0x15://更新定时任务
                UpdateTimerTask(DataContent);
                break;
            case 0x16://更新车辆信息
                UpdateCarInfo(DataContent);
                break;
            case 0x17://更新网络信息
                UpdateNetWorkConfing(DataContent);
                break;
            case 0x18://更新关联主从设备
                UpdateLinkDevice(DataContent);
                break;
            case 0x19://更新数据保存间隔
                UpdateSaveInterval(DataContent);
                break;
            case 0x1A://更新报警阈值
                UpdateAlarmLimit(DataContent);
                break;
            case 0x1B://删除指定文件
                UpdateZIPFile(DataContent);
                break;
#ifndef DEVICE_SLAVE
            case 0x1C://删除指定日期前量纲
                DeleteDimensionalValue(DataContent);
                break;
            case 0x1D://删除指定日期前日志
                DeleteLog(DataContent);
                break;
#endif
            case 0x1E://清除报警指示灯
                ClearAlarmLedState(DataContent);
                break;
            case 0x1F://测试新版添加设备
            {
                QStringList list = DataContent.split(";");
                if(list.size() < 2) return;
                if(list.at(0) == APPSetting::WagonNumber){
                    Q_EMIT TestAddPRE(list.at(1));
                }
            }

                break;
            case 0x80://读取前置设备状态
                GetPreStateList(DataContent);
                break;
            case 0x81://读取轴承信息
                GetBearingInfo(DataContent);
                break;
            case 0x82://读取定时任务
                GetTimerTask(DataContent);
                break;
            case 0x83://读取车辆信息
                GetCarConfig(DataContent);
                break;
            case 0x84://读取网络信息
                GetNetworkConfig(DataContent);
                break;
            case 0x85://读取关联设备
                GetLinkDeviceInfo(DataContent);
                break;
            case 0x86://读取数据保存间隔
                GetSaveInterval(DataContent);
                break;
            case 0x87://读取背板状态
                GetOtherBoardState(DataContent);
                break;
            case 0x88://读取报警阈值
                GetAlarmLimit(DataContent);
                break;
            case 0x89://读取软件版本
                GetAppVersion(DataContent);
                break;
            default:
                break;
            }
        }
    }
}

QByteArray UDPMulticastAPI::GetPackage(uint8_t order, QString data)
{

    QByteArray messageData = data.toUtf8();
    uint16_t len = messageData.size();
    QByteArray array;
    array.append(UDP_PACKET_HEAD1);
    array.append(UDP_PACKET_HEAD2);
    array.append(order);
    array.append((len >> 8) & 0xff);
    array.append(len & 0xff);
    array.append(messageData);
    len = array.size();
    array.append(CoreHelper::CheckSum(array,2,len));
    array.append(UDP_PACKET_END1);
    array.append(UDP_PACKET_END2);
    return array;
}

void UDPMulticastAPI::SendSelfInspection(QString Data)
{
    QByteArray array = GetPackage(0x03,Data);
    senddata(array);
}

void UDPMulticastAPI::SendTemInspection(QString Data)
{
    QByteArray array = GetPackage(0x04,Data);
    senddata(array);
}

void UDPMulticastAPI::SendComprehensiveInspection(QString Data)
{
    QByteArray array = GetPackage(0x05,Data);
    senddata(array);
}

void UDPMulticastAPI::UpdateFailed(uint8_t preid)
{
    HMIError(0x11,QString::number(preid));
}

void UDPMulticastAPI::updateTime(QString DataContent)
{
    //解析DataContent，与其他命令不同，这里用 ‘-’ 解析而非 ‘;’
    QStringList list = DataContent.split("-");
    if(list.size() != 6) return;
#ifdef Q_OS_WIN
    QProcess p(0);
    p.start("cmd");
    p.waitForStarted();
    p.write(QString("date %1-%2-%3\n").arg(list.at(0)).arg(list.at(1)).arg(list.at(2)).toLatin1());
    p.closeWriteChannel();
    p.waitForFinished(1000);
    p.close();
    p.start("cmd");
    p.waitForStarted();
    p.write(QString("time %1:%2:%3.00\n").arg(list.at(3)).arg(list.at(4)).arg(list.at(5)).toLatin1());
    p.closeWriteChannel();
    p.waitForFinished(1000);
    p.close();
#else
    QString cmd = QString("date %1%2%3%4%5.%6").arg(list.at(1)).arg(list.at(2)).arg(list.at(3)).arg(list.at(4)).arg(list.at(0)).arg(list.at(5));
    //    system(cmd.toLatin1());
    //    system("hwclock -w");
    // 创建 QProcess 实例
    QProcess *process = new QProcess();

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [process](int exitCode, QProcess::ExitStatus status){
        if (status == QProcess::NormalExit && exitCode == 0){
            QProcess *hwclockProcess = new QProcess();
            connect(hwclockProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), hwclockProcess, &QProcess::deleteLater);
            hwclockProcess->start("hwclock", QStringList() << "-w");
        }
        // 删除 process 对象，避免内存泄漏
        process->deleteLater();
    });

    // 启动日期命令
    process->start("bash", QStringList() << "-c" << cmd);

#endif
}

void UDPMulticastAPI::SelfInspection(QString DataContent)
{
    QStringList list = DataContent.split(";");
    QString Wagon = list.at(0);
    int pre_id = list.at(1).toInt();
    if(Wagon == "all" || Wagon == APPSetting::WagonNumber){
        //如果是全部或指定了本机需要自检，需要处理
        Q_EMIT Signal_SelfInspection(pre_id);
    }
}

void UDPMulticastAPI::ReceiveSelfInspection(QString DataContent)
{
    QStringList list = DataContent.split(";");
    QString WagonNum = list.at(0);
    QString time = list.at(1);
    uint8_t pre_id = list.at(2).toInt();
    uint8_t pre_state = list.at(3).toInt();
    uint8_t ChannelCount = list.at(4).toInt();
    if(ChannelCount <= 0) return;
    uint8_t ChannelState[ChannelCount];
    for(int i=5;i<list.size();i++){
        ChannelState[i-5] = list.at(i).toInt();
    }

    //之前是通过can2将状态更新到TRDP，现在还不知道要如何与车上交互，先暂留---LMG
    //如果报警，要更新到TRDP并且加入到日志中
    //判断前置主体状态
    if(pre_state == 1){
        //        M_DataBaseAPI::addLog(APPSetting::CarNumber,WagonNum,pre_id,0,"前置主体","自检信息",pre_state,time,"前置主体自检异常");
        Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,0,-1,"前置主体","自检信息",pre_state,time,"前置主体自检异常");
    }
    //判断各通道状态
    for(int i=0;i<ChannelCount;i++){
        if(ChannelState[i] == 1){
            //            M_DataBaseAPI::addLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,"无","自检信息",ChannelState[i],time,"该通道自检异常");
            Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,-1,"无","自检信息",ChannelState[i],time,"该通道自检异常");
        }
    }
}

void UDPMulticastAPI::TemInspection(QString DataContent)
{
    //取出对应数据
    QStringList list = DataContent.split(";");
    if(list.size() < 5) return;
    QString WagonNum = list.at(0);
    QString time = list.at(1);
    uint8_t pre_id = list.at(2).toInt();
    uint8_t ChannelCount = list.at(3).toInt();

    uint8_t ChannelState[ChannelCount];
    for(int i=4;i<list.size();i++){
        ChannelState[i-4] = list.at(i).toInt();
    }
    //这里应该更新TRDP或MVB等标志位---LMG
    //判断各通道状态
    for(int i=0;i<ChannelCount;i++){
        if(ChannelState[i] > 0){
            //            M_DataBaseAPI::addLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,"无","报警信息",ChannelState[i]-1,time,"通道温度异常");
            Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,-1,"无","报警信息",ChannelState[i]-1,time,"通道温度异常");
        }
    }
}

void UDPMulticastAPI::ComprehensiveInspection(QString DataContent)
{
    QStringList list = DataContent.split(";");
    if(list.size() < 15) return;
    QString WagonNum = list.at(0);
    QString time = list.at(1);
    uint8_t pre_id = list.at(2).toInt();
    uint8_t ChannelNum = list.at(3).toInt();
    QString devicename = list.at(4);
    int axis = list.at(5).toInt();
    uint32_t Speed = list.at(6).toInt();
    double AmbientTem = list.at(7).toDouble();
    double pointTem = list.at(8).toDouble();
    QStringList alarmlist = list.at(10).split("|");
    quint8 rmsAlarmgrade = list.at(11).toInt();
    quint8 ppAlarmgrade = list.at(12).toInt();
    QStringList DimensionList = list.at(13).split("|");
    QStringList DemodulatedList = list.at(14).split("|");
    uint8_t state[9];
    state[0] = list.at(9).toInt();
    for(int i=0;i<alarmlist.size();i++){
        state[i+1] = alarmlist.at(i).toInt();
    }

    //判断状态 -----LMG 未添加TRDP
    QStringList logList;
    QStringList Contents;
    int lastgrade = -1;
    Contents << "温度" << "保外" << "保内" << "外环" << "内环" << "滚单" << "大齿轮" << "小齿轮" << "踏面";
    for(int i=0;i<9;i++){
        if(state[i] != 0xff){//0:预警，1:一级报警 2:二级报警
//            QString name = WagonNum + "测点";
            //            M_DataBaseAPI::addLog(APPSetting::CarNumber,WagonNum,pre_id,ChannelNum,name,"报警信息",state[i]-1,time,Contents.at(i) + "异常");
            QString grade = (state[i] == 0 ? "预警" : QString("%1级").arg(state[i]));
            logList.append(Contents.at(i) + grade);
            if(lastgrade < state[i]){
                lastgrade = state[i];
            }
//            Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,ChannelNum,axis,name,"报警信息",state[i],time,Contents.at(i) + "异常");
        }
    }

    if(rmsAlarmgrade > 0){
        if(lastgrade < rmsAlarmgrade){
            lastgrade = rmsAlarmgrade;
        }
        logList.append(QString("RMS值 %1 级报警").arg(rmsAlarmgrade));
    }

    if(ppAlarmgrade > 0){
        if(lastgrade < ppAlarmgrade){
            lastgrade = ppAlarmgrade;
        }
        logList.append(QString("PP值 %1 级报警").arg(ppAlarmgrade));
    }

    if(!logList.isEmpty() && (lastgrade != -1)){
        Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,ChannelNum,axis,devicename,"报警信息",lastgrade,time,logList.join(";"));
    }
    //存储量纲
    QVector<float> DimensionValus;
    for(const QString str : DimensionList){
        DimensionValus.append(str.toFloat());
    }
    //    M_DataBaseAPI::AddDimensionalValue(WagonNum,pre_id,ChannelNum,Speed,time,DimensionValus);


    //存储包络量纲
    QVector<float> DemodulatedValus;
    for(const QString str : DemodulatedList){
        DemodulatedValus.append(str.toFloat());
    }
    //    M_DataBaseAPI::AddDemodulatedValue(WagonNum,pre_id,ChannelNum,Speed,time,DemodulatedValus);
    Q_EMIT UDPAddEigenvalue(WagonNum,pre_id,ChannelNum,Speed,AmbientTem,pointTem,time,DimensionValus,DemodulatedValus);

}

void UDPMulticastAPI::HeartbeatEvent(QString DataContent)
{//心跳只负责在线与否，不管是否报警
#ifndef DEVICE_SLAVE
    for(int i=0;i<DBData::SlaveStatus.size();i++){
        //数据格式：车厢号|车厢类型|当前状态|最后一个心跳时间
        QStringList list  = DBData::SlaveStatus.at(i).split("|");
        QString OldType = list.at(1);
        QString OldWagonNumber = list.at(0);
        int OldState = list.at(2).toInt();
        if(DataContent == OldWagonNumber){
            QString str = QString("%1|%2|0|%3").arg(OldWagonNumber).arg(OldType).arg(DATETIME);
            DBData::SlaveStatus[i] = str;
            if(OldState == DBData::DeviceState_Offline){//设备上线
                //                M_DataBaseAPI::addLog(APPSetting::CarNumber,OldWagonNumber,"报警信息",DATETIME,"设备上线");
                Q_EMIT UDPAddLog1(APPSetting::CarNumber,OldWagonNumber,"报警信息",DATETIME,"设备上线");
                if(OldType == "Host"){
                    HostOffline = false;
                }
            }
            break;
        }
    }
#endif
}

void UDPMulticastAPI::Time_Aligned()
{
#ifndef DEVICE_SLAVE
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss");
    QByteArray array = GetPackage(0x01,time);
    senddata(array);
#endif
}

void UDPMulticastAPI::StartSelfInspection(QString WagonName, QString preID)
{
#ifndef DEVICE_SLAVE
    QString data = WagonName + ";" + preID;
    QByteArray array = GetPackage(0x02,data);
    senddata(array);
#endif
}

void UDPMulticastAPI::SendHeartbeat()
{
    QByteArray array = GetPackage(0x06,APPSetting::WagonNumber);
    senddata(array);
}

void UDPMulticastAPI::SendSynchronous()
{
    bool SendSynchronous = APPSetting::WagonType == "Host" ||(APPSetting::WagonType == "StandbyHost" && HostOffline);
    if (SendSynchronous){
        quint32 speed = DBData::RunSpeed;
        QByteArray array = GetPackage(0x07,QString::number(speed));
        senddata(array);
    }
}

void UDPMulticastAPI::UpdateSynchronous(QString DataContent)
{
    if(!APPSetting::UseExternalSpeed) return;
    if (APPSetting::WagonType == "Slave") {
        DBData::RunSpeed = DataContent.toUInt();
        if(DBData::SpeedStruct.Bus0CanSave){
            DBData::SpeedStruct.Bus0SpeedVector.append(DBData::RunSpeed);
        }
        if(DBData::SpeedStruct.Bus1CanSave){
            DBData::SpeedStruct.Bus1SpeedVector.append(DBData::RunSpeed);
        }
    }
}

void UDPMulticastAPI::SendBoardStatus(QString info)
{
    QByteArray array = GetPackage(0x08,info);
    senddata(array);
}

void UDPMulticastAPI::UpdateBoardStatus(QString DataContent)
{
    QStringList list = DataContent.split(";");
    if(list.size() < 7) return;
    QString wagon = list.at(0);
    QString type = list.at(1);
    QString strid = list.at(2);
    QString devicename = list.at(3);
    int state = list.at(4).toInt();
    QString oldContent = list.at(5);
    QString Time = list.at(6);
    int id =0,ch =0,axis = -1;
    if(type == "前置"){
        QStringList infolist = strid.split("|");
        if(infolist.size() < 3) return;
        id = strid.split("|").at(0).toInt();
        ch = strid.split("|").at(1).toInt();
        axis = strid.split("|").at(2).toInt();
    }else{
        id = strid.toInt();
    }
//    QString newContent = oldContent + QString("状态: %1").arg(state);

    Q_EMIT UDPAddLog(APPSetting::CarNumber,wagon,id,ch,axis,devicename,"报警信息",0,Time,oldContent);
    //    M_DataBaseAPI::addLog(APPSetting::CarNumber,wagon,id,ch,type,"报警信息",0,Time,Content);
}

void UDPMulticastAPI::UpdateSoftware(QString info)
{
//    qDebug()<<"UDPMulticastAPI::UpdateSoftware";
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if((wagon != APPSetting::WagonNumber) && (wagon != "AllDevice")){
        return;
    }
    uint8_t type;
    QString Device = templist.at(1);
    if(Device.contains("|")){
        type = Device.split("|").at(0).toInt();
    }else{
        type = templist.at(1).toInt();
    }

    int current = templist.at(2).toInt();
    int all = templist.at(3).toInt();
    QString version = templist.at(4);
    uint32_t oldcrc = templist.at(5).toUInt();
    QByteArray data = QByteArray::fromBase64(templist.at(6).toUtf8());
    if(current == 1){//第一个包需要打开文件
        //完善文件名称
        QString updatedevice = "null";
        switch(type){
        case 0x01:
            updatedevice = "Pre";
            break;
        case 0x02:
            updatedevice = "Speed";
            break;
        case 0x03:
            updatedevice = "IO";
            break;
        case 0x04:
            updatedevice = "Communication";
            break;
        case 0x05:
            updatedevice = "Host";
            break;
        }

        QString filename = QString("%1/%2_%3.bin").arg(CoreHelper::APPPath()).arg(updatedevice).arg(version);
        UpdateFile.setFileName(filename);
        if(UpdateFile.exists()){
            UpdateFile.remove();
        }
        if(!UpdateFile.open(QIODevice::WriteOnly | QIODevice::Append)){
            qDebug()<<"更新指令-文件打开失败: " << filename;
            return;
        }
        Q_EMIT UDPSetTimerState(true);
    }else{
        if(!UpdateFile.isOpen()){
            Q_EMIT UDPSetTimerState(false);
            return;
        }
    }
    qDebug()<<"currentPage = " << current << "AllPage = " << all;
    QDataStream out(&UpdateFile);
    out.writeRawData(data.data(),data.length());
    currentPage = current;
    AllPage = all;

    if(current == all){//接收完成
        Q_EMIT UDPSetTimerState(false);
        QString filename = UpdateFile.fileName();
        UpdateFile.flush();
        UpdateFile.close();
        //验证crc
        quint32 newcrc = CoreHelper::GetCRC32_STM32H750(filename);

        qDebug()<<"newcrc " <<newcrc << "oldcrc "<< oldcrc;
        QStringList sendlist;
        sendlist.append(APPSetting::WagonNumber);
        if(newcrc != oldcrc){
            sendlist.append("1");
            QByteArray array = GetPackage(0x11,sendlist.join(";"));
            senddata(array);
        }else{
            sendlist.append("0");
            QByteArray array = GetPackage(0x11,sendlist.join(";"));
            senddata(array);
            switch (type) {
            case 0x01://前置处理器
                Q_EMIT PreSoftwareUpdate(filename,newcrc,Device.split("|").at(1).toInt());
                break;
            case 0x02://速度检测板卡
            case 0x03://前置IO板卡
            case 0x04://通信板卡
                Q_EMIT BackboardCardSoftwareUpdate(filename,newcrc,type);
                break;
            case 0x05://计算板卡
            {
                //                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"系统升级");
                Q_EMIT UDPAddLog1(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"系统升级");
                //更新软件版本
                APPSetting::version = version;
                APPSetting::WriteConfig();
                system(QString("mv %1 /v3/RailTrafficMonitoring_KM4").arg(filename).toLatin1());
                //                CoreHelper::sleep(1000);
                QThread::msleep(1000);
                system("chmod 777 /v3/RailTrafficMonitoring_KM4");
                CoreHelper::Reboot();
            }
                break;
            }
        }
    }
}

void UDPMulticastAPI::DeviceReboot(QString info)
{
    if((info != APPSetting::WagonNumber) && (info != "AllDevice")){
        return;
    }
//    CoreHelper::Reboot();
    Q_EMIT UDPReboot();
}

void UDPMulticastAPI::UpdateBearing(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 7){
        HMIError(0x13);
        return;
    }

    uint8_t order = templist.at(1).toInt();
    switch (order) {
    case 0x00://新增
        //        M_DataBaseAPI::AddBearingInfo(templist.at(2),templist.at(3).toFloat(),
        //                                      templist.at(4).toFloat(),templist.at(5).toFloat(),templist.at(6).toFloat());
        Q_EMIT UDPAddBearing(templist.at(2),templist.at(3).toFloat(),
                             templist.at(4).toFloat(),templist.at(5).toFloat(),templist.at(6).toFloat());
        break;
    case 0x01://修改
    {

        QString modelname = templist.at(2);
        float pitchdiameter = templist.at(3).toFloat();
        float rollingdiameter = templist.at(4).toFloat();
        int rollernum = templist.at(5).toInt();
        float contactangle  = templist.at(6).toFloat();
#if 1
        QString updateSql = QString("UPDATE BearingInfo SET"
                                    "PitchDiameter = '%1', RollingDiameter = '%2',"
                                    "RollerNum = '%3', ContactAngle = '%4' "
                                    "WHERE ModelName = '%5'").arg(pitchdiameter).arg(rollingdiameter).arg(rollernum).arg(contactangle).arg(modelname);
        Q_EMIT RunSQLStr(updateSql);
#else
        QString updateSql = QString("UPDATE BearingInfo SET"
                                    "PitchDiameter = :pitchdiameter, RollingDiameter = :rollingdiameter,"
                                    "RollerNum = :rollernum, ContactAngle = :contactangle "
                                    "WHERE ModelName = :modelname");
        QSqlQuery query;
        query.prepare(updateSql);
        query.bindValue(":pitchdiameter",pitchdiameter);
        query.bindValue(":rollingdiameter",rollingdiameter);
        query.bindValue(":rollernum",rollernum);
        query.bindValue(":contactangle",contactangle);
        query.bindValue(":modelname",modelname);
        if (!query.exec()) {
            qDebug() << "Update failed: " << query.lastError();
            HMIError(0x13);
        }
#endif
    }
        break;
    case 0x02://删除
        //        M_DataBaseAPI::DeleteBearingInfo(templist.at(2));
        Q_EMIT UDPDeleteBearing(templist.at(2));
        break;
    }
}

void UDPMulticastAPI::UpdatePre(QString info)
{
//    qDebug()<<"info = " << info;
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 23){
        HMIError(0x14);
        return;
    }

    uint8_t order = templist.at(1).toInt();
    switch (order) {
    case 0x00://新增
        //        M_DataBaseAPI::AddDeviceInfo(templist.at(2).toInt(),templist.at(3).toInt(),templist.at(4),templist.at(5),templist.at(6).toFloat(),templist.at(7).toFloat(),
        //                                     templist.at(8),templist.at(9),templist.at(10),templist.at(11),templist.at(12),templist.at(13),templist.at(14),templist.at(15),
        //                                     templist.at(16),templist.at(17).toInt(),templist.at(18),templist.at(19).toInt(),templist.at(20),templist.at(21).toInt());
        Q_EMIT UDPAddDevice(templist.at(2).toInt(),templist.at(3).toInt(),templist.at(4),templist.at(5),templist.at(6).toFloat(),templist.at(7).toInt(),templist.at(8).toFloat(),
                            templist.at(9),templist.at(10),templist.at(11),templist.at(12),templist.at(13),templist.at(14),templist.at(15),templist.at(16),
                            templist.at(17),templist.at(18).toInt(),templist.at(19),templist.at(20).toInt(),templist.at(21),templist.at(22).toInt());
        break;
    case 0x01://修改
    {
        int deviceId = templist.at(2).toInt();
        int deviceCh = templist.at(3).toInt();
        QString deviceName = templist.at(4);
        QString deviceType = templist.at(5);
        float deviceSensitivity = templist.at(6).toFloat();
        int AxisPosition = templist.at(7).toInt();
        float shaftDiameter = templist.at(8).toFloat();
        QString bearing1Name = templist.at(9);
        QString bearing1Model = templist.at(10);
        QString bearing2Name = templist.at(11);
        QString bearing2Model = templist.at(12);
        QString bearing3Name = templist.at(13);
        QString bearing3Model = templist.at(14);
        QString bearing4Name = templist.at(15);
        QString bearing4Model = templist.at(16);
        QString capstanName = templist.at(17);
        int capstanTeethNum = templist.at(18).toInt();
        QString drivenwheelName = templist.at(19);
        int drivenwheelTeethNum = templist.at(20).toInt();
        QString version = templist.at(21);
        bool isEnabled = templist.at(22).toInt() == 1;
#if 1
        QString updateSql = QString("UPDATE DeviceInfo SET "
                                    "DeviceName = '%1', DeviceType = '%2', DeviceSensitivity = '%3', AxisPosition = '%4'"
                                    "ShaftDiameter = '%5', bearing1Name = '%6', bearing1_model = '%7', "
                                    "bearing2Name = '%8', bearing2_model = '%9', bearing3Name = '%10', "
                                    "bearing3_model = '%11', bearing4Name = '%12', bearing4_model = '%13', "
                                    "capstanName = '%14', capstanTeethNum = '%15', "
                                    "DrivenwheelName = '%16', DrivenwheelTeethNum = '%17', "
                                    "version = '%18', IsEnable = '%19' "
                                    "WHERE DeviceID = '%20' AND DeviceCH = '%21'").arg(deviceName).arg(deviceType).arg(deviceSensitivity).arg(AxisPosition).arg(shaftDiameter)
                .arg(bearing1Name).arg(bearing1Model).arg(bearing2Name).arg(bearing2Model).arg(bearing3Name).arg(bearing3Model).arg(bearing4Name)
                .arg(bearing4Model).arg(capstanName).arg(capstanTeethNum).arg(drivenwheelName).arg(drivenwheelTeethNum).arg(version).arg(isEnabled)
                .arg(deviceId).arg(deviceCh);
        Q_EMIT RunSQLStr(updateSql);
#else
        QString updateSql = QString("UPDATE DeviceInfo SET "
                                    "DeviceName = :deviceName, DeviceType = :deviceType, DeviceSensitivity = :deviceSensitivity, "
                                    "ShaftDiameter = :shaftDiameter, bearing1Name = :bearing1Name, bearing1_model = :bearing1Model, "
                                    "bearing2Name = :bearing2Name, bearing2_model = :bearing2Model, bearing3Name = :bearing3Name, "
                                    "bearing3_model = :bearing3Model, bearing4Name = :bearing4Name, bearing4_model = :bearing4Model, "
                                    "capstanName = :capstanName, capstanTeethNum = :capstanTeethNum, "
                                    "DrivenwheelName = :drivenwheelName, DrivenwheelTeethNum = :drivenwheelTeethNum, "
                                    "version = :version, IsEnable = :isEnabled "
                                    "WHERE DeviceID = :deviceId AND DeviceCH = :deviceCh");

        QSqlQuery query;
        query.prepare(updateSql);

        // Bind values
        query.bindValue(":deviceName", deviceName);
        query.bindValue(":deviceType", deviceType);
        query.bindValue(":deviceSensitivity", deviceSensitivity);
        query.bindValue(":shaftDiameter", shaftDiameter);
        query.bindValue(":bearing1Name", bearing1Name);
        query.bindValue(":bearing1Model", bearing1Model);
        query.bindValue(":bearing2Name", bearing2Name);
        query.bindValue(":bearing2Model", bearing2Model);
        query.bindValue(":bearing3Name", bearing3Name);
        query.bindValue(":bearing3Model", bearing3Model);
        query.bindValue(":bearing4Name", bearing4Name);
        query.bindValue(":bearing4Model", bearing4Model);
        query.bindValue(":capstanName", capstanName);
        query.bindValue(":capstanTeethNum", capstanTeethNum);
        query.bindValue(":drivenwheelName", drivenwheelName);
        query.bindValue(":drivenwheelTeethNum", drivenwheelTeethNum);
        query.bindValue(":version", version);
        query.bindValue(":isEnabled", isEnabled);
        query.bindValue(":deviceId", deviceId);
        query.bindValue(":deviceCh", deviceCh);

        // Execute the query
        if (!query.exec()) {
            qDebug() << "Update failed: " << query.lastError();
            HMIError(0x14);
        }
#endif
    }
        break;
    case 0x02://删除
        //        M_DataBaseAPI::DeleteDeviceInfo(templist.at(2).toInt(),templist.at(3).toInt());
        Q_EMIT UDPDeleteDevice(templist.at(2).toInt(),templist.at(3).toInt());
        break;
    }
}

void UDPMulticastAPI::UpdateTimerTask(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 4){
        HMIError(0x15);
        return;
    }

    uint8_t order = templist.at(1).toInt();
    switch (order) {
    case 0x00://新增
        //        M_DataBaseAPI::AddTaskInfo(templist.at(2),templist.at(3));
        Q_EMIT UDPAddTask(templist.at(2),templist.at(3));
        break;
    case 0x01://修改
    {
        QString str = QString("update TimedTasksInfo set TriggerTime = '%1' where Action = '%2';")
                .arg(templist.at(3)).arg(templist.at(2));
#if 1
        Q_EMIT RunSQLStr(str);
#else
        QSqlQuery query;
        if (!query.exec()) {
            qDebug() << "Update failed: " << query.lastError();
            HMIError(0x15);
        }
#endif
    }
        break;
    case 0x02://删除
        //        M_DataBaseAPI::DeleteTasks(templist.at(2));
        Q_EMIT UDPDeleteTasks(templist.at(2));
        break;
    }
}

void UDPMulticastAPI::UpdateCarInfo(QString info)
{
    qDebug()<<"------------UpdateCarInfo--------------------------";
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 8){
        HMIError(0x16);
        return;
    }
    APPSetting::projectName = templist.at(1);
    APPSetting::LineNumber = templist.at(2).toInt();
    APPSetting::CarNumber = templist.at(3);
    APPSetting::WagonNumber = templist.at(4);
    APPSetting::WheelDiameter = templist.at(5).toDouble();
    APPSetting::WagonType = templist.at(6);
    APPSetting::SpeedCoefficient = templist.at(7).toDouble();
    APPSetting::WriteConfig();
}

void UDPMulticastAPI::UpdateNetWorkConfing(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 16){
        HMIError(0x17);
        return;
    }

    APPSetting::LocalIP_eth0 = templist.at(1);
    APPSetting::LocalMask_eth0 = templist.at(2);
    APPSetting::LocalGateway_eth0 = templist.at(3);
    APPSetting::LocalDNS_eth0 = templist.at(4);

    APPSetting::LocalIP_eth1 = templist.at(5);
    APPSetting::LocalMask_eth1 = templist.at(6);
    APPSetting::LocalGateway_eth1 = templist.at(7);
    APPSetting::LocalDNS_eth1 = templist.at(8);

    APPSetting::TcpServerIP = templist.at(9);
    APPSetting::TcpServerPort = templist.at(10).toInt();
    APPSetting::TcpListenPort = templist.at(11).toInt();
#ifdef DEVICE_SLAVE
    APPSetting::TcpServerIPStandby = templist.at(12);
    APPSetting::TcpServerPortStandby = templist.at(13).toInt();
#endif
    APPSetting::UDPMulticastIP = templist.at(14);
    APPSetting::UDPListenPort = templist.at(15).toInt();

    APPSetting::WriteConfig();
    APPSetting::UpdateNetworkconfig();
}

void UDPMulticastAPI::UpdateLinkDevice(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 6){
        HMIError(0x18);
        return;
    }

    uint8_t order = templist.at(1).toInt();
    switch (order) {
    case 0x00://新增
        //        M_DataBaseAPI::AddLinkDeviceInfo(templist.at(2),templist.at(3),templist.at(4).toInt(),templist.at(5));
        Q_EMIT UDPAddLinkDevice(templist.at(2),templist.at(3),templist.at(4).toInt(),templist.at(5));
        break;
    case 0x01://修改
    {
#if 1
        QString str = QString("UPDATE LinkDeviceInfo SET Type = '%1', Port = '%2', WagonNumber = '%3' WHERE IP = '%4'")
                .arg(templist.at(2)).arg(templist.at(4).toInt()).arg(templist.at(5)).arg(templist.at(3));
        Q_EMIT RunSQLStr(str);
#else
        QString str = "UPDATE LinkDeviceInfo SET Type = ?, Port = ?, WagonNumber = ? WHERE IP = ?";
        QSqlQuery query;
        query.prepare(str);
        query.bindValue(0, templist.at(2));  // Type
        query.bindValue(1, templist.at(4).toInt());  // Port
        query.bindValue(2, templist.at(5));  // WagonNumber
        query.bindValue(3, templist.at(3));  // IP

        if (!query.exec()) {
            qDebug() << "Update failed: " << query.lastError() << "sql str = " << str;
            HMIError(0x18);
        }
#endif
    }
        break;
    case 0x02://删除
        //        M_DataBaseAPI::DeleteLinkDeviceInfo(templist.at(3));
        Q_EMIT UDPDeleteLinkDevice(templist.at(3));
        break;
    }
}

void UDPMulticastAPI::UpdateSaveInterval(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 6){
        HMIError(0x19);
        return;
    }
    APPSetting::TmpTaskSpeedHourMax = templist.at(1).toInt();
    APPSetting::LowPowerTaskSpeedHourMax = templist.at(2).toInt();
    APPSetting::UseFileInterval = templist.at(3).toInt();
    APPSetting::VIBSaveInterval = templist.at(4).toInt();
    APPSetting::TMPSaveInterval = templist.at(5).toInt();

    APPSetting::WriteConfig();
}

void UDPMulticastAPI::UpdateAlarmLimit(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 50){//固定的50
        HMIError(0x1A);
        return;
    }
    APPSetting::baowaiWarning = templist.at(1).toInt();
    APPSetting::baoneiWarning = templist.at(2).toInt();
    APPSetting::waihuanWarning = templist.at(3).toInt();
    APPSetting::neihuanWarning = templist.at(4).toInt();
    APPSetting::gundanWarning = templist.at(5).toInt();
    APPSetting::gunshuangWarning = templist.at(6).toInt();
    APPSetting::treadWarning = templist.at(7).toInt();
    APPSetting::benzhouchilunWarning = templist.at(8).toInt();
    APPSetting::linzhouchilunWarning = templist.at(9).toInt();

    APPSetting::baowaiFirstLevelAlarm = templist.at(10).toInt();
    APPSetting::baoneiFirstLevelAlarm = templist.at(11).toInt();
    APPSetting::waihuanFirstLevelAlarm = templist.at(12).toInt();
    APPSetting::neihuanFirstLevelAlarm = templist.at(13).toInt();
    APPSetting::gundanFirstLevelAlarm = templist.at(14).toInt();
    APPSetting::gunshuangFirstLevelAlarm = templist.at(15).toInt();
    APPSetting::treadFirstLevelAlarm = templist.at(16).toInt();
    APPSetting::benzhouchilunFirstLevelAlarm = templist.at(17).toInt();
    APPSetting::linzhouchilunFirstLevelAlarm = templist.at(18).toInt();

    APPSetting::baowaiSecondaryAlarm = templist.at(19).toInt();
    APPSetting::baoneiSecondaryAlarm = templist.at(20).toInt();
    APPSetting::waihuanSecondaryAlarm = templist.at(21).toInt();
    APPSetting::neihuanSecondaryAlarm = templist.at(22).toInt();
    APPSetting::gundanSecondaryAlarm = templist.at(23).toInt();
    APPSetting::gunshuangSecondaryAlarm = templist.at(24).toInt();
    APPSetting::treadSecondaryAlarm = templist.at(25).toInt();
    APPSetting::benzhouchilunSecondaryAlarm = templist.at(26).toInt();
    APPSetting::linzhouchilunSecondaryAlarm = templist.at(27).toInt();

    APPSetting::AbsoluteTemperatureThreshold = templist.at(28).toInt();
    APPSetting::RelativeTemperatureThreshold = templist.at(29).toInt();
    APPSetting::AbsoluteTemperatureWarning = templist.at(30).toInt();

    APPSetting::Axlebox_Rms_FirstLevelAlarm = templist.at(31).toDouble();
    APPSetting::Gearbox_Rms_FirstLevelAlarm = templist.at(32).toDouble();
    APPSetting::Motor_Rms_FirstLevelAlarm = templist.at(33).toDouble();
    APPSetting::Axlebox_Rms_SecondaryAlarm = templist.at(34).toDouble();
    APPSetting::Gearbox_Rms_SecondaryAlarm = templist.at(35).toDouble();
    APPSetting::Motor_Rms_SecondaryAlarm = templist.at(36).toDouble();

    APPSetting::Axlebox_PP_FirstLevelAlarm = templist.at(37).toDouble();
    APPSetting::Gearbox_PP_FirstLevelAlarm = templist.at(38).toDouble();
    APPSetting::Motor_PP_FirstLevelAlarm = templist.at(39).toDouble();
    APPSetting::Axlebox_PP_SecondaryAlarm = templist.at(40).toDouble();
    APPSetting::Gearbox_PP_SecondaryAlarm = templist.at(41).toDouble();
    APPSetting::Motor_PP_SecondaryAlarm = templist.at(42).toDouble();

    APPSetting::UseDimensionalAlarm = templist.at(43).toInt();
    APPSetting::UseVibrateWarning = templist.at(44).toInt();
    APPSetting::UseVibrateFirstLevelAlarm = templist.at(45).toInt();
    APPSetting::UseVibrateSecondaryAlarm = templist.at(46).toInt();
    APPSetting::UseTemperatureAlarm = templist.at(47).toInt();
    APPSetting::UseOfflineAlarm = templist.at(48).toInt();
    APPSetting::UseExternalSpeed = templist.at(49).toInt();

    APPSetting::WriteConfig();
}

void UDPMulticastAPI::UpdateZIPFile(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    QString filename = templist.at(1);
    QString deletefile = APPSetting::FtpDirName + "/" + filename;
    QFile file(deletefile);
    if(file.exists()){
        file.remove();
    }
}

void UDPMulticastAPI::DeleteDimensionalValue(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    QString deadlinetime = templist.at(1);
    //判断time 是否有效
    QDateTime datetime = QDateTime::fromString(deadlinetime,"YYYY-MM-dd HH:mm:ss");
    if(datetime.isValid()){
        Q_EMIT UDPDeleteDimensional(deadlinetime);
    }

}

void UDPMulticastAPI::DeleteLog(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    QString deadlinetime = templist.at(1);
    //判断time 是否有效
    QDateTime datetime = QDateTime::fromString(deadlinetime,"YYYY-MM-dd HH:mm:ss");
    if(datetime.isValid()){
        QString sqlstr = QString("DELETE FROM LogInfo where TriggerTime < '%1'");
        Q_EMIT RunSQLStr(sqlstr);
    }
}

void UDPMulticastAPI::ClearAlarmLedState(QString info)
{
    QStringList templist = info.split(";");
    if(templist.size() < 3){
        return;
    }
    QString wagon = templist.at(0);
    qint8 id = templist.at(1).toInt();
    qint8 ch = templist.at(2).toInt();
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    Q_EMIT UDPClearAlarm(id,ch);

}

void UDPMulticastAPI::GetPreStateList(QString info)
{
    if((info != APPSetting::WagonNumber) && (info != "AllDevice")){
        return;
    }

    if(DBData::PreStatus.isEmpty()){
        HMIError(0x80);
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);

    list.append(DBData::PreStatus);

    QString str = list.join(";");
    QByteArray array = GetPackage(0x80,str);
    senddata(array);
}

void UDPMulticastAPI::GetBearingInfo(QString info)
{
    QStringList templist = info.split(";");
    if(templist.at(0) != APPSetting::WagonNumber){
        return;
    }
    QString model = templist.at(1);

    //查找轴承型号
    if(DBData::BearingInfo_Model.contains(model)){
        QVector<float> values = DBData::QueryBearingParameters(model);
        QStringList list;
        list.append(APPSetting::WagonNumber);

        list.append(model);
        for(int i=0;i<values.size();i++){
            list.append(QString::number(values.at(i)));
        }

        QString str = list.join(";");
        QByteArray array = GetPackage(0x81,str);
        senddata(array);
    }else{
        HMIError(0x81);
        return;
    }
}

void UDPMulticastAPI::GetTimerTask(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }

    int len = DBData::Task_Action.size();
    if(len == 0){
        HMIError(0x82);
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);


    for(int i=0;i<len;i++){
        QString temp = QString("%1|%2").arg(DBData::Task_Action.at(i)).arg(DBData::Task_TriggerTime.at(i));
        list.append(temp);
    }

    QString str = list.join(";");
    QByteArray array = GetPackage(0x82,str);
    senddata(array);
}

void UDPMulticastAPI::GetCarConfig(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append(APPSetting::projectName);
    list.append(QString::number(APPSetting::LineNumber));
    list.append(APPSetting::CarNumber);
    list.append(APPSetting::WagonNumber);
    list.append(QString::number(APPSetting::WheelDiameter));
    list.append(APPSetting::WagonType);
    list.append(QString::number(APPSetting::SpeedCoefficient));


    QString str = list.join(";");
    QByteArray array = GetPackage(0x83,str);
    senddata(array);
}

void UDPMulticastAPI::GetNetworkConfig(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);

    list.append(APPSetting::LocalIP_eth0);
    list.append(APPSetting::LocalMask_eth0);
    list.append(APPSetting::LocalGateway_eth0);
    list.append(APPSetting::LocalDNS_eth0);

    list.append(APPSetting::LocalIP_eth1);
    list.append(APPSetting::LocalMask_eth1);
    list.append(APPSetting::LocalGateway_eth1);
    list.append(APPSetting::LocalDNS_eth1);

    list.append(APPSetting::TcpServerIP);
    list.append(QString::number(APPSetting::TcpServerPort));
    list.append(QString::number(APPSetting::TcpListenPort));

    list.append(APPSetting::TcpServerIP);
    list.append(QString::number(APPSetting::TcpServerPort));

    list.append(APPSetting::UDPMulticastIP);
    list.append(QString::number(APPSetting::UDPListenPort));


    QString str = list.join(";");
    QByteArray array = GetPackage(0x84,str);
    senddata(array);
}

void UDPMulticastAPI::GetLinkDeviceInfo(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }
    if(DBData::LinkDeviceInfo_IP.isEmpty()){
        HMIError(0x85);
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);

    for(int i=0;i<DBData::LinkDeviceInfo_IP.size();i++){
        QStringList temp;
        temp.append(DBData::LinkDeviceInfo_Type.at(i));
        temp.append(DBData::LinkDeviceInfo_IP.at(i));
        temp.append(QString::number(DBData::LinkDeviceInfo_Port.at(i)));
        temp.append(DBData::LinkDeviceInfo_WagonNumber.at(i));
        list.append(temp.join("|"));
    }

    QString str = list.join(";");
    QByteArray array = GetPackage(0x85,str);
    senddata(array);
}

void UDPMulticastAPI::GetSaveInterval(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);

    list.append(QString::number(APPSetting::TmpTaskSpeedHourMax));
    list.append(QString::number(APPSetting::LowPowerTaskSpeedHourMax));
    list.append(QString::number(APPSetting::UseFileInterval));
    list.append(QString::number(APPSetting::VIBSaveInterval));
    list.append(QString::number(APPSetting::TMPSaveInterval));

    QString str = list.join(";");
    QByteArray array = GetPackage(0x86,str);
    senddata(array);
}

void UDPMulticastAPI::GetOtherBoardState(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }
    if(DBData::OtherBoardStatus.isEmpty()){
        HMIError(0x87);
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);
#ifdef DEVICE_SLAVE
    list.append("0|无板卡|无板卡");
#endif
    for(int i=0;i<DBData::OtherBoardStatus.size();i++){
        QStringList info = DBData::OtherBoardStatus.at(i).split("|");
        uint8_t id = info.at(0).toInt();
        QString state = info.at(1);
        QByteArray versionarr = Bus2API::Instance()->GetVersionMap(id);
        QString temp = state + "|" + QString(versionarr);
        QByteArray statearr = Bus2API::Instance()->GetStateMap(id);
        temp = temp + "|" + QString(statearr);
        list.append(temp);
    }
    QString str = list.join(";");
    QByteArray array = GetPackage(0x87,str);
    senddata(array);
}

void UDPMulticastAPI::GetAlarmLimit(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);
    //振动预警
    list.append(QString::number(APPSetting::baowaiWarning));
    list.append(QString::number(APPSetting::baoneiWarning));
    list.append(QString::number(APPSetting::waihuanWarning));
    list.append(QString::number(APPSetting::neihuanWarning));
    list.append(QString::number(APPSetting::gundanWarning));
    list.append(QString::number(APPSetting::gunshuangWarning));
    list.append(QString::number(APPSetting::treadWarning));
    list.append(QString::number(APPSetting::benzhouchilunWarning));
    list.append(QString::number(APPSetting::linzhouchilunWarning));
    //振动一级
    list.append(QString::number(APPSetting::baowaiFirstLevelAlarm));
    list.append(QString::number(APPSetting::baoneiFirstLevelAlarm));
    list.append(QString::number(APPSetting::waihuanFirstLevelAlarm));
    list.append(QString::number(APPSetting::neihuanFirstLevelAlarm));
    list.append(QString::number(APPSetting::gundanFirstLevelAlarm));
    list.append(QString::number(APPSetting::gunshuangFirstLevelAlarm));
    list.append(QString::number(APPSetting::treadFirstLevelAlarm));
    list.append(QString::number(APPSetting::benzhouchilunFirstLevelAlarm));
    list.append(QString::number(APPSetting::linzhouchilunFirstLevelAlarm));
    //振动二级
    list.append(QString::number(APPSetting::baowaiSecondaryAlarm));
    list.append(QString::number(APPSetting::baoneiSecondaryAlarm));
    list.append(QString::number(APPSetting::waihuanSecondaryAlarm));
    list.append(QString::number(APPSetting::neihuanSecondaryAlarm));
    list.append(QString::number(APPSetting::gundanSecondaryAlarm));
    list.append(QString::number(APPSetting::gunshuangSecondaryAlarm));
    list.append(QString::number(APPSetting::treadSecondaryAlarm));
    list.append(QString::number(APPSetting::benzhouchilunSecondaryAlarm));
    list.append(QString::number(APPSetting::linzhouchilunSecondaryAlarm));
    //温度
    list.append(QString::number(APPSetting::AbsoluteTemperatureThreshold));
    list.append(QString::number(APPSetting::RelativeTemperatureThreshold));
    list.append(QString::number(APPSetting::AbsoluteTemperatureWarning));
    //RMS
    list.append(QString::number(APPSetting::Axlebox_Rms_FirstLevelAlarm));
    list.append(QString::number(APPSetting::Gearbox_Rms_FirstLevelAlarm));
    list.append(QString::number(APPSetting::Motor_Rms_FirstLevelAlarm));
    list.append(QString::number(APPSetting::Axlebox_Rms_SecondaryAlarm));
    list.append(QString::number(APPSetting::Gearbox_Rms_SecondaryAlarm));
    list.append(QString::number(APPSetting::Motor_Rms_SecondaryAlarm));
    //PP
    list.append(QString::number(APPSetting::Axlebox_PP_FirstLevelAlarm));
    list.append(QString::number(APPSetting::Gearbox_PP_FirstLevelAlarm));
    list.append(QString::number(APPSetting::Motor_PP_FirstLevelAlarm));
    list.append(QString::number(APPSetting::Axlebox_PP_SecondaryAlarm));
    list.append(QString::number(APPSetting::Gearbox_PP_SecondaryAlarm));
    list.append(QString::number(APPSetting::Motor_PP_SecondaryAlarm));
    //报警开关
    list.append(QString::number(APPSetting::UseDimensionalAlarm));
    list.append(QString::number(APPSetting::UseVibrateWarning));
    list.append(QString::number(APPSetting::UseVibrateFirstLevelAlarm));
    list.append(QString::number(APPSetting::UseVibrateSecondaryAlarm));
    list.append(QString::number(APPSetting::UseTemperatureAlarm));
    list.append(QString::number(APPSetting::UseOfflineAlarm));
    list.append(QString::number(APPSetting::UseExternalSpeed));

    QString str = list.join(";");
    QByteArray array = GetPackage(0x88,str);
    senddata(array);

}

void UDPMulticastAPI::GetAppVersion(QString info)
{
    //判断车厢号，若不是本车厢则不处理
    if(info != APPSetting::WagonNumber){
        return;
    }

    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append(APPSetting::version);

    QString str = list.join(";");
    QByteArray array = GetPackage(0x89,str);
    senddata(array);
}

void UDPMulticastAPI::HMIError(uint8_t error_order, QString Content)
{
    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append(QString::number(error_order,16));
    if(!Content.isEmpty()){
        list.append(Content);
    }
    QString str = list.join(";");
    QByteArray array = GetPackage(0xFF,str);
    senddata(array);
}

void UDPMulticastAPI::PreStateChange(uint8_t id, uint8_t ch,  QString name, quint8 axis, DBData::DeviceState state)
{
    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append("前置");
    list.append(QString("%1|%2|%3").arg(id).arg(ch).arg(axis));
    list.append(name);
    list.append(QString::number(state));
    if(state == DBData::DeviceState_Offline){
        list.append("设备离线");
    }else if(state == DBData::DeviceState_Online){
        list.append("设备上线");
    }else{
        list.append("null");
    }
    list.append(DATETIME);
    SendBoardStatus(list.join(";"));
}

void UDPMulticastAPI::OtherBoardStateChange(uint8_t id, DBData::DeviceState state)
{
    QString devicename;
    if(id == COMMUNICATIONBOARDID){
        devicename = "通信板卡";
    }else if(id == SPEEDBOARDID){
        devicename = "转速板卡";
    }else if(id == PREIOBOARDID){
        devicename = "前置IO板卡";
    }
    QString statestr = "未知";
    if(state == DBData::DeviceState_Offline){
        statestr = "离线";
    }else if(state == DBData::DeviceState_Online){
        statestr = "上线";
    }
//    QString content = QString("背板设备(canid：%1,name: %2) %3").arg(id).arg(devicename).arg(statestr);
    QString content = QString("背板设备(canid：%1)%2").arg(id).arg(statestr);
    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append("OtherBoard");
    list.append(QString("%1").arg(id));
    list.append(devicename);
    list.append(QString::number(state));
    list.append(content);
    list.append(DATETIME);
    SendBoardStatus(list.join(";"));
}

void UDPMulticastAPI::SetTimerState(bool enable)
{
    if(enable && (!timerUpdate->isActive())){
        timerUpdate->start(APPSetting::TimeoutInterval_UDPUpdateFile);
    }else if(!enable && timerUpdate->isActive()){
        timerUpdate->stop();
    }
}
