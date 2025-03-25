#ifndef UPPERCOMPUTER_H
#define UPPERCOMPUTER_H
/*
 * 上位机通信模块 2024-12-03
 * 通信协议版本v1.0.0
 * 作者：Mr.Liu
*/
#include <QObject>
#include <QtCore>
#include <QtNetwork>

#define PACKET_HEADER   0xB6            //帧头
#define PACKET_END      0x6B            //帧尾

class UpperComputer : public QObject
{
    Q_OBJECT
public:
    static UpperComputer *Instance();
    explicit UpperComputer(QObject *parent = nullptr);

private:
    static UpperComputer *self;
    QTcpSocket *socket;
    QTcpServer *server;
    int ListenPort;
    bool InCompress;
    QStringList deletefilelist;

private slots:
    void newConnection(void);//新连接
    void disConnect(void);//断开连接
    void error(QAbstractSocket::SocketError socketError);
    void readData();
    void checkData(QByteArray data);
    void onCompressionFinished(QString filename);
    void compressTarxz(const QString &outputfile, const QStringList &filelist);
    void comperssCopy(QString filename);
public:
    bool StartListen();
private://事件处理函数
    //获取配置文件
    void GetConfigIni(QString info);

    //获取数据库文件
    void GetDBFile(QString info);

    //获取原始数据(范围)
    void GetDataFile(QString info);

    //数据下载
    void DownloadData(QString info);

    //数据下载
    void GetData(QString info);

    //删除zip数据
    void ClearZIPData(QString info);

    //获取发送包
    QByteArray GetPackage(uint8_t order, QString data);

    //错误命令
    void ErrorOrder(uint8_t order,QString content);

    //发送数据
    void SendData(QByteArray arr);

    //压缩文件
    bool compressZip(const QString &zipFilePath, const QStringList &filesToCompress,bool deletefile = false);

    //发送打包文件
    void SendAllDataFile(bool ShouldDelete,const QStringList &filelist);


signals:
    void DBFileCopy(QString dbfilename);
    void DBReturn(QString filename);
    void StartTarBZ2(const QString &outputfile, const QStringList &filelist);
    void GetDataReturn(bool ShouldDelete,const QStringList &filelist);
};

#endif // UPPERCOMPUTER_H
