#include "udpmulticastclient.h"
#include <QMutex>
#include <QNetworkDatagram>
#include <QFile>
#include "corehelper.h"
#include <QDataStream>
#include "appsetting.h"
#include "m_databaseapi.h"
UDPMulticastClient *UDPMulticastClient::self = 0;
UDPMulticastClient *UDPMulticastClient::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if(!self){
            self = new UDPMulticastClient;
        }
    }
    return self;
}

UDPMulticastClient::UDPMulticastClient(QObject *parent) : QObject(parent)
{
    IsOpen = false;
    m_ip = QHostAddress(APPSetting::UDPMulticastIP);
    udpServer = new QUdpSocket(this);
    connect(udpServer,&QUdpSocket::readyRead,this,&UDPMulticastClient::readData);


    timerHeartbeat = new QTimer;
    timerHeartbeat->setInterval(APPSetting::HeartbeatInterval);
    connect(timerHeartbeat,&QTimer::timeout,this,&UDPMulticastClient::SendHeartbeat);
}

void UDPMulticastClient::readData()
{
    QNetworkDatagram datagrm = udpServer->receiveDatagram();
    CheckData(datagrm.data());
}

void UDPMulticastClient::CheckData(const QByteArray array)
{
    int len = array.length();
    if(len == 0){
        return;
    }

    uint8_t frame[len];
    for(int i=0;i<len;i++){
        frame[i] = array.at(i);
    }

    //判断帧头帧尾若非主从机通信数据则不处理
    if((frame[0] == UDP_PACKET_HEAD1) && (frame[1] == UDP_PACKET_HEAD2) && (frame[len-1] == UDP_PACKET_END1) &&(frame[len-2] == UDP_PACKET_END2)){
        //校验和判断
        uint8_t oldcrc = frame[len-3];
        uint8_t newcrc = CoreHelper::CheckSum(frame,2,len-3);
        if(oldcrc == newcrc){
            uint8_t order = frame[2];
            uint16_t datalen = frame[3]<<8 | frame[4];
            QString DataContent = array.mid(5,datalen);
            switch (order) {
            case 0x01://校时
                updateTime(DataContent);
                break;
            case 0x02://自检
                SelfInspection(DataContent);
                break;
            default:
                break;
            }
        }
    }
}

void UDPMulticastClient::updateTime(QString DataContent)
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
    QString cmd = QString("date %1%2%3%4%5.%6").arg(list.at(1)).arg(list.at(2)).arg(list.at(3)).arg(list.at(4)).arg(list.at(5)).arg(list.at(6));
    system(cmd.toLatin1());
    system("hwclock -w");
#endif
}

void UDPMulticastClient::SelfInspection(QString DataContent)
{
    QStringList list = DataContent.split(";");
    QString Wagon = list.at(0);
    int pre_id = list.at(1).toInt();
    if(Wagon == "all" || Wagon == APPSetting::WagonNumber){
        //如果是全部或指定了本机需要自检，需要处理
        Q_EMIT Signal_SelfInspection(pre_id);
    }
}

bool UDPMulticastClient::start()
{
    bool ret = udpServer->bind(QHostAddress::AnyIPv4,APPSetting::UDPListenPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    if(ret){
        ret = udpServer->joinMulticastGroup(m_ip);
        if(ret){
            udpServer->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁用本地回环
            IsOpen = true;
            SendHeartbeat();
            timerHeartbeat->start();
        }
    }
    return ret;
}

bool UDPMulticastClient::stop()
{
    timerHeartbeat->stop();
    bool ret = udpServer->leaveMulticastGroup(m_ip);
    if(ret){
        udpServer->abort();
        IsOpen = false;
    }
    return ret;
}

void UDPMulticastClient::SendSelfInspection(QString Data)
{
    QByteArray array = GetPackage(0x03,Data);
    senddata(array);
}

void UDPMulticastClient::SendTemInspection(QString Data)
{
    QByteArray array = GetPackage(0x04,Data);
    senddata(array);
}

void UDPMulticastClient::SendComprehensiveInspection(QString Data)
{
    QByteArray array = GetPackage(0x05,Data);
    senddata(array);
}

void UDPMulticastClient::SendHeartbeat()
{
    QByteArray array = GetPackage(0x06,APPSetting::WagonNumber);
    senddata(array);
}

void UDPMulticastClient::UpdateSoftware(QString info)
{
    QStringList templist = info.split(";");
    QString wagon = templist.at(0);
    if((wagon != APPSetting::WagonNumber) && (wagon != "AllDevice")){
        return;
    }
    uint8_t type = templist.at(1).toInt();
    int current = templist.at(2).toInt();
    int all = templist.at(3).toInt();
    QString version = templist.at(4);
    int oldcrc = templist.at(5).toInt();
    QByteArray data = QByteArray::fromBase64(templist.at(6).toUtf8());
    if(current == 1){//第一个包需要打开文件
        QString filename = QString("%1/%2.bin").arg(CoreHelper::APPPath()).arg(version);
        UpdateFile.setFileName(filename);
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
        qint32 newcrc = CoreHelper::GetrCRC32_MPEG_2(filename);
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
                Q_EMIT PreSoftwareUpdate(filename,newcrc);
                break;
            case 0x02://速度检测板卡
            case 0x03://前置IO板卡
            case 0x04://通信板卡
                Q_EMIT BackboardCardSoftwareUpdate(filename,newcrc,type);
                break;
            case 0x05://计算板卡
            {
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"系统升级");
                system(QString("mv %1 /software/RailTrafficMonitoring_KM4").arg(filename).toLatin1());
//                CoreHelper::sleep(1000);
                QThread::msleep(1000);
                system("chmod 777 /software/RailTrafficMonitoring_KM4");
                CoreHelper::Reboot();
            }
                break;
            }
        }
    }
}

void UDPMulticastClient::DeviceReboot(QString info)
{

}

void UDPMulticastClient::UpdateBearing(QString info)
{

}

void UDPMulticastClient::UpdatePre(QString info)
{

}

void UDPMulticastClient::UpdateTimerTask(QString info)
{

}

void UDPMulticastClient::UpdateCarInfo(QString info)
{

}

void UDPMulticastClient::UpdateNetWorkConfing(QString info)
{

}

void UDPMulticastClient::UpdateLinkDevice(QString info)
{

}

void UDPMulticastClient::UpdateSaveInterval(QString info)
{

}

void UDPMulticastClient::UpdateAlarmLimit(QString info)
{

}

void UDPMulticastClient::GetPreStateList(QString info)
{

}

void UDPMulticastClient::GetBearingInfo(QString info)
{

}

void UDPMulticastClient::GetTimerTask(QString info)
{

}

void UDPMulticastClient::GetCarConfig(QString info)
{

}

void UDPMulticastClient::GetNetworkConfig(QString info)
{

}

void UDPMulticastClient::GetLinkDeviceInfo(QString info)
{

}

void UDPMulticastClient::GetSaveInterval(QString info)
{

}

void UDPMulticastClient::GetOtherBoardState(QString info)
{

}

void UDPMulticastClient::GetAlarmLimit(QString info)
{

}

void UDPMulticastClient::HMIError(uint8_t error_order)
{

}

void UDPMulticastClient::senddata(const QByteArray data)
{
    if(IsOpen){
        udpServer->writeDatagram(data,m_ip,APPSetting::UDPListenPort);
    }
}

QByteArray UDPMulticastClient::GetPackage(uint8_t order, QString data)
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

