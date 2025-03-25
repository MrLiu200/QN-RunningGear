#include "uppercomputer.h"
#include "appsetting.h"
#include "m_databaseapi.h"
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QFileInfo>
#include <QtConcurrent>
#include "datalog.h"
#include "quazip.h"
#include "quazipfile.h"
#include "JlCompress.h"
UpperComputer *UpperComputer::self = 0;
UpperComputer *UpperComputer::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if(!self){
            self = new UpperComputer;
        }
    }
    return self;
}

UpperComputer::UpperComputer(QObject *parent) : QObject(parent)
{
    this->ListenPort = APPSetting::HMIListenPort;
    InCompress = false;
    deletefilelist.clear();

    socket =0;
    server = new QTcpServer(this);
    connect(server,&QTcpServer::newConnection,this,&UpperComputer::newConnection);
    connect(this,&UpperComputer::DBFileCopy,this,&UpperComputer::comperssCopy);
    connect(this,&UpperComputer::DBReturn,this,&UpperComputer::onCompressionFinished);
    connect(this,&UpperComputer::StartTarBZ2,this,&UpperComputer::compressTarxz);
    connect(this,&UpperComputer::GetDataReturn,this,&UpperComputer::SendAllDataFile);


}

void UpperComputer::newConnection()
{
    if (socket) {
        // 如果已经有客户端连接，拒绝新的连接
        qDebug() << "Client already connected, rejecting new connection.";
        QTcpSocket *newClient = server->nextPendingConnection();
        newClient->disconnectFromHost();
        newClient->deleteLater();
        return;
    }
    socket = server->nextPendingConnection();
    QString ip = socket->peerAddress().toString();
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("上位机(ip:%1)加入调试").arg(ip));
    connect(socket,&QTcpSocket::readyRead,this,&UpperComputer::readData);
    connect(socket,&QTcpSocket::disconnected,this,&UpperComputer::disConnect);
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));

    qDebug()<<"Welcome to the server!";
    socket->write("Welcome to the server!\n");

    socket->flush();
}

void UpperComputer::disConnect()
{
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"上位机退出调试");
    socket->deleteLater();
    socket = nullptr;
}

void UpperComputer::error(QAbstractSocket::SocketError socketError)
{

    Q_UNUSED(socketError);

    if (socket) {
        // 打印错误信息
        qWarning() << "Socket error: " << socket->errorString();
    }

    // 处理错误后的清理工作
    if (socket) {
        socket->close();
        socket->deleteLater();
        socket = nullptr;
    }
}

void UpperComputer::readData()
{
    if(socket){
        QByteArray array = socket->readAll();
        checkData(array);
    }
}


void UpperComputer::checkData(QByteArray data)
{
    int len = data.size();
    if(len == 0 ) return;

    uint8_t frame[len];

    for(int i=0; i<len; i++){
        frame[i] = data[i];
    }
    if((frame[0] == PACKET_HEADER) && (frame[1] == PACKET_HEADER) && (frame[len-1] == PACKET_END) &&(frame[len-2] == PACKET_END)){
        uint8_t orderByte = frame[2];
        uint16_t datalen = (frame[3] << 8) | frame[4];
        //数据内容
        QString info = QString::fromUtf8(data.mid(5,datalen));

        switch (orderByte) {
        case 0xA0://获取配置文件
            GetConfigIni(info);
            break;
        case 0xA1://获取数据库文件
//            GetDBFile(info);
            if(!InCompress){
                QtConcurrent::run(this, &UpperComputer::GetDBFile, info);
            }
            break;
        case 0xA2://获取原始数据(范围u)
            if(!InCompress){
                QtConcurrent::run(this, &UpperComputer::GetDataFile, info);
            }
            break;
        case 0xA3://下载所有数据
//            DownloadData(info);
            if(!InCompress){
                QtConcurrent::run(this, &UpperComputer::DownloadData, info);
            }
            break;
        case 0xA4://2025-02-12 测试
            if(!InCompress){
                QtConcurrent::run(this, &UpperComputer::GetData, info);
            }
            break;
        case 0xA5://删除文件
        {
            if(QFile::exists(info)){
                QFile::remove(info);
            }else{
                ErrorOrder(0xA5,QString("%1 不存在").arg(info));
            }
        }
            break;
        case 0xA6://清空打包文件
            ClearZIPData(info);
            break;
        default:
            break;
        }
    }
}

