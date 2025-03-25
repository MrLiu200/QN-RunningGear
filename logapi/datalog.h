#ifndef DATALOG_H
#define DATALOG_H

#include <QObject>
#include <QMutex>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QtCore>
#include "corehelper.h"
#include "compressthread.h"

class DataLog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString LogHookPath READ GetHookPath WRITE SetHookPath)
    Q_PROPERTY(QString LogHookName READ GetHookName WRITE SetHookName)
    Q_PROPERTY(QString LogPath READ GetLogPath WRITE SetLogPath)
    Q_PROPERTY(QString RunDataPath READ GetRunDataPath WRITE SetRunDataPath)
    Q_PROPERTY(QString PreviousPath READ GetPreviousPath WRITE SetPreviousPath)
public:
    explicit DataLog(QObject *parent = nullptr);
    static DataLog *Instance();

private:
    static DataLog *self;
    QString LogHookpath;        //日志钩子文件路径
    QString LogHookname;        //日志钩子文件名称
//    static QFile *logFile;             //日志钩子文件
    QString LogPath;            //日志总路径
    QString RunDataPath;        //运行数据路径
    QString PreviousPath;       //压缩文件路径
    bool CanSave;               //是否可以保存日志


private:
    static void FunLogHook(QtMsgType type, const QMessageLogContext &, const QString &msg);
    static bool initializeLogFile();
public:
    //获取、设置日志钩子路径
    QString GetHookPath() const;
    void SetHookPath(QString Path);

    //获取、设置日志钩子名称
    QString GetHookName() const;
    void SetHookName(QString Name);

    //获取、设置日志总路径
    QString GetLogPath() const;
    void SetLogPath(QString Path);

    //获取、设置运行数据路径
    QString GetRunDataPath() const;
    void SetRunDataPath(QString Path);

    //获取、设置压缩文件路径
    QString GetPreviousPath() const;
    void SetPreviousPath(QString Path);

    //存储路径初始化
    bool ParameterInit();
#ifndef DEVICE_SLAVE
    //删除指定日期之前的压缩文件
    void DeleteExternalLog(QString Time);

    //按比例稀释压缩文件
    void diluteExternalLog(void);

    //打包当天所有日志
    void PackTodayLog(void);
#endif
    //新增运行数据
    //name 格式: 振动数据：数据类型_车厢号_前置ID_通道号_时间.bin 例如 vib_2_1_2_20240101133000254.bin
    // 温度数据：数据类型_车厢号_前置ID_时间_.bin 如：tem_2_1_20240101133000254.bin
    void AddData(QString filename, QByteArray data);

    //筛选数据
    bool isFileMatchConditions(const QString &fileName, const QString &wagon, int preID, int channel);
    bool isFileInRange(const QString &fileName, const QString &startTime, const QString &endTime);

public slots:
    void InstallHook();//安装日志钩子,输出调试信息到文件,便于调试
    void UnInstatllHook();//卸载日志钩子
signals:

};

#endif // DATALOG_H
