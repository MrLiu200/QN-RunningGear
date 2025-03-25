#include "udpslotapi.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
UDPSlotAPI *UDPSlotAPI::self = 0;
UDPSlotAPI *UDPSlotAPI::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if(!self){
            self = new UDPSlotAPI;
        }
    }
    return self;
}

UDPSlotAPI::UDPSlotAPI(QObject *parent) : QObject(parent)
{
}

void UDPSlotAPI::Init()
{
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddLog,this,&UDPSlotAPI::UDPAddLog);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddLog1,this,&UDPSlotAPI::UDPAddLog1);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddEigenvalue,this,&UDPSlotAPI::UDPAddEigenvalue);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddBearing,this,&UDPSlotAPI::UDPAddBearing);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPDeleteBearing,this,&UDPSlotAPI::UDPDeleteBearing);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddDevice,this,&UDPSlotAPI::UDPAddDevice);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPDeleteDevice,this,&UDPSlotAPI::UDPDeleteDevice);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddTask,this,&UDPSlotAPI::UDPAddTask);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPDeleteTasks,this,&UDPSlotAPI::UDPDeleteTasks);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPAddLinkDevice,this,&UDPSlotAPI::UDPAddLinkDevice);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPDeleteLinkDevice,this,&UDPSlotAPI::UDPDeleteLinkDevice);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::RunSQLStr,this,&UDPSlotAPI::RunSQLStr);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPDeleteDimensional,this,&UDPSlotAPI::UDPDeleteDimensional);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPClearAlarm,this,&UDPSlotAPI::UDPClearAlarm);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::UDPReboot,this,&UDPSlotAPI::UDPReboot);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::TestAddPRE,this,&UDPSlotAPI::UDPTestAddPRE);
}

void UDPSlotAPI::UDPAddLog(const QString &carNumber, const QString &WagonNumber, const int &DeviceID, const int &DeviceCh,
                           const QString &DeviceName, const QString &LogType, const int &AlarmGrade, const QString &TriggerTime, const QString &LogContent)
{
    M_DataBaseAPI::addLog(carNumber,WagonNumber,DeviceID,DeviceCh,DeviceName,LogType,AlarmGrade,TriggerTime,LogContent);
}

void UDPSlotAPI::UDPAddLog1(const QString &carNumber, const QString &WagonNumber, const QString &LogType, const QString &TriggerTime, const QString &LogContent)
{
    M_DataBaseAPI::addLog(carNumber,WagonNumber,LogType,TriggerTime,LogContent);
}

void UDPSlotAPI::UDPAddEigenvalue(QString Wagon, qint8 id, qint8 ch, uint32_t speed, double AmbientTem, double PointTem, QString time,
                                  QVector<float> Dimensional, QVector<float> Demodulated)
{
    M_DataBaseAPI::AddDimensionalValue(Wagon,id,ch,speed,AmbientTem,PointTem,time,Dimensional);
    M_DataBaseAPI::AddDemodulatedValue(Wagon,id,ch,speed,AmbientTem,PointTem,time,Demodulated);
}

void UDPSlotAPI::UDPAddBearing(QString modelname, float PitchDiameter, float RollingDiameter, int RollerNum, float ContactAngle)
{
    M_DataBaseAPI::AddBearingInfo(modelname,PitchDiameter,RollingDiameter,RollerNum,ContactAngle);
}

void UDPSlotAPI::UDPDeleteBearing(QString BearingName)
{
    M_DataBaseAPI::DeleteBearingInfo(BearingName);
}

void UDPSlotAPI::UDPAddDevice(int DeviceId, int DeviceCH, QString DeviceName, QString DeviceType, float DeviceSensitivity, float ShaftDiameter,
                              QString Bearing1Typedef, QString Bearing1_model, QString Bearing2Typedef, QString Bearing2_model, QString Bearing3Typedef,
                              QString Bearing3_model, QString Bearing4Typedef, QString Bearing4_model, QString capstanName, int capstanTeethNum,
                              QString DrivenwheelName, int DrivenwheelTeethNum, QString version, bool Enable)
{
    M_DataBaseAPI::AddDeviceInfo(DeviceId,DeviceCH,DeviceName,DeviceType,DeviceSensitivity,ShaftDiameter,Bearing1Typedef,Bearing1_model,Bearing2Typedef,Bearing2_model,
                                 Bearing3Typedef,Bearing3_model,Bearing4Typedef,Bearing4_model,capstanName,capstanTeethNum,DrivenwheelName,DrivenwheelTeethNum,
                                 version,Enable);
}

