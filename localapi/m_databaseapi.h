#ifndef M_DATABASEAPI_H
#define M_DATABASEAPI_H

#include <QObject>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QFile>
#include <QTextStream>
#include "corehelper.h"

class M_DataBaseAPI : public QObject
{
    Q_OBJECT
public:
    explicit M_DataBaseAPI(QObject *parent = nullptr);

    //获取指定表名字段数据内容
    static QStringList getContent(const QString &tableName,
                                  const QString &columnNames,
                                  const QString &whereSql = QString());

    //导入数据文件
    static bool inputData(int columnCount,
                          const QString &filter,
                          const QString &tableName,
                          QString &fileName,
                          const QString &defaultDir = QCoreApplication::applicationDirPath());

    //导出数据文件
    static bool outputData(const QString &defaultName,
                           const QString &filter,
                           const QStringList &content,
                           QString &fileName,
                           const QString &defaultDir = QCoreApplication::applicationDirPath());

    //导出数据
    static bool outputData(QString &fileName,const QStringList &content);

    //加载轴承参数
    static void LoadBearingInfo();

    //加载定时任务
    static void LoadTask();

    //载入前置处理器设备信息
    static void LoadDeviceInfo(void);

    //加载管理设备信息
    static void LoadLinkDeviceInfo(void);

    //设备信息写入文件
    static bool saveDeviceInfoToFile(QString filename);

    //添加日志文件
    static void addLog(const QString &carNumber, const QString &WagonNumber, const int &DeviceID, const int &DeviceCh, const int AxisPoint, const QString &DeviceName,
                       const QString &LogType, const int &AlarmGrade, const QString &TriggerTime, const QString &LogContent = QString());
    static void addLog(const QString &carNumber, const QString &WagonNumber, const QString &LogType, const QString &TriggerTime, const QString &LogContent = QString());

    //添加量纲值
    static void AddDimensionalValue(QString Wagon, qint8 id, qint8 ch, uint32_t speed, double Ambienttem, double pointtem, QString time, QVector<float> Value);

    //添加包络量纲值
    static void AddDemodulatedValue(QString Wagon, qint8 id, qint8 ch, uint32_t speed, double Ambienttem, double pointtem, QString time, QVector<float> Value);

    //删除指定时间期限内的量纲值
    static void DeleteDimensionalValue(QString timeStart, QString timeEnd);

    //获取指定时间内的量纲值文件
    static QString GetDimensionalCSV(QString timeStart, QString timeEnd);

    //新增前置处理器
    static void AddDeviceInfo(int DeviceId, int DeviceCH, QString DeviceName, QString DeviceType, float DeviceSensitivity, int AxisPosition, float ShaftDiameter,
                              QString Bearing1Typedef, QString Bearing1_model, QString Bearing2Typedef, QString Bearing2_model, QString Bearing3Typedef,
                              QString Bearing3_model, QString Bearing4Typedef, QString Bearing4_model, QString capstanName, int capstanTeethNum,
                              QString DrivenwheelName, int DrivenwheelTeethNum, QString version, bool Enable);

    //删除前置处理器
    static void DeleteDeviceInfo(int DeviceId, int DeviceCH);

    //新增轴承信息
    static void AddBearingInfo(QString modelname, float PitchDiameter, float RollingDiameter, int RollerNum, float ContactAngle);

    //删除轴承信息
    static void DeleteBearingInfo(QString BearingName);

    //添加定时任务
    static void AddTaskInfo(QString Action, QString TriggerTime);

    //删除定时任务
    static void DeleteTasks(QString action);

    //新增关联设备
    static void AddLinkDeviceInfo(QString Type, QString IP, int Port, QString WagonNumber);

    //删除关联设备
    static void DeleteLinkDeviceInfo(QString IP);

    //清空表
    static void ClearTable(QString tabname);

    //更新前置处理器软件版本号
    static void UpdateVersion(quint8 id, quint8 ch, QString newversion);

signals:

};

#endif // M_DATABASEAPI_H
