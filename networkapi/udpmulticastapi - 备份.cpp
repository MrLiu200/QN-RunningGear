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

UDPMulticastAPI::UDPMulticastAPI(QObject *parent) : QObject(parent)
{
    IsOpen = false;
    HostOffline = false;
    m_ip = QHostAddress(APPSetting::UDPMulticastIP);
    udpServer = new QUdpSocket;
//    Stopped = false;


    connect(udpServer,&QUdpSocket::readyRead,this,&UDPMulticastAPI::readData);

    timerHeartbeat = new QTimer;
    timerHeartbeat->setInterval(APPSetting::HeartbeatInterval);
    connect(timerHeartbeat,&QTimer::timeout,this,&UDPMulticastAPI::SendHeartbeat);

    timerSynchronous = new QTimer;
    timerSynchronous->setInterval(APPSetting::SynchronousInterval);
    connect(timerSynchronous,&QTimer::timeout,this,&UDPMulticastAPI::SendSynchronous);
    timerUpdate = new QTimer;
    timerUpdate->setInterval(180000);
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

    currentPage = 0;
    AllPage = 0;
}
#if 0
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
    bool ret = udpServer->bind(QHostAddress::AnyIPv4,APPSetting::UDPListenPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    if(ret){
        ret = udpServer->joinMulticastGroup(m_ip);
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
//            QSqlDatabase dbThread = QSqlDatabase::cloneDatabase(dbConn, "ThreadConnection");
//            dbThread.setDatabaseName(APPSetting::dbfile);
//            bool ok = dbThread.open();
//            QThread::start();
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
    CheckData(datagrm.data());
//    QByteArray data = datagrm.data();
//    WriteDataToList(data);
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
        Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,0,"前置主体","自检信息",pre_state,time,"前置主体自检异常");
    }
    //判断各通道状态
    for(int i=0;i<ChannelCount;i++){
        if(ChannelState[i] == 1){
//            M_DataBaseAPI::addLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,"无","自检信息",ChannelState[i],time,"该通道自检异常");
            Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,"无","自检信息",ChannelState[i],time,"该通道自检异常");
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
            Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,i+1,"无","报警信息",ChannelState[i]-1,time,"通道温度异常");
        }
    }
}

void UDPMulticastAPI::ComprehensiveInspection(QString DataContent)
{
    QStringList list = DataContent.split(";");
    if(list.size() < 9) return;
    QString WagonNum = list.at(0);
    QString time = list.at(1);
    uint8_t pre_id = list.at(2).toInt();
    uint8_t ChannelNum = list.at(3).toInt();
    uint32_t Speed = list.at(4).toInt();
    QStringList alarmlist = list.at(6).split("|");
    QStringList DimensionList = list.at(7).split("|");
    QStringList DemodulatedList = list.at(8).split("|");
    uint8_t state[9];
    state[0] = list.at(5).toInt();
    for(int i=0;i<alarmlist.size();i++){
        state[i+1] = alarmlist.at(i).toInt();
    }

    //判断状态 -----LMG 未添加TRDP
    QStringList Contents;
    Contents << "温度" << "保外" << "保内" << "外环" << "内环" << "滚单" << "大齿轮" << "小齿轮" << "踏面";
    for(int i=0;i<9;i++){
        if(state[i] != 0xff){//0:预警，1:一级报警 2:二级报警
            QString name = WagonNum + "测点";
//            M_DataBaseAPI::addLog(APPSetting::CarNumber,WagonNum,pre_id,ChannelNum,name,"报警信息",state[i]-1,time,Contents.at(i) + "异常");
            Q_EMIT UDPAddLog(APPSetting::CarNumber,WagonNum,pre_id,ChannelNum,name,"报警信息",state[i]-1,time,Contents.at(i) + "异常");
        }
    }

    //存储量纲
    QVector<float> DimensionValus;
    for(const QString str : DimensionList){
        DimensionValus.append(str.toFloat());
    }
//    M_DataBaseAPI::AddDimensionalValue(WagonNum,pre_id,ChannelNum,Speed,time,DimensionValus);
    Q_EMIT UDPAddDimensional(WagonNum,pre_id,ChannelNum,Speed,time,DimensionValus);

    //存储包络量纲
    QVector<float> DemodulatedValus;
    for(const QString str : DemodulatedList){
        DemodulatedValus.append(str.toFloat());
    }
//    M_DataBaseAPI::AddDemodulatedValue(WagonNum,pre_id,ChannelNum,Speed,time,DemodulatedValus);
    Q_EMIT UDPAddDemodulated(WagonNum,pre_id,ChannelNum,Speed,time,DemodulatedValus);
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
                Q_EMIT UDPAddLog(APPSetting::CarNumber,OldWagonNumber,"报警信息",DATETIME,"设备上线");
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
    QString wagon = list.at(0);
    QString type = list.at(1);
    QString strid = list.at(2);
    int state = list.at(3).toInt();
    QString Time = list.at(4);
    int id =0,ch =0;
    if(type == "前置"){
        id = strid.split("|").at(0).toInt();
        ch = strid.split("|").at(1).toInt();
    }else{
        id = strid.toInt();
    }
    QString Content = QString("%1").arg(state == 0 ? "设备上线" : "设备离线");

    Q_EMIT UDPAddLog(APPSetting::CarNumber,wagon,id,ch,type,"报警信息",0,Time,Content);
//    M_DataBaseAPI::addLog(APPSetting::CarNumber,wagon,id,ch,type,"报警信息",0,Time,Content);
}