void UDPSlotAPI::UDPDeleteDevice(int DeviceId, int DeviceCH)
{
    M_DataBaseAPI::DeleteDeviceInfo(DeviceId,DeviceCH);
}

void UDPSlotAPI::UDPAddTask(QString Action, QString TriggerTime)
{
    M_DataBaseAPI::AddTaskInfo(Action,TriggerTime);
}

void UDPSlotAPI::UDPDeleteTasks(QString action)
{
    M_DataBaseAPI::DeleteTasks(action);
}

void UDPSlotAPI::UDPAddLinkDevice(QString Type, QString IP, int Port, QString WagonNumber)
{
    M_DataBaseAPI::AddLinkDeviceInfo(Type,IP,Port,WagonNumber);
}

void UDPSlotAPI::UDPDeleteLinkDevice(QString IP)
{
    M_DataBaseAPI::DeleteLinkDeviceInfo(IP);
}

void UDPSlotAPI::RunSQLStr(QString str)
{
    if(str.isEmpty()) return;
//    qDebug()<<"RunSQLStr : " << str;
    QSqlQuery query;
    query.exec(str);
}

void UDPSlotAPI::UDPDeleteDimensional(QString Deadlinetime)
{
    if(Deadlinetime.isEmpty()) return;
    // 使用参数化查询
    QString str1 = "DELETE FROM DemodulatedValue WHERE TriggerTime < :deadline";
    QString str2 = "DELETE FROM DimensionalValue WHERE TriggerTime < :deadline";

    QSqlQuery query;
    query.prepare(str1);
    query.bindValue(":deadline", Deadlinetime);
    query.exec();

    query.prepare(str2);
    query.bindValue(":deadline", Deadlinetime);
    query.exec();

}

void UDPSlotAPI::UDPClearAlarm(qint8 id, qint8 ch)
{
    //更新前置列表内容
    int count = DBData::PreStatus.size();
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    DBData::DeviceState alarmgrade = DBData::DeviceState_FirstAlarm;
    for(int i=0; i < count; i++){
        QStringList list = DBData::PreStatus.at(i).split("|");
        QString Wagon = list.at(0);
        int oldid = list.at(1).toInt();
        int oldch = list.at(2).toInt();
        DBData::DeviceState oldState = static_cast<DBData::DeviceState>(list.at(3).toInt());
        DBData::DeviceState oldAlarm = static_cast<DBData::DeviceState>(list.at(4).toInt());
        QString oldtime = list.at(5);
        if((id == oldid) && (ch == oldch) && (oldAlarm != DBData::DeviceState_Online)){
            alarmgrade = oldAlarm;
            QString str = QString("%1|%2|%3|%4|0|%5").arg(Wagon).arg(oldid).arg(oldch).arg(oldState).arg(oldtime);
            DBData::PreStatus[i] = str;
            break;
        }
    }
    //更新相对应的指示灯
    if(alarmgrade == DBData::DeviceState_FirstAlarm){
        //一级报警
        CoreHelper::SetLedState(APPSetting::Led_alarm1,0);
    }else if(alarmgrade == DBData::DeviceState_Secondary){
        //二级报警
        CoreHelper::SetLedState(APPSetting::Led_alarm2,0);
    }
    DBData::CurrentAlarmLevel = -1;
}

void UDPSlotAPI::UDPReboot()
{
    Bus2API::Instance()->SetPreIOPowerState(false);
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"上位机控制重启软件");
    QTimer::singleShot(5000,this,&UDPSlotAPI::reboot);
}

