#include "datalog.h"
#include "appsetting.h"
#include "m_databaseapi.h"
#include "dbdata.h"
DataLog *DataLog::self = 0;
QFile *logFile = new QFile;
DataLog::DataLog(QObject *parent) : QObject(parent)
{
    LogPath = "/udisk/log";
    PreviousPath = "/udisk/previouslog";
    RunDataPath = QString("%1/rundata").arg(LogPath);
    //    logFile = new QFile;
    LogHookpath = qApp->applicationDirPath();
    QString str = qApp->applicationFilePath();
    QStringList list = str.split("/");
    LogHookname = list.at(list.count() - 1).split(".").at(0);
    CanSave = true;

}

DataLog *DataLog::Instance()
{
    if (!self) {
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!self) {
            self = new DataLog;
        }
    }

    return self;
}

void DataLog::FunLogHook(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if(!logFile->isOpen()){
        if(!initializeLogFile()){
            return;
        }
    }
    QString content;

    switch (type) {
    case QtDebugMsg:
            content = QString("Debug: %1").arg(msg);
            break;
        case QtWarningMsg:
            content = QString("Warning: %1").arg(msg);
            break;
        case QtCriticalMsg:
            content = QString("Critical: %1").arg(msg);
            break;
        case QtFatalMsg:
            content = QString("Fatal: %1").arg(msg);
            break;
        case QtInfoMsg:
            content = QString("Info: %1").arg(msg);
            break;
    }

    QTextStream logStream(logFile);
    logStream << content << "\t"<<DATETIMES<<"\n";
}

bool DataLog::initializeLogFile()
{
    //方法改进:之前每次输出日志都打开文件,改成只有当日期改变时才新建和打开文件
    QString currentDate = QDate::currentDate().toString("yyyyMMdd");
    QString fileName = QString("%1/%2_%3_log.txt")
            .arg(DataLog::Instance()->GetHookPath())
            .arg(DataLog::Instance()->GetHookName())
            .arg(currentDate);

    if(logFile->isOpen()){
        logFile->close();
    }
    logFile->setFileName(fileName);
    return logFile->open(QIODevice::WriteOnly | QIODevice::Append | QFile::Text);
}

QString DataLog::GetHookPath() const
{
    return this->LogHookpath;
}

void DataLog::SetHookPath(QString Path)
{
    if(this->LogHookpath != Path){
        this->LogHookpath = Path;
    }
}

QString DataLog::GetHookName() const
{
    return this->LogHookname;
}

void DataLog::SetHookName(QString Name)
{
    if(this->LogHookname != Name){
        this->LogHookname = Name;
    }
}

QString DataLog::GetLogPath() const
{
    return this->LogPath;
}

void DataLog::SetLogPath(QString Path)
{
    if(this->LogPath != Path){
        this->LogPath = Path;
    }
}

QString DataLog::GetRunDataPath() const
{
    return this->RunDataPath;
}

void DataLog::SetRunDataPath(QString Path)
{
    if(this->RunDataPath != Path){
        this->RunDataPath = Path;
    }
}

QString DataLog::GetPreviousPath() const
{
    return this->PreviousPath;
}

void DataLog::SetPreviousPath(QString Path)
{
    if(this->PreviousPath != Path){
        this->PreviousPath = Path;
    }
}

bool DataLog::ParameterInit()
{
    CanSave = true;
#ifndef DEVICE_SLAVE
    if(!APPSetting::IsHardDiskReady){
        CanSave = false;
        return false;
    }
#endif

    QDir LogDir(LogPath);
    QDir RunDir(RunDataPath);
    QDir PreviousDir(PreviousPath);
    if(!LogDir.exists()){
        if (!LogDir.mkdir(LogDir.absolutePath())) {
            qDebug()<<"mLogPathkdir LogDir fail";
            CanSave = false;
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"日志目录初始化错误");
        }
    }

    if(!RunDir.exists()){
        if (!RunDir.mkdir(RunDir.absolutePath())) {
            qDebug()<<"mkdir RunLogDir fail";
            CanSave = false;
        }
    }

    if(!PreviousDir.exists()){
        if(!PreviousDir.mkdir(PreviousDir.absolutePath())){
            qDebug() << "mkdir PreviousDir fail";
            CanSave = false;
        }
    }
    return CanSave;
}
#ifndef DEVICE_SLAVE
void DataLog::DeleteExternalLog(QString Time)
{
    if(!APPSetting::IsHardDiskReady) return;
    QStringList baseNames;
    QString filename;
    QDir dir(PreviousPath);
    foreach (QFileInfo info, dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        QString name = info.baseName().mid(4,10);
        baseNames.append(name);
    }

    for(int i=0; i<baseNames.length(); i++){
        if(strncmp(Time.toLatin1().data(), baseNames.at(i).toLatin1().data(), 10) > 0 ){
            filename = QString("%1/log_%2.tar.xz").arg(PreviousPath).arg(baseNames.at(i));
            QFile file(filename);
            if(file.exists()){
                file.remove();
            }
        }
    }
}

