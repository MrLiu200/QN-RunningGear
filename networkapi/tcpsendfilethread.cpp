#include "tcpsendfilethread.h"
#include "m_databaseapi.h"
#include "appsetting.h"
#include <dbdata.h>
#include "datalog.h"
#include <QElapsedTimer>
TcpSendFileThread::TcpSendFileThread(QObject *parent) : QThread(parent)
{
    InitParameter();
    timeSendVibData = new QTimer(this);
    timeSendVibData->setInterval(APPSetting::TcpSendFileInterval);
//    timeSendVibData->start();
    connect(this,SIGNAL(error(QString)),this,SLOT(SendError(QString)));
    connect(timeSendVibData,SIGNAL(timeout()),this,SLOT(FindSendFile()));

}

void TcpSendFileThread::run()
{
    Socket = new QTcpSocket;
#ifdef DEVICE_SLAVE
    SocketStandby = new QTcpSocket;
#endif
    while(!stopped){
        FindSendFile();
        SendFile();

    }
    stopped = false;
}

void TcpSendFileThread::InitParameter()
{
    this->NeedSend = false;
    this->stopped = false;
    this->Filename = "";
    int count = DBData::LinkDeviceInfo_Type.count();
    this->ServerIp = APPSetting::TcpServerIP;
    this->ServerPort = APPSetting::TcpServerPort;
#ifdef DEVICE_SLAVE
    this->ServerIpStandby = APPSetting::TcpServerIPStandby;
    this->ServerPortStandby = APPSetting::TcpServerPortStandby;
#endif
    if(count != 0){
        for(int i=0;i<count;i++){
            QString str = DBData::LinkDeviceInfo_Type.at(i);
            if(str == "Host"){
                this->ServerIp = DBData::LinkDeviceInfo_IP.at(i);
                this->ServerPort = DBData::LinkDeviceInfo_Port.at(i);
            }
#ifdef DEVICE_SLAVE
            if(str == "StandbyHost"){
                this->ServerIpStandby = DBData::LinkDeviceInfo_IP.at(i);
                this->ServerPortStandby = DBData::LinkDeviceInfo_Port.at(i);
            }
#endif
        }
    }
}

void TcpSendFileThread::Thread_Stop()
{
    stopped = true;
    timeSendVibData->stop();
}

void TcpSendFileThread::FindSendFile()
{
//    qDebug()<<"****" <<DATETIMES << "****DBData::TcpSendList.size() = " << DBData::TcpSendList.size() << "NeedSend = " << NeedSend;
    if(NeedSend == true) return;
    int count = DBData::TcpSendList.size();
    if(count == 0){
        return;
    }

    QString name = DBData::TcpSendList.first();
    if(!name.isEmpty()){
        Filename = name;
        DBData::TCPDeleteSendFile(0);
        NeedSend = true;
    }

//    for(int i=0;i<count;i++){
//        QString name = DBData::TcpSendList.at(i);
//        if(!name.isEmpty()){
//            Filename = name;
//            DBData::TCPDeleteSendFile(i);
//            NeedSend = true;
//            break;
//        }
//    }
}

void TcpSendFileThread::disConnect()
{
    Socket->deleteLater();
#ifdef DEVICE_SLAVE
    SocketStandby->deleteLater();
#endif
}