void UDPSlotAPI::reboot()
{
    CoreHelper::Reboot();
}

void UDPSlotAPI::UDPTestAddPRE(QString str)
{
    M_DataBaseAPI::ClearTable(str);
#if 0
    // 将 QByteArray 转换为 QString
    QString jsonString = str;

    // 解析 JSON 字符串
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!jsonDoc.isNull()) {
        if (jsonDoc.isArray()) {
            QJsonArray jsonArray = jsonDoc.array();
            qDebug()<<"json isArray";
            if (!jsonArray.isEmpty()) {
                M_DataBaseAPI::ClearTable("DeviceInfo"); // 仅在有数据时清空表
            }

            for (const QJsonValue &value : jsonArray) {
                if (!value.isObject()) continue;
                QJsonObject obj = value.toObject();
                qDebug() << "Parsed JSON Object:";
                int DeviceId = obj.contains("前置ID") ? obj["前置ID"].toInt() : -1;
                int DeviceCH = obj.contains("通道号") ? obj["通道号"].toInt() : -1;
                QString DeviceName = obj.contains("测点名称") ? obj["测点名称"].toString() : "未定义";
                QString DeviceType = obj.contains("测点类型") ? obj["测点类型"].toString() : "轴箱";
                float DeviceSensitivity = obj.contains("灵敏度") ? obj["灵敏度"].toDouble() : 10;
                float ShaftDiameter = obj.contains("轴径") ? obj["轴径"].toDouble() : 22;
                QString Bearing1Typedef = obj.contains("轴承1名称") ? obj["轴承1名称"].toString() : "未知";
                QString Bearing1_model = obj.contains("轴承1型号") ? obj["轴承1型号"].toString() : "QNNULL";
                QString Bearing2Typedef = obj.contains("轴承2名称") ? obj["轴承2名称"].toString() : "未知";
                QString Bearing2_model = obj.contains("轴承2型号") ? obj["轴承2型号"].toString() : "QNNULL";
                QString Bearing3Typedef = obj.contains("轴承3名称") ? obj["轴承3名称"].toString() : "未知";
                QString Bearing3_model = obj.contains("轴承3型号") ? obj["轴承3型号"].toString() : "QNNULL";
                QString Bearing4Typedef = obj.contains("轴承4名称") ? obj["轴承4名称"].toString() : "未知";
                QString Bearing4_model = obj.contains("轴承4型号") ? obj["轴承4型号"].toString() : "QNNULL";
                QString capstanName = obj.contains("从动轮名称") ? obj["从动轮名称"].toString() : "未知";
                int capstanTeethNum = obj.contains("从动轮齿数") ? obj["从动轮齿数"].toInt() : 10;
                QString DrivenwheelName = obj.contains("主动轮名称") ? obj["主动轮名称"].toString() : "未知";
                int DrivenwheelTeethNum = obj.contains("主动轮齿数") ? obj["主动轮齿数"].toInt() : 0;
                QString version = obj.contains("软件版本") ? obj["软件版本"].toString() : "v1.0.0";
                bool Enable = obj.contains("是否使能") ? obj["是否使能"].toBool() : true;
                M_DataBaseAPI::AddDeviceInfo(DeviceId,DeviceCH,DeviceName,DeviceType,DeviceSensitivity,ShaftDiameter,Bearing1Typedef,Bearing1_model,
                                             Bearing2Typedef,Bearing2_model,Bearing3Typedef,Bearing3_model,Bearing4Typedef,Bearing4_model,capstanName,
                                             capstanTeethNum,DrivenwheelName,DrivenwheelTeethNum,version,Enable);
                for (QString key : obj.keys()) {
                    qDebug() << key << ":" << obj.value(key).toString();
                }
            }
        } else if (jsonDoc.isObject()) {
            QJsonObject jsonObject = jsonDoc.object();
            qDebug() << "Received JSON Object:";

            for (QString key : jsonObject.keys()) {
                qDebug() << key << ":" << jsonObject.value(key).toVariant();
            }
        }
    } else {
        qDebug() << "Invalid JSON received";
    }
#endif
}