void UpperComputer::onCompressionFinished(QString filename)
{
    InCompress = false;
    if(filename.isEmpty()){
        ErrorOrder(0xA2,"区间内数据为空");
        qDebug()<< "文件压缩失败";
    }else{
        qDebug()<<"压缩完成,发送";
        QByteArray arr = GetPackage(0xA2,filename);
        SendData(arr);
    }
}

bool UpperComputer::StartListen()
{
#if (QT_VERSION > QT_VERSION_CHECK(5,0,0))
    bool ok = server->listen(QHostAddress::AnyIPv4, ListenPort);
#else
    bool ok = server->listen(QHostAddress::Any, listenPort);
#endif
    qDebug() << TIMEMS << QString("listen UpperComputer port %1").arg(ok ? "ok" : "error");
    return ok;
}

void UpperComputer::GetConfigIni(QString info)
{
    //将现有的配置文件拷贝至ftp访问目录中
    //找出所有配置文件
    if(!info.isEmpty()) return;
    QStringList SendList;
    QString configdir = QString("%1/config").arg(CoreHelper::APPPath());
    QDir dir(configdir);

    // 设置过滤器，查找所有以 config 开头的文件
    QStringList filters;
    filters << "config*";  // 根据需要调整匹配模式
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);  // 仅查找文件，不包括目录

    QFileInfoList files = dir.entryInfoList();  // 获取文件列表

    // 遍历文件并复制到目标目录

    foreach (const QFileInfo &fileInfo, files) {
#if 0
        QString targetFilePath = APPSetting::FtpDirName + "/" + fileInfo.fileName();
        if(CoreHelper::fileIsExist(targetFilePath)){
            QFile(targetFilePath).remove();
        }
        if (QFile::copy(fileInfo.absoluteFilePath(), targetFilePath)) {
            SendList.append(fileInfo.fileName());
            qDebug()<<"获取配置文件,发送了";
        } else {
            qDebug() << "Failed to copy:" << fileInfo.fileName();
        }
#endif
        SendList.append(fileInfo.absoluteFilePath());
    }


    QByteArray arr = GetPackage(0xA0,SendList.join(";"));
    SendData(arr);
}

void UpperComputer::GetDBFile(QString info)
{
    if(!info.isEmpty()) return;
    InCompress = true;
#if 0
    QString dbname = CoreHelper::APPPath() + "/RunningGear.db";
    QString targetFilePath = APPSetting::FtpDirName + "/RunningGear.db";
    if(CoreHelper::fileIsExist(targetFilePath)){
        QFile(targetFilePath).remove();
    }
    if(QFile::copy(dbname, targetFilePath)){
//        QByteArray arr = GetPackage(0xA1,"RunningGear.db");
//        SendData(arr);
        qDebug()<<"获取db file，发送了";
        Q_EMIT DBFileCopy("RunningGear.db");
    }else{
         qDebug()<<"获取db file，拷贝失败";
         Q_EMIT DBFileCopy(QString());
    }
#else
    QString dbfile = CoreHelper::APPPath() + "/RunningGear.db";
    QFileInfo fileinfo(dbfile);
    QString filename = fileinfo.absoluteFilePath();
    Q_EMIT DBFileCopy(filename);
#endif
}