void DataLog::diluteExternalLog()
{
    if(!APPSetting::IsHardDiskReady) return;
    QDir dir(PreviousPath);
    QFileInfo FileInfo;
    QStringList namelist;
    if(!dir.exists()){
        return;
    }
    namelist = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    int len = namelist.length();
    for(int i=0;i<len/5;i+=2){
        QString mfilename = dir.absoluteFilePath(namelist.at(i));
        QFile::remove(mfilename);
    }
}

void DataLog::PackTodayLog()
{
    if(!APPSetting::IsHardDiskReady) return;
    //先确定存储的容量是否足够，超过90就需要删除一部分再进行存储了
    int proportion = CoreHelper::Getcapacity("/udisk");
    if(proportion > APPSetting::MemorySpaceRatio){
        qDebug()<<"This udisk is full";
        DataLog::Instance()->diluteExternalLog();
    }else{
        qDebug()<<"/udisk proportion = " << proportion;
    }

    QString newname = QString("%1/log_%2.tar.xz").arg(PreviousPath).arg(QDATE);
    CompressThread *compress = new CompressThread(this);

    compress->SetFileName(newname);
    QStringList list;
    list << RunDataPath;
    compress->SetTarpath(LogPath);
    compress->SetPathlist(list);
    compress->start();

}
#endif
void DataLog::AddData(QString filename, QByteArray data)
{
#ifndef DEVICE_SLAVE
    if(!APPSetting::IsHardDiskReady) return;
#endif
    if(!CanSave) return;
    int proportion = CoreHelper::Getcapacity("/");
    if(proportion > 60) return;//本地最多存储60%
//    qDebug()<<"AddData len = " << data.size();
    QString name = QString("%1/%2").arg(RunDataPath).arg(filename);
    QFile file;
    file.setFileName(name);
    if(file.open(QFile::WriteOnly)){
        QDataStream in(&file);
        in.writeRawData(data.data(),data.size());
        file.flush();
        file.close();
        DBData::TCPAddSendFile(name);
    }
}

bool DataLog::isFileMatchConditions(const QString &fileName, const QString &wagon, int preID, int channel)
{
    // 假设文件名格式为：数据类型_车厢号_前置ID_通道号_时间.bin
    // 提取文件名中的车厢号、前置ID、通道号
    QStringList parts = fileName.split('_');
    if (parts.size() < 5) return false; // 格式不对

    QString fileWagon = parts.at(1); // 车厢号
    int filePreID = parts.at(2).toInt(); // 前置ID
    int fileChannel = parts.at(3).toInt(); // 通道号

    // 过滤车厢号、前置ID、通道号
    if ((wagon != "All" && fileWagon != wagon) ||
            (preID != 0 && filePreID != preID) ||
            (channel != 0 && fileChannel != channel)) {
        return false;
    }

    return true;
}

bool DataLog::isFileInRange(const QString &fileName, const QString &startTime, const QString &endTime)
{
    // 假设文件名格式为：数据类型_车厢号_前置ID_通道号_时间.bin
    QStringList parts = fileName.split('_');
    if (parts.size() < 5) return false; // 格式不对

    QString fileTime = parts.at(4).split(".").at(0); // 获取时间部分 (去掉后缀)

    // 将文件时间转换为 QDateTime 对象
    QDateTime fileDateTime = QDateTime::fromString(fileTime, "yyyyMMddHHmmss");
    QDateTime startDateTime = QDateTime::fromString(startTime, "yyyy-MM-dd HH:mm:ss");
    QDateTime endDateTime = QDateTime::fromString(endTime, "yyyy-MM-dd HH:mm:ss");
//            qDebug()<<"isFileInRange: fileDateTime " << fileTime << "start " <<startTime << "end " << endTime;
    return (fileDateTime >= startDateTime && fileDateTime <= endDateTime);
}

void DataLog::InstallHook()
{
    if(initializeLogFile()){
        qInstallMessageHandler(FunLogHook);
    }
}

void DataLog::UnInstatllHook()
{
    qInstallMessageHandler(0);
}
