#include "taskapi.h"
#include <QMutex>

TaskAPI *TaskAPI::self = 0;
TaskAPI *TaskAPI::Instance()
{
    if (!self) {
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!self) {
            self = new TaskAPI;
        }
    }

    return self;
}

TaskAPI::TaskAPI(QObject *parent) : QObject(parent)
{
    checkListInit();
    timerCheckCan = new QTimer(this);
    timerCheckCan->setInterval(APPSetting::CheckCanInterval);
    connect(timerCheckCan,&QTimer::timeout,this,&TaskAPI::CheckCanTask);
#ifndef DEVICE_SLAVE
    timerCheckSlave = new QTimer(this);
    timerCheckSlave->setInterval(APPSetting::CheckSlaceInterval);
    connect(timerCheckSlave,&QTimer::timeout,this,&TaskAPI::CheckSlaveTask);
#endif
    timerOtherBoard = new QTimer(this);
    timerOtherBoard->setInterval(APPSetting::CheckOtherBoardInterval);
    connect(timerOtherBoard,&QTimer::timeout,this,&TaskAPI::CheckOtherBoard);
    timerTask = new QTimer(this);
    timerTask->setInterval(APPSetting::CheckOtherTaskInterval);
    connect(timerTask,&QTimer::timeout,this,&TaskAPI::CheckTimerTask);
}

void TaskAPI::checkListInit()
{
    //can总线设备列表初始化
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    for(int i=0;i<DBData::DeviceInfo_Count;i++){
        if(DBData::DeviceInfo_IsEnable.at(i)){
            int id = DBData::DeviceInfo_DeviceID.at(i);
            int ch = DBData::DeviceInfo_DeviceCH.at(i);
            QString str = QString("%1|%2|%3|0|0|%4").arg(APPSetting::WagonNumber).arg(id).arg(ch).arg(DATETIME);
            DBData::PreStatus.append(str);
        }
    }

#ifndef DEVICE_SLAVE
    //从机列表初始化
    //车厢号|车厢类型|当前状态|最后一个心跳时间
    for(int i=0;i<DBData::LinkDeviceInfo_Type.size();i++){
        QString Type  = DBData::LinkDeviceInfo_Type.at(i);
        QString WagonNumber = DBData::LinkDeviceInfo_WagonNumber.at(i);
        QString str = QString("%1|%2|0|%3").arg(WagonNumber).arg(Type).arg(DATETIME);
        DBData::SlaveStatus.append(str);
    }
#endif
    //背板设备初始化
    QStringList list;
#ifdef DEVICE_SLAVE
    list.append(QString::number(COMMUNICATIONBOARDID));
    list.append(QString::number(DBData::DeviceState_Online));
    list.append(DATETIME);
    DBData::OtherBoardStatus.append(list.join("|"));
    list[0] = QString::number(PREIOBOARDID);
    DBData::OtherBoardStatus.append(list.join("|"));
#else
    list.append(QString::number(SPEEDBOARDID));
    list.append(QString::number(DBData::DeviceState_Online));
    list.append(DATETIME);
    DBData::OtherBoardStatus.append(list.join("|"));
    list[0] = QString::number(COMMUNICATIONBOARDID);
    DBData::OtherBoardStatus.append(list.join("|"));
    list[0] = QString::number(PREIOBOARDID);
    DBData::OtherBoardStatus.append(list.join("|"));
#endif
}

void TaskAPI::CheckCanTask()
{
    int count = DBData::PreStatus.size();
    for(int i=0; i<count; i++){
        QStringList list = DBData::PreStatus.at(i).split("|");
        QString time = list.at(list.size()-1);
        QDateTime oldtime = QDateTime::fromString(time,"yyyy-MM-dd HH:mm:ss");
        QDateTime nowtime = QDateTime::currentDateTime();
        if(oldtime.secsTo(nowtime) >= APPSetting::TimeoutInterval_Pre){
//            if(list[3].toInt() == DBData::DeviceState_Online){
            if((list[3].toInt() == DBData::DeviceState_Online) && APPSetting::UseOfflineAlarm){
                int id = list.at(1).toInt();
                int ch = list.at(2).toInt();
                addErrorprelist(id,ch);

//                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,QString("preid = %1,ch = %2 掉线")
//                                      .arg(list.at(1)).arg(list.at(2)));
                int index = DBData::GetDeviceIndex(id,ch);
                int axis = (index == -1? -1 : DBData::DeviceInfo_AxisPosition.at(index));
                QString devicename = (index == -1? "---" : DBData::DeviceInfo_DeviceName.at(index));
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,devicename,"报警信息",
                                      -1,DATETIME,QString("preid = %1,ch = %2 掉线").arg(id).arg(ch));
                list[3] = QString::number(DBData::DeviceState_Offline);
                DBData::PreStatus[i] = list.join("|");
                Q_EMIT PreStateChange(list.at(1).toInt(),list.at(2).toInt(),devicename,axis,DBData::DeviceState_Offline);
            }
        }
    }
}