void UDPMulticastAPI::UpdateSoftware(QString info)
{
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
        QString filename = QString("%1/%2.bin").arg(CoreHelper::APPPath()).arg(version);
        UpdateFile.setFileName(filename);
        if(UpdateFile.exists()){
            UpdateFile.remove();
        }
        if(!UpdateFile.open(QIODevice::WriteOnly | QIODevice::Append)){
            qDebug()<<"更新指令-文件打开失败: " << filename;
            return;
        }
        timerUpdate->setInterval(180000);
        timerUpdate->start();
    }else{
        if(!UpdateFile.isOpen()){
            HMIError(0x11);
            return;
        }
    }
//    qDebug()<<"currentPage = " << current << "AllPage = " << all;
    QDataStream out(&UpdateFile);
    out.writeRawData(data.data(),data.length());
    currentPage = current;
    AllPage = all;

    if(current == all){//接收完成
        QString filename = UpdateFile.fileName();
        timerUpdate->stop();
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
                Q_EMIT UDPAddLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"系统升级");
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
    CoreHelper::Reboot();
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
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 22){
        HMIError(0x14);
        return;
    }

    uint8_t order = templist.at(1).toInt();
    switch (order) {
    case 0x00://新增
//        M_DataBaseAPI::AddDeviceInfo(templist.at(2).toInt(),templist.at(3).toInt(),templist.at(4),templist.at(5),templist.at(6).toFloat(),templist.at(7).toFloat(),
//                                     templist.at(8),templist.at(9),templist.at(10),templist.at(11),templist.at(12),templist.at(13),templist.at(14),templist.at(15),
//                                     templist.at(16),templist.at(17).toInt(),templist.at(18),templist.at(19).toInt(),templist.at(20),templist.at(21).toInt());
        Q_EMIT UDPAddDevice(templist.at(2).toInt(),templist.at(3).toInt(),templist.at(4),templist.at(5),templist.at(6).toFloat(),templist.at(7).toFloat(),
                            templist.at(8),templist.at(9),templist.at(10),templist.at(11),templist.at(12),templist.at(13),templist.at(14),templist.at(15),
                            templist.at(16),templist.at(17).toInt(),templist.at(18),templist.at(19).toInt(),templist.at(20),templist.at(21).toInt());
        break;
    case 0x01://修改
    {
        int deviceId = templist.at(2).toInt();
        int deviceCh = templist.at(3).toInt();
        QString deviceName = templist.at(4);
        QString deviceType = templist.at(5);
        float deviceSensitivity = templist.at(6).toFloat();
        float shaftDiameter = templist.at(7).toFloat();
        QString bearing1Name = templist.at(8);
        QString bearing1Model = templist.at(9);
        QString bearing2Name = templist.at(10);
        QString bearing2Model = templist.at(11);
        QString bearing3Name = templist.at(12);
        QString bearing3Model = templist.at(13);
        QString bearing4Name = templist.at(14);
        QString bearing4Model = templist.at(15);
        QString capstanName = templist.at(16);
        int capstanTeethNum = templist.at(17).toInt();
        QString drivenwheelName = templist.at(18);
        int drivenwheelTeethNum = templist.at(19).toInt();
        QString version = templist.at(20);
        bool isEnabled = templist.at(21).toInt() == 1;

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
        QSqlQuery query;
        if (!query.exec()) {
            qDebug() << "Update failed: " << query.lastError();
            HMIError(0x15);
        }
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
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 5){
        HMIError(0x16);
        return;
    }

    APPSetting::LineNumber = templist.at(1).toInt();
    APPSetting::CarNumber = templist.at(2);
    APPSetting::WagonNumber = templist.at(3);
    APPSetting::WheelDiameter = templist.at(4).toDouble();

    APPSetting::WriteConfig();
}