void UpperComputer::GetDataFile(QString info)
{
    InCompress = true;
    QStringList list = info.split(";");
    qDebug()<<"receive data: " << info;
    QString wagon = list.at(0);
    int id = list.at(1).toInt();
    int ch = list.at(2).toInt();
    QString starttime = list.at(3);
    QString endtime = list.at(4);

    QString datadir = DataLog::Instance()->GetRunDataPath();
    QDir dir(datadir);
//    qDebug()<<"datadir = " << datadir;
    // 查找符合条件的文件（数据类型_车厢号_前置ID_通道号_时间.bin）
    QStringList filters;
    filters << "*.bin"; // 查找所有 .bin 文件
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    QFileInfoList files = dir.entryInfoList();
    qDebug()<<"files.size = " << files.size();
    //筛选出的文件列表
    QStringList filesToCompress;

    //循环匹配文件
    foreach (const QFileInfo &fileInfo, files) {
        QString fileName = fileInfo.fileName();
//        qDebug()<<"selete fileName = " << fileName;
        // 根据车厢号、前置ID、通道号和时间过滤文件
        if (DataLog::Instance()->isFileMatchConditions(fileName, wagon, id, ch)
                && DataLog::Instance()->isFileInRange(fileName, starttime, endtime)) {
            filesToCompress.append(fileInfo.absoluteFilePath());
        }
    }
    qDebug()<<"filesToCompress file size : " << filesToCompress.size();
    // 将符合条件的文件压缩为一个压缩包
    if (!filesToCompress.isEmpty()) {
//        QString basename = APPSetting::WagonNumber + "_temporarily.zip";
        QString basename = QString("%1_%2_%3_temporarily.zip").arg(wagon).arg(id).arg(ch);
        QString tarFileName = APPSetting::FtpDirName + "/" + basename;
        if(compressZip(tarFileName,filesToCompress)){
//            Q_EMIT DBReturn(basename);
            Q_EMIT DBReturn(tarFileName);
            qDebug()<<"获取振动数据，发送了";
        }
    }else{
        Q_EMIT DBReturn(QString());
        qDebug()<<"获取振动数据，数据为空";
    }
}

void UpperComputer::DownloadData(QString info)
{
    InCompress = true;
    int proportion = CoreHelper::Getcapacity("/udisk");
    qDebug()<< "/udisk proportion = " << proportion;
    if(proportion > 45){
        QByteArray arr = GetPackage(0xA3,QString());
        SendData(arr);
        InCompress = false;
        return;
    }
    QStringList list = info.split(";");
    QString wagon = list.at(0);
    int id = list.at(1).toInt();
    int ch = list.at(2).toInt();
    uint8_t enabletime = list.at(3).toInt();
    QString starttime = list.at(4);
    QString endtime = list.at(5);
    uint8_t deleteSourceFiles = list.at(6).toInt();

    QString datadir = DataLog::Instance()->GetRunDataPath();
    QDir dir(datadir);
    // 查找符合条件的文件（数据类型_车厢号_前置ID_通道号_时间.bin）
    QStringList filters;
    filters << "*.bin"; // 查找所有 .bin 文件
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    QFileInfoList files = dir.entryInfoList();
    qDebug()<<"files.size = " << files.size();
    //筛选出的文件列表
    QStringList filesToCompress;

    //循环匹配文件
    foreach (const QFileInfo &fileInfo, files) {
        QString fileName = fileInfo.fileName();
        // 根据车厢号、前置ID、通道号和时间过滤文件
        if (DataLog::Instance()->isFileMatchConditions(fileName, wagon, id, ch)){
            if (!enabletime || DataLog::Instance()->isFileInRange(fileName, starttime, endtime)) {
                filesToCompress.append(fileInfo.absoluteFilePath());
            }
        }
    }
    qDebug()<<"符合条件的文件数量为:" << filesToCompress.size();
    if(deleteSourceFiles){
        deletefilelist = filesToCompress;
    }else{
        deletefilelist.clear();
    }
    if (!filesToCompress.isEmpty()) {
        QString basename = APPSetting::WagonNumber + "_AllData.tar.bz2";
        QString tarFileName = APPSetting::FtpDirName + "/" + basename;
//        compressTarxz(tarFileName,filesToCompress);
        Q_EMIT StartTarBZ2(tarFileName,filesToCompress);
    }
}