void TaskAPI::CheckSlaveTask()
{
#ifndef DEVICE_SLAVE
    int count = DBData::SlaveStatus.size();
    for(int i=0; i<count; i++){
        QStringList list = DBData::SlaveStatus.at(i).split("|");
        QString type = list.at(1);
        QString time = list.at(list.size()-1);
        QDateTime oldtime = QDateTime::fromString(time,"yyyy-MM-dd HH:mm:ss");
        QDateTime nowtime = QDateTime::currentDateTime();
        if(oldtime.secsTo(nowtime) >= APPSetting::TimeoutInterval_Slave){
            if(list[2].toInt() == DBData::DeviceState_Online){
                list[2] = QString::number(DBData::DeviceState_Offline);
                DBData::SlaveStatus[i] = list.join("|");
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,
                                      QString("%1(车厢号:%2)掉线").arg(list.at(1)).arg(list.at(0)));
                if(type == "Host"){
                    Q_EMIT HostOffline();
                }
            }
        }
    }
#endif
}

void TaskAPI::CheckOtherBoard()
{
    int count = DBData::OtherBoardStatus.size();
    for(int i=0; i<count; i++){
        QStringList list = DBData::OtherBoardStatus.at(i).split("|");
        QString time = list.at(list.size()-1);
        QDateTime oldtime = QDateTime::fromString(time,"yyyy-MM-dd HH:mm:ss");
        QDateTime nowtime = QDateTime::currentDateTime();
        if(oldtime.secsTo(nowtime) >= APPSetting::TimeoutInterval_OtherBrade){
            if(list[1].toInt() == DBData::DeviceState_Online){
                list[1] = QString::number(DBData::DeviceState_Offline);
                DBData::OtherBoardStatus[i] = list.join("|");
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,
                                      QString("背板(canid = %1) 掉线").arg(list.at(0)));
                Q_EMIT OtherBoardStateChange(list.at(0).toInt(),DBData::DeviceState_Offline);
            }
        }
    }
}

void TaskAPI::CheckTimerTask()
{
    int count = DBData::Task_TriggerTime.count();
    if(count == 0) return;
    for(int i=0;i<count;i++){
        QString triggerTime = DBData::Task_TriggerTime.at(i);
        QDateTime old = QDateTime::fromString(triggerTime,"yyyy-MM-dd HH:mm:ss");
        QDateTime now = QDateTime::currentDateTime();
        if(old.secsTo(now) >= 0){
            QDateTime tomorrow = old.addDays(1);
            QString str = tomorrow.toString("yyyy-MM-dd HH:mm:ss");
            DBData::Task_TriggerTime[i] = str;
            QString action = DBData::Task_Action.at(i);
            //判断命令
            if(action == "重启"){
                CoreHelper::Reboot();
            }else if(action == "压缩日志"){
#ifndef DEVICE_SLAVE
                DataLog::Instance()->PackTodayLog();
#endif
            }
        }
    }
}

bool TaskAPI::existErrorprelist(uint8_t preid, uint8_t ch)
{
    bool ret = false;
    if(errorPrelist.contains(preid)){
        QList<uint8_t> list = errorPrelist.value(preid);
        if(list.contains(ch)){
            ret = true;
        }
    }
    return ret;
}

void TaskAPI::removeErrorprelist(uint8_t preid, uint8_t ch)
{
    if(!existErrorprelist(preid,ch)) return;
    if (errorPrelist.contains(preid)) {
        QList<uint8_t>& channelList = errorPrelist[preid];
        channelList.removeOne(ch);
    }
    //判断errorPrelist是否还存在内容，若不存在故障前置，则检查是否已开指示灯
    if(errorPrelist.isEmpty() && DBData::PreIsAlarm){
        DBData::PreIsAlarm = false;
        CoreHelper::SetLedState(APPSetting::Led_PreError,1);
    }
}

void TaskAPI::addErrorprelist(uint8_t preid, uint8_t ch)
{
    //新增前置报警指示灯
    if(!DBData::PreIsAlarm){
        DBData::PreIsAlarm = true;
        CoreHelper::SetLedState(APPSetting::Led_PreError,0);
    }
    if(errorPrelist.contains(preid)){
        auto &list = errorPrelist[preid];
        if(!list.contains(ch)){
            list.append(ch);
        }else{
            return;
        }
    }else{
        QList<uint8_t> list;
        list.append(ch);
        errorPrelist.insert(preid,list);
    }
}