void TcpSendFileThread::SendFile()
{
    if(!NeedSend) return;
//    QElapsedTimer elapsedtimer;
//    elapsedtimer.start();
    QFile file(Filename);
    qint64 fileSize = file.size();
    bool sendsuccess = false;
    //检测文件是否存在已经文件能否打开
    if (fileSize == 0 || !file.open(QIODevice::ReadOnly)) {
        qDebug()<<"TcpSendFileThread::SendFile Error << fileSize = " << fileSize << file.errorString() << Filename;
        if(file.isOpen()){
            file.close();
        }
        NeedSend = false;
        return;
    }

    bool ok = true;
    bool ok1 = true;

    //连接不存在则建立连接,连接服务器,等待1秒钟
    if (Socket->state() !=  QAbstractSocket::ConnectedState) {
        Socket->connectToHost(ServerIp, ServerPort);
        ok = Socket->waitForConnected(500);
    }
#ifdef DEVICE_SLAVE
    if (SocketStandby->state() !=  QAbstractSocket::ConnectedState) {
        SocketStandby->connectToHost(ServerIpStandby, ServerPortStandby);
        ok1 = SocketStandby->waitForConnected(500);
    }
#endif
    if ((ok || ok1)) {
        qint64 size;
        QByteArray block;

        //准备数据流,设置数据流版本
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);

        //获取文件名称
        QFileInfo fileInfo(Filename);
        QString name = fileInfo.fileName();

        //写入开始符及文件名称
        block.clear();
        out.device()->seek(0);
        out << 0x01 << name.toUtf8();
        size = block.size();
        //写入当前数据块大小,以便接收端收到后判断
        if(ok){
            Socket->write((char *)&size, sizeof(qint64));
            Socket->write(block.data(), size);
            if (!Socket->waitForBytesWritten(500)) {
                Socket->disconnectFromHost();
                ok = false;
            }
        }
#ifdef DEVICE_SLAVE
        if(ok1){
            SocketStandby->write((char *)&size, sizeof(qint64));
            SocketStandby->write(block.data(), size);

            if (!SocketStandby->waitForBytesWritten(500)) {
                SocketStandby->disconnectFromHost();
                ok1 = false;
            }
        }
#endif
        //写入文件大小
        block.clear();
        out.device()->seek(0);
        out << 0x02 << QString::number(fileSize).toUtf8();
        size = block.size();
        //写入当前数据块大小,以便接收端收到后判断
        if(ok){
            Socket->write((char *)&size, sizeof(qint64));
            Socket->write(block.data(), size);

            if (!Socket->waitForBytesWritten(500)) {
                Socket->disconnectFromHost();
                ok = false;
            }
        }
#ifdef DEVICE_SLAVE
        if(ok1){
            SocketStandby->write((char *)&size, sizeof(qint64));
            SocketStandby->write(block.data(), size);

            if (!SocketStandby->waitForBytesWritten(500)) {
                SocketStandby->disconnectFromHost();
                ok1 = false;
            }
        }
#endif
        //        qDebug()<<"Send file size"<< DATETIMES;
        //循环写入文件数据
        do {
            block.clear();
            out.device()->seek(0);
            //这里做个限制,每次最多发送 65535 个字节
            out << 0x03 << file.read(0xFFFF);
            size = block.size();
            //写入当前数据块大小,以便接收端收到后判断
            if(ok){
                Socket->write((char *)&size, sizeof(qint64));
                Socket->write(block.data(), size);
                if (!Socket->waitForBytesWritten(500)) {
                    Socket->disconnectFromHost();
                    ok = false;
                }
            }
#ifdef DEVICE_SLAVE
            if(ok1){
                SocketStandby->write((char *)&size, sizeof(qint64));
                SocketStandby->write(block.data(), size);

                if (!SocketStandby->waitForBytesWritten(500)) {
                    SocketStandby->disconnectFromHost();
                    ok1 = false;
                }
            }
#endif
            //            qDebug()<<"Send file content"<< DATETIMES;
        } while (!file.atEnd());


        //写入结束符及文件名称
        block.clear();
        out.device()->seek(0);
        out << 0x04 << name.toUtf8();
        size = block.size();
        //写入当前数据块大小,以便接收端收到后判断.
        if(ok){
            Socket->write((char *)&size, sizeof(qint64));
            Socket->write(block.data(), size);
            if (!Socket->waitForBytesWritten(500)) {
                Socket->disconnectFromHost();
                ok = false;
            }
        }
#ifdef DEVICE_SLAVE
        if(ok1){
            SocketStandby->write((char *)&size, sizeof(qint64));
            SocketStandby->write(block.data(), size);
            if (!SocketStandby->waitForBytesWritten(500)) {
                SocketStandby->disconnectFromHost();
                ok1 = false;
            }
        }
#endif
//        qDebug()<< "connect state" <<Socket->state() << SocketStandby->state();
//        NeedSend = false;
        sendsuccess = true;
    }else{
//        qDebug()<< "connect fail" <<Socket->state() << SocketStandby->state();
    }
    file.close();

    if(sendsuccess){
#ifdef DEVICE_SLAVE
       file.remove();
//       qDebug()<<"remove file :" << Filename;
#endif
       NeedSend = false;
//       qint64 elapsed = elapsedtimer.elapsed();
//       qDebug() << "Elapsed time: " << elapsed << "ms";
//       elapsedtimer.start();
    }
}

void TcpSendFileThread::SendError(QString name)
{
    //取出数据文件的内容
    QString str = QString("TCP发送出错,文件:").arg(name);
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"主机上报",DATETIME,"str");
}