void UDPMulticastAPI::UpdateNetWorkConfing(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if(wagon != APPSetting::WagonNumber){
        return;
    }
    if(templist.size() != 12){
        HMIError(0x17);
        return;
    }

    APPSetting::LocalIP = templist.at(1);
    APPSetting::LocalMask = templist.at(2);
    APPSetting::LocalGateway = templist.at(3);
    APPSetting::LocalDNS = templist.at(4);

    APPSetting::TcpServerIP = templist.at(5);
    APPSetting::TcpServerPort = templist.at(6).toInt();
    APPSetting::TcpListenPort = templist.at(7).toInt();
#ifdef DEVICE_SLAVE
    APPSetting::TcpServerIPStandby = templist.at(8);
    APPSetting::TcpServerPortStandby = templist.at(9).toInt();
#endif
    APPSetting::UDPMulticastIP = templist.at(10);
    APPSetting::UDPListenPort = templist.at(11).toInt();

    APPSetting::WriteConfig();
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
    if(templist.size() != 28){//固定的28
        HMIError(0x1A);
        return;
    }
    APPSetting::baowaiWarning = templist.at(1).toInt();
    APPSetting::baoneiWarning = templist.at(2).toInt();
    APPSetting::waihuanWarning = templist.at(3).toInt();
    APPSetting::neihuanWarning = templist.at(4).toInt();
    APPSetting::gundanWarning = templist.at(5).toInt();
    APPSetting::gunshuangWarning = templist.at(6).toInt();
    APPSetting::benzhouchilunWarning = templist.at(7).toInt();
    APPSetting::linzhouchilunWarning = templist.at(8).toInt();

    APPSetting::baowaiFirstLevelAlarm = templist.at(9).toInt();
    APPSetting::baoneiFirstLevelAlarm = templist.at(10).toInt();
    APPSetting::waihuanFirstLevelAlarm = templist.at(11).toInt();
    APPSetting::neihuanFirstLevelAlarm = templist.at(12).toInt();
    APPSetting::gundanFirstLevelAlarm = templist.at(13).toInt();
    APPSetting::gunshuangFirstLevelAlarm = templist.at(14).toInt();
    APPSetting::benzhouchilunFirstLevelAlarm = templist.at(15).toInt();
    APPSetting::linzhouchilunFirstLevelAlarm = templist.at(16).toInt();

    APPSetting::baowaiSecondaryAlarm = templist.at(17).toInt();
    APPSetting::baoneiSecondaryAlarm = templist.at(18).toInt();
    APPSetting::waihuanSecondaryAlarm = templist.at(19).toInt();
    APPSetting::neihuanSecondaryAlarm = templist.at(20).toInt();
    APPSetting::gundanSecondaryAlarm = templist.at(21).toInt();
    APPSetting::gunshuangSecondaryAlarm = templist.at(22).toInt();
    APPSetting::benzhouchilunSecondaryAlarm = templist.at(23).toInt();
    APPSetting::linzhouchilunSecondaryAlarm = templist.at(24).toInt();

    APPSetting::AbsoluteTemperatureThreshold = templist.at(25).toInt();
    APPSetting::RelativeTemperatureThreshold = templist.at(26).toInt();
    APPSetting::AbsoluteTemperatureWarning = templist.at(27).toInt();

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

    list.append(QString::number(APPSetting::LineNumber));
    list.append(APPSetting::CarNumber);
    list.append(APPSetting::WagonNumber);
    list.append(QString::number(APPSetting::WheelDiameter));
    list.append(APPSetting::WagonType);


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

    list.append(APPSetting::LocalIP);
    list.append(APPSetting::LocalMask);
    list.append(APPSetting::LocalGateway);
    list.append(APPSetting::LocalDNS);

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

    list.append(QString::number(APPSetting::baowaiWarning));
    list.append(QString::number(APPSetting::baoneiWarning));
    list.append(QString::number(APPSetting::waihuanWarning));
    list.append(QString::number(APPSetting::neihuanWarning));
    list.append(QString::number(APPSetting::gundanWarning));
    list.append(QString::number(APPSetting::gunshuangWarning));
    list.append(QString::number(APPSetting::benzhouchilunWarning));
    list.append(QString::number(APPSetting::linzhouchilunWarning));

    list.append(QString::number(APPSetting::baowaiFirstLevelAlarm));
    list.append(QString::number(APPSetting::baoneiFirstLevelAlarm));
    list.append(QString::number(APPSetting::waihuanFirstLevelAlarm));
    list.append(QString::number(APPSetting::neihuanFirstLevelAlarm));
    list.append(QString::number(APPSetting::gundanFirstLevelAlarm));
    list.append(QString::number(APPSetting::gunshuangFirstLevelAlarm));
    list.append(QString::number(APPSetting::benzhouchilunFirstLevelAlarm));
    list.append(QString::number(APPSetting::linzhouchilunFirstLevelAlarm));

    list.append(QString::number(APPSetting::baowaiSecondaryAlarm));
    list.append(QString::number(APPSetting::baoneiSecondaryAlarm));
    list.append(QString::number(APPSetting::waihuanSecondaryAlarm));
    list.append(QString::number(APPSetting::neihuanSecondaryAlarm));
    list.append(QString::number(APPSetting::gundanSecondaryAlarm));
    list.append(QString::number(APPSetting::gunshuangSecondaryAlarm));
    list.append(QString::number(APPSetting::benzhouchilunSecondaryAlarm));
    list.append(QString::number(APPSetting::linzhouchilunSecondaryAlarm));

    list.append(QString::number(APPSetting::AbsoluteTemperatureThreshold));
    list.append(QString::number(APPSetting::RelativeTemperatureThreshold));
    list.append(QString::number(APPSetting::AbsoluteTemperatureWarning));

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

void UDPMulticastAPI::PreStateChange(uint8_t id, uint8_t ch, DBData::DeviceState state)
{
    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append("前置");
    list.append(QString("%1|%2").arg(id).arg(ch));
    list.append(QString::number(state));
    list.append(DATETIME);
    SendBoardStatus(list.join(";"));
}

void UDPMulticastAPI::OtherBoardStateChange(uint8_t id, DBData::DeviceState state)
{
    QStringList list;
    list.append(APPSetting::WagonNumber);
    list.append("OtherBoard");
    list.append(QString("%1").arg(id));
    list.append(QString::number(state));
    list.append(DATETIME);
    SendBoardStatus(list.join(";"));
}