void UpperComputer::GetData(QString info)
{
    InCompress = true;
    int proportion = CoreHelper::Getcapacity("/udisk");
//    qDebug()<< "/udisk proportion = " << proportion;
    if(proportion > 60){
        ErrorOrder(0xA4,QString("内存已使用%1,无法生成压缩包").arg(proportion));
        InCompress = false;
        return;
    }
    QStringList list = info.split(";");
    QString starttime = list.at(0);
    QString endtime = list.at(1);
    uint8_t deleteSourceFiles = list.at(2).toInt();
    QDate StartDate = QDate::fromString(starttime,"yyyy-MM-dd");
    QDate EndDate = QDate::fromString(endtime,"yyyy-MM-dd");

    //将打包的文件判断时间是否符合区间
    QStringList SendFileList;
    QString dirstr = DataLog::Instance()->GetPreviousPath();
    QDir dir_all(dirstr);
    // 查找符合条件的文件（数据类型_车厢号_前置ID_通道号_时间.bin）
    QStringList filters;
    filters << "*data.zip"; // 查找所有 _data.zip 文件
    dir_all.setNameFilters(filters);
    dir_all.setFilter(QDir::Files);
    QFileInfoList files_pack = dir_all.entryInfoList();

    //筛选出的文件列表

    //循环匹配文件
    foreach (const QFileInfo &fileInfo, files_pack) {
        QString fileName = fileInfo.fileName();
        //文件名称yyyy-MM-dd_data.zip
        QStringList fileList = fileName.split("_");
        QString filetime = fileList.at(0);
        QDate fileDate = QDate::fromString(fileList.at(0),"yyyy-MM-dd");
        if((fileDate >= StartDate) && (fileDate <= EndDate)){
            SendFileList.append(fileInfo.absoluteFilePath());
        }
    }



    //判断截止时间是否大于待打包日期，若大于，则需要将文件打包
    QDate packagingDate = QDate::fromString(APPSetting::PackagingDate,"yyyy-MM-dd");

    if(EndDate >= packagingDate){
        //打包现有文件
        QString datadir = DataLog::Instance()->GetRunDataPath();
        QDir dir(datadir);
        // 查找符合条件的文件（数据类型_车厢号_前置ID_通道号_时间.bin）
        QStringList filters;
        filters << "*.bin"; // 查找所有 .bin 文件
        dir.setNameFilters(filters);
        dir.setFilter(QDir::Files);
        QFileInfoList files = dir.entryInfoList();
        qDebug()<<"files.size = " << files.size();

        //筛选出的文件列表
        QStringList filesToCompress;
        foreach (const QFileInfo &fileInfo, files) {
            QString fileName = fileInfo.fileName();
            // 根据车厢号、前置ID、通道号和时间过滤文件
            if (DataLog::Instance()->isFileInRange(fileName, starttime+" 00:00:00", endtime+" 00:00:00")) {
                filesToCompress.append(fileInfo.absoluteFilePath());
            }
        }

        if (!filesToCompress.isEmpty()) {
            QString basename = QDate::currentDate().toString("yyyy-MM-dd") + "_data.zip";
            QString tarFileName = DataLog::Instance()->GetPreviousPath() + "/" + basename;
            qDebug()<<"开始打包今天的代码";
            if(compressZip(tarFileName,filesToCompress,deleteSourceFiles)){
                SendFileList.append(tarFileName);
            }
        }
    }
    Q_EMIT GetDataReturn(deleteSourceFiles,SendFileList);
}

void UpperComputer::ClearZIPData(QString info)
{
    if(!info.isEmpty()) return;
    if(InCompress){
        ErrorOrder(0xA6,"正在打包数据,无法清空");
    }
    //判断是否处于打包阶段，如果处于打包阶段则不处理
    QString nowdata = QDate::currentDate().toString("yyyy-MM-dd");
    if(nowdata == APPSetting::PackagingDate){
        system("rm -rf /udisk/previouslog/*");
    }else{
        ErrorOrder(0xA6,"正在进行定时任务,无法清空");
    }
}

QByteArray UpperComputer::GetPackage(uint8_t order, QString data)
{
    QByteArray messageData = data.toUtf8();
    uint16_t len = messageData.size();
    QByteArray array;
    array.append(0xB6);
    array.append(0xB6);
    array.append(order);
    array.append((len >> 8) & 0xff);
    array.append(len & 0xff);
    array.append(messageData);
    array.append(0x6B);
    array.append(0x6B);
    return array;
}

void UpperComputer::ErrorOrder(uint8_t order, QString content)
{
    QStringList list;
    list.append(QString::number(order));
    list.append(content);
    QByteArray arr = GetPackage(0xFF,list.join(";"));
    SendData(arr);
}

void UpperComputer::SendData(QByteArray arr)
{
    if(arr.isEmpty()) return;
    if(socket){
        socket->write(arr);
//        qint64 len = socket->write(arr);
//        qDebug()<<"UpperComputer::SendData >> " << len;
    }
}