void TaskAPI::clearErrorPre()
{
    int count = DBData::PreStatus.size();
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    for(int i=0; i < count; i++){
        QStringList list = DBData::PreStatus.at(i).split("|");
        QString Wagon = list.at(0);
        int oldid = list.at(1).toInt();
        int oldch = list.at(2).toInt();
        QString str = QString("%1|%2|%3|%4|%5|%6").arg(Wagon).arg(oldid).arg(oldch).arg(0).arg(0).arg(DATETIME);
        DBData::PreStatus[i] = str;
        if(existErrorprelist(oldid,oldch)){
            removeErrorprelist(oldid,oldch);
        }
    }
}

void TaskAPI::clearErrorPre(uint8_t preid, uint8_t ch)
{
    int count = DBData::PreStatus.size();
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    for(int i=0; i < count; i++){
        QStringList list = DBData::PreStatus.at(i).split("|");
        QString Wagon = list.at(0);
        int oldid = list.at(1).toInt();
        int oldch = list.at(2).toInt();
        if((oldid == preid) && (oldch == ch)){
            QString str = QString("%1|%2|%3|%4|%5|%6").arg(Wagon).arg(oldid).arg(oldch).arg(0).arg(0).arg(DATETIME);
            DBData::PreStatus[i] = str;
            if(existErrorprelist(oldid,oldch)){
                removeErrorprelist(oldid,oldch);
            }
            break;
        }
    }
}
#ifndef DEVICE_SLAVE
void TaskAPI::clearErrorSlave()
{
    //数据格式：车厢号|车厢类型|当前状态|最后一个心跳时间
    int count = DBData::SlaveStatus.size();
    for(int i=0; i < count; i++){
        QStringList list = DBData::SlaveStatus.at(i).split("|");
        QString Wagon = list.at(0);
        QString type = list.at(1);
        QString str = QString("%1|%2|%3|%4").arg(Wagon).arg(0).arg(0).arg(DATETIME);
        DBData::SlaveStatus[i] = str;
    }
}

void TaskAPI::clearErrorSlave(QString Wagon)
{
    int count = DBData::SlaveStatus.size();
    for(int i=0; i < count; i++){
        QStringList list = DBData::SlaveStatus.at(i).split("|");
        QString oldWagon = list.at(0);
        QString type = list.at(1);
        if(oldWagon == Wagon){
            QString str = QString("%1|%2|%3|%4").arg(Wagon).arg(0).arg(0).arg(DATETIME);
            DBData::SlaveStatus[i] = str;
        }
    }
}
#endif
void TaskAPI::clearErrorOtherBoard()
{
    //数据格式：板卡帧ID|当前状态|最后一次心跳时间
    int count = DBData::OtherBoardStatus.size();
    for(int i=0; i < count; i++){
        QStringList list = DBData::OtherBoardStatus.at(i).split("|");
        int id = list.at(0).toInt();
        QString str = QString("%1|%2|%3").arg(id).arg(0).arg(0).arg(DATETIME);
        DBData::OtherBoardStatus[i] = str;
    }
}

void TaskAPI::clearErrorOtherBoard(uint8_t id)
{
    int count = DBData::OtherBoardStatus.size();
    for(int i=0; i < count; i++){
        QStringList list = DBData::OtherBoardStatus.at(i).split("|");
        int oldid = list.at(0).toInt();
        if(id == oldid){
         QString str = QString("%1|%2|%3").arg(id).arg(0).arg(0).arg(DATETIME);
         DBData::OtherBoardStatus[i] = str;
        }
    }
}

void TaskAPI::Start()
{
    if(!timerCheckCan->isActive() && (!DBData::PreStatus.isEmpty())){
        timerCheckCan->start();
    }
#ifndef DEVICE_SLAVE
    if(!timerCheckSlave->isActive() && (!DBData::SlaveStatus.isEmpty())){
        timerCheckSlave->start();
    }
#endif
    if(!timerOtherBoard->isActive() && (!DBData::OtherBoardStatus.isEmpty())){
        timerOtherBoard->start();
    }
    if(!timerTask->isActive() && (!DBData::Task_TriggerTime.isEmpty())){
        timerTask->start();
    }
}

void TaskAPI::Stop()
{
    if(timerCheckCan->isActive()){
        timerCheckCan->stop();
    }
#ifndef DEVICE_SLAVE
    if(timerCheckSlave->isActive()){
        timerCheckSlave->stop();
    }
#endif
    if(timerOtherBoard->isActive()){
        timerOtherBoard->stop();
    }
    if(timerTask->isActive()){
        timerTask->stop();
    }
}
