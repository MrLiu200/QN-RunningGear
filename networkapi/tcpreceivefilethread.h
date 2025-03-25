#ifndef TCPRECEIVEFILETHREAD_H
#define TCPRECEIVEFILETHREAD_H

#include <QObject>
#include <QThread>
#include <QtNetwork>
#include "appsetting.h"
#include "corehelper.h"
#include "datalog.h"
//#define TA_Origin
class ReceiveFileThread : public QThread
{
    Q_OBJECT
public:
    explicit ReceiveFileThread(QTcpSocket *socket, QObject *parent = 0);

protected:
    void run();

private:
    QFile file;             //接收文件对象
    QString fileName;       //文件名称
    QString savePath;       //文件保存目录
    QTcpSocket *socket;     //tcp通信对象
    qint64 blockSize;       //数据块大小
    QByteArrayList arraylist;
    QMutex mutex;
    bool stopped;

private slots:
    //读取网络数据
    void readData();
    //连接断开
    void disConnect();
    //网络通信出错
    void error();
    //处理接收的数据
    void receiveData(QByteArray &buffer);
    void receiveData();
};

class TcpReceiveFileThread : public QObject
{
    Q_OBJECT
public:
    static TcpReceiveFileThread *Instance();
    explicit TcpReceiveFileThread(QObject *parent = 0);

private:
    static TcpReceiveFileThread *self;

    int listenPort;         //监听端口

    QTcpServer *server;     //用于监听端口被动接收文件
    QTcpSocket *socket;     //用于主动连接服务器请求文件

private slots:
    //服务端模式连接建立
    void newConnection();
signals:

public:
    //启动监听,开始被动接收文件
    bool startListen();
    void closeListen();

    //设置监听端口,被动接收传输过来的文件
    void setListenPort(int listenPort);
};

#endif // TCPRECEIVEFILETHREAD_H
