#include "tcpreceivefilethread.h"

ReceiveFileThread::ReceiveFileThread(QTcpSocket *socket, QObject *parent) : QThread(parent)
{
    blockSize = 0;
    savePath = DataLog::Instance()->GetRunDataPath();
    stopped = false;
    arraylist.clear();
    this->socket = socket;
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disConnect()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));
}

void ReceiveFileThread::run()
{
#ifndef TA_Origin
    while(!stopped){
        receiveData();
    }
    stopped = false;
#else
    exec();
#endif
}

void ReceiveFileThread::disConnect()
{
#ifndef TA_Origin
    stopped = true;
#endif
    quit();
    socket->deleteLater();
}

void ReceiveFileThread::readData()
{
    while (socket->bytesAvailable() >= sizeof(qint64)) {
        if (blockSize == 0) {
            if (socket->bytesAvailable() < sizeof(qint64)) {
                return;
            }

            //读取数据块大小标识符
            socket->read((char *)&blockSize, sizeof(qint64));
        }

        if (socket->bytesAvailable() < blockSize) {
            return;
        }

        //只有当收到的数据块大小和预期的一致才读取数据
        QByteArray data = socket->read(blockSize);
        //读取到的数据交给receiveData函数处理
#ifndef TA_Origin
        QMutexLocker lock(&mutex);
        arraylist.append(data);
        blockSize = 0;
#else
        receiveData(data);
#endif

    }
}

void ReceiveFileThread::error()
{
    //通信出错则打印错误内容并关闭文件和删除文件
    if (file.isOpen()) {
        file.close();
        //通信出错则打印错误内容并关闭文件和删除文件
        qDebug() << TIMEMS << QString("Tcp serve error:%1").arg(socket->errorString());
        file.remove(fileName);
        qDebug() << TIMEMS << QString("remove file:%1").arg(file.fileName());
    }
    disConnect();
}

void ReceiveFileThread::receiveData(QByteArray &buffer)
{
    //初始化数据流用于接收并写入文件
    QDataStream in(&buffer, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_12);

    //设置关键字接收对应的标识符
    int key;
    QByteArray data;
    in >> key >> data;

    switch (key) {
    case 0x01:
    {
        //接收到文件名称,将对应文件保存 默认可执行文件同一目录
        fileName = fileName.fromUtf8(data.data(), data.size());
        QString dir;
        if(fileName.startsWith("config", Qt::CaseInsensitive)){
            //如果是其他设备配置，存入config文件夹
            dir = QString("%1/config/%2").arg(CoreHelper::APPPath()).arg(fileName);
        }else{//否则存入运行日志
            dir = savePath + "/" + fileName;
        }
        file.setFileName(dir);

        //如果同名文件存在则先删除原有文件
        if (file.exists()) {
            qDebug() << TIMEMS << "file exist and remove file:" << fileName;
            file.remove();
        }

        //如果文件打不开写入则退出
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << TIMEMS << "can not open file for write";
            break;
        }
    }
        break;

    case 0x02: {
        //接收到文件大小
        QString size = QString::fromUtf8(data.data(), data.size());
        break;
    }

    case 0x03:
        //逐个写入文件内容,调用flush使其立即执行
        file.write(data.data(), data.size());
//        file.flush();
        break;

    case 0x04:
        //接收到文件结束符号
        file.flush();
        file.close();
//        qDebug()<<"receiveFile close " << DATETIMES;
        //延时1秒钟等到文件写入完成
        msleep(300);
        break;
    }
}

void ReceiveFileThread::receiveData()
{
    if(arraylist.isEmpty()) return;
    mutex.lock();
    QByteArray buffer = arraylist.takeFirst();
    mutex.unlock();

    //初始化数据流用于接收并写入文件
    QDataStream in(&buffer, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_12);

    //设置关键字接收对应的标识符
    int key;
    QByteArray data;
    in >> key >> data;

    switch (key) {
    case 0x01:
    {
        //接收到文件名称,将对应文件保存 默认可执行文件同一目录
        fileName = fileName.fromUtf8(data.data(), data.size());
        QString dir;
        if(fileName.startsWith("config", Qt::CaseInsensitive)){
            //如果是其他设备配置，存入config文件夹
            dir = QString("%1/config/%2").arg(CoreHelper::APPPath()).arg(fileName);
        }else{//否则存入运行日志
            dir = savePath + "/" + fileName;
        }
        file.setFileName(dir);

        //如果同名文件存在则先删除原有文件
        if (file.exists()) {
            qDebug() << TIMEMS << "file exist and remove file:" << fileName;
            file.remove();
        }

        //如果文件打不开写入则退出
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << TIMEMS << "can not open file for write";
            break;
        }
    }
        break;

    case 0x02: {
        //接收到文件大小
        QString size = QString::fromUtf8(data.data(), data.size());
        break;
    }

    case 0x03:
        //逐个写入文件内容,调用flush使其立即执行
        file.write(data.data(), data.size());
//        file.flush();
        break;

    case 0x04:
        //接收到文件结束符号
        file.flush();
        file.close();
//        qDebug()<<"receiveFile close " << DATETIMES;
        //延时1秒钟等到文件写入完成
        msleep(300);
        break;
    }
}

TcpReceiveFileThread *TcpReceiveFileThread::self = 0;
TcpReceiveFileThread *TcpReceiveFileThread::Instance()
{
    if (!self) {
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!self) {
            self = new TcpReceiveFileThread;
        }
    }
    return self;
}

TcpReceiveFileThread::TcpReceiveFileThread(QObject *parent) : QObject(parent)
{
    listenPort = APPSetting::TcpListenPort;

    socket = 0;

    server = new QTcpServer(this);
    connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void TcpReceiveFileThread::newConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *socket = server->nextPendingConnection();
        QString ip = socket->peerAddress().toString();
        qDebug()<<"new connect ip is " << ip;
        ReceiveFileThread *receiveFile = new ReceiveFileThread(socket);
        receiveFile->start();
    }
}


bool TcpReceiveFileThread::startListen()
{
#if (QT_VERSION > QT_VERSION_CHECK(5,0,0))
    bool ok = server->listen(QHostAddress::AnyIPv4, listenPort);
#else
    bool ok = server->listen(QHostAddress::Any, listenPort);
#endif
    qDebug() << TIMEMS << QString("listen receivefile port %1").arg(ok ? "ok" : "error");
    return ok;
}

void TcpReceiveFileThread::closeListen()
{
    server->close();
}

void TcpReceiveFileThread::setListenPort(int listenPort)
{
    if (this->listenPort != listenPort) {
        this->listenPort = listenPort;
    }
}