bool UpperComputer::compressZip(const QString &zipFilePath, const QStringList &filesToCompress, bool deletefile)
{
    //判断文件是否存在，若文件存在，需要删除
    if(QFile::exists(zipFilePath)){
        QFile::remove(zipFilePath);
    }

    // 创建QuaZip对象，表示目标压缩包
    QuaZip zip(zipFilePath);

    // 尝试打开压缩包，如果打开失败返回false
    if (!zip.open(QuaZip::mdCreate)) {
        qDebug() << "无法打开压缩包：" << zipFilePath;
        return false;
    }

    // 遍历文件列表，将每个文件添加到压缩包
    for (const QString &file : filesToCompress) {
        QFile inputFile(file);
        if (!inputFile.open(QIODevice::ReadOnly)) {
            qDebug() << "无法打开文件：" << file;
            zip.close();
            return false;
        }

        QString fileName = QFileInfo(file).fileName();
        // 创建一个新的压缩包文件
        QuaZipFile zipFile(&zip);

        // 打开压缩包文件进行写入
        if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileName, file)))
        {
            qDebug() << "Failed to add file to ZIP:" << file;
            zip.close();
            return false;
        }

        // 读取文件数据并写入到压缩包
        QByteArray fileData = inputFile.readAll();
        zipFile.write(fileData);

        // 关闭压缩包文件
        zipFile.close();
        inputFile.close();
        if(deletefile){
            inputFile.remove();
        }
    }

    // 关闭压缩包
    zip.close();

    qDebug() << "文件已成功压缩到：" << zipFilePath;
    return true;
}

void UpperComputer::SendAllDataFile(bool ShouldDelete, const QStringList &filelist)
{
    InCompress = false;
    if(filelist.isEmpty()){
        ErrorOrder(0xA4,"期限内数据为空");
        return;
    }
    QStringList sendlist;
    sendlist.append(QString::number(ShouldDelete));
    sendlist.append(filelist.join("|"));

    SendData(GetPackage(0xA4,sendlist.join(";")));
}

void UpperComputer::compressTarxz(const QString &outputfile, const QStringList &filelist)
{
    if(filelist.isEmpty())
        return;

    if(QFile::exists(outputfile)){
        QFile::remove(outputfile);
    }

    QStringList arguments;
    arguments << "-jcf" << outputfile;  // -c: 创建, -J: xz 压缩, -v: 显示过程, -f: 指定文件名
    arguments.append(filelist);           // 添加文件列表

    // 打印命令及参数以供调试
//    qDebug() << "tar command:" << "tar" << arguments;

    QProcess *tarProcess = new QProcess(this);
//    connect(tarProcess, &QProcess::readyReadStandardOutput, [tarProcess]() {
//        qDebug() << "stdout:" << tarProcess->readAllStandardOutput();
//    });
//    connect(tarProcess, &QProcess::readyReadStandardError, [tarProcess]() {
//        qWarning() << "stderr:" << tarProcess->readAllStandardError();
//    });

    // 错误处理
    connect(tarProcess, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            this, [tarProcess](QProcess::ProcessError error) {
        qWarning() << "Process error occurred:" << error << tarProcess->errorString();
        tarProcess->deleteLater();
    });

    // 执行完成后的处理
    connect(tarProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, tarProcess, outputfile](int exitCode, QProcess::ExitStatus status) {
        InCompress = false;
        QByteArray arr;
        if (status == QProcess::NormalExit && exitCode == 0) {
            qDebug() << "Compression completed successfully.";
            if(!deletefilelist.isEmpty()){//如需删除
                for(auto file : deletefilelist){
                    if(QFile::exists(file)){
                        QFile::remove(file);
                    }
                }
            }
            QFileInfo info(outputfile);
            arr = GetPackage(0xA3,info.fileName());
        } else {
            qWarning() << "Compression failed with exit code:" << exitCode;
            deletefilelist.clear();
            arr = GetPackage(0xA3,QString());
        }
        SendData(arr);
        tarProcess->deleteLater();
    });

    // 启动 tar 进程
    tarProcess->start("tar", arguments);

}

void UpperComputer::comperssCopy(QString filename)
{
    InCompress = false;
    QByteArray arr = GetPackage(0xA1,filename);
    SendData(arr);
}

