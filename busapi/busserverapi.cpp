#include "busserverapi.h"
#include "udpmulticastapi.h"
#include "datalog.h"
#include "dbdata.h"
BusServerAPI *BusServerAPI::self = 0;
BusServerAPI *BusServerAPI::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker lock(&mutex);
        if(!self){
            self = new BusServerAPI;
        }
    }
    return self;
}

BusServerAPI::BusServerAPI(QObject *parent) : QObject(parent)
{
    timerTask = new QTimer;
    timerTask->setInterval(APPSetting::CanBusTaskInterval);
    connect(timerTask, &QTimer::timeout,this,&BusServerAPI::TaskMain);

    timerUpdate = new QTimer;
    timerUpdate->setInterval(APPSetting::CanBusUpdateTaskInterval);
    connect(timerUpdate, &QTimer::timeout,this,&BusServerAPI::TaskUpdate);

    currentRunMode = RUNMODE::InitMode;
}

void BusServerAPI::Init()
{
    if(Bus0API::Instance()->Open()){
        qDebug() << "Bus0 初始化成功";
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"Bus0初始化成功");
        connect(Bus0API::Instance(),&Bus0API::SelfInspection,this,&BusServerAPI::SelfInspection);
        connect(Bus0API::Instance(),&Bus0API::TemInspection,this,&BusServerAPI::TemInspection);
        connect(Bus0API::Instance(),&Bus0API::ReturnVIBData,this,&BusServerAPI::ReturnVIBData);
        connect(Bus0API::Instance(),&Bus0API::PreState,this,&BusServerAPI::PreState);
        connect(Bus0API::Instance(),&Bus0API::UpdateResults,this,&BusServerAPI::UpdateResults);
        connect(Bus0API::Instance(),&Bus0API::RebootState,this,&BusServerAPI::RebootState);
        connect(Bus0API::Instance(),&Bus0API::ReturnChannelStatus,this,&BusServerAPI::ReturnChannelStatus);
        //        QTimer::singleShot(300,this,SLOT(SelfInspection_Task()));
    }else{
        qDebug() << "Bus0 初始化失败";
    }

    if(Bus1API::Instance()->Open()){
        qDebug() << "Bus1 初始化成功";
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,"Bus1初始化成功");
        connect(Bus1API::Instance(),&Bus1API::SelfInspection,this,&BusServerAPI::SelfInspection);
        connect(Bus1API::Instance(),&Bus1API::TemInspection,this,&BusServerAPI::TemInspection);
        connect(Bus1API::Instance(),&Bus1API::ReturnVIBData,this,&BusServerAPI::ReturnVIBData);
        connect(Bus1API::Instance(),&Bus1API::PreState,this,&BusServerAPI::PreState);
        connect(Bus1API::Instance(),&Bus1API::UpdateResults,this,&BusServerAPI::UpdateResults);
        connect(Bus1API::Instance(),&Bus1API::RebootState,this,&BusServerAPI::RebootState);
        connect(Bus1API::Instance(),&Bus1API::ReturnChannelStatus,this,&BusServerAPI::ReturnChannelStatus);
    }else{
        qDebug() << "Bus1 初始化失败";
    }

    QTimer::singleShot(300,this,SLOT(SelfInspection_Task()));
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::PreSoftwareUpdate,this,&BusServerAPI::PreSoftwareUpdate);
    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::Signal_SelfInspection,this,&BusServerAPI::Slot_SelfInspection);
}

void BusServerAPI::TaskMain()
{
    double SpeedHour = GetCurrentSpeed();

    if(SpeedHour < APPSetting::LowPowerTaskSpeedHourMax){//低功耗模式
        if(currentRunMode != LowPower){
            qDebug()<<"--------------------- current SpeedHour = " << SpeedHour << ", Switch LowPower Task--------------------------";
            currentRunMode = LowPower;
            ResetState_Task();
        }
        LowPower_Task();
    }else if(SpeedHour < APPSetting::TmpTaskSpeedHourMax){//温度模式
        if(currentRunMode != Temperature){
            qDebug()<<"--------------------- current SpeedHour = " << SpeedHour << ", Switch TMP Task--------------------------";
            currentRunMode = Temperature;
            ResetState_Task();
        }
        TmpInspection_Task();
    }else{//综合模式
        if(currentRunMode != Comprehensive){
            qDebug()<<"--------------------- current SpeedHour = " << SpeedHour << ", Switch CMP Task--------------------------";
            currentRunMode = Comprehensive;
            ResetState_Task();
        }
        CmpInspection_Task();
    }
}

double BusServerAPI::GetCurrentSpeed()
{
    uint32_t Speed = DBData::RunSpeed;
//    qDebug()<<"转速(rpm): "<<DBData::RunSpeed;
    //周长 = 直径 * π *
    double perimeter = M_PI * APPSetting::WheelDiameter/1000;
    //    qDebug()<<"Speed = " << Speed << "perimeter = " << perimeter;
    //时速 = 转速 * 轮周长 * 60.0(60分钟)
    return static_cast<double>(Speed) * perimeter * 60.0;
}

void BusServerAPI::ResetState_Task()
{
    Bus1API::Instance()->ResetTaskState();
    Bus0API::Instance()->ResetTaskState();
    qDebug()<<"**************ResetState****************";
}

void BusServerAPI::LowPower_Task()
{
    Bus1API::Instance()->LowPowerMode();
    Bus0API::Instance()->LowPowerMode();
}

void BusServerAPI::TmpInspection_Task()
{//温度巡检模式
    Bus1_TmpTask();
    Bus0_TmpTask();
}

void BusServerAPI::CmpInspection_Task()
{//综合巡检模式
    Bus1_CmpTask();
    Bus0_CmpTask();
}

void BusServerAPI::UpdateAlarmState(uint8_t preid, uint8_t prech, uint8_t alarmgrade)
{
    //    将报警等级更改 0:预警，1：一级 ，2：二级
    alarmgrade += 2;

    int count = DBData::PreStatus.size();
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    for(int i=0; i < count; i++){
        QStringList list = DBData::PreStatus.at(i).split("|");
        QString Wagon = list.at(0);
        int oldid = list.at(1).toInt();
        int oldch = list.at(2).toInt();
        DBData::DeviceState oldState = static_cast<DBData::DeviceState>(list.at(3).toInt());
        DBData::DeviceState oldAlarm = static_cast<DBData::DeviceState>(list.at(4).toInt());
        if((preid == oldid) && (prech == oldch)){
            if(oldAlarm < alarmgrade){//报警等级不相同，更改状态
                QString str = QString("%1|%2|%3|%4|%5|%6").arg(Wagon).arg(oldid).arg(oldch).arg(oldState).arg(alarmgrade).arg(DATETIME);
                DBData::PreStatus[i] = str;
                break;
            }
        }
    }
}

void BusServerAPI::TaskUpdate()
{
    qDebug()<<"*-*-*-*-*-*-*-*-*    TaskUpdate   *-*-*-*-*-*-*-*-*";
    if(((m_updatestruct.bus0_pre.isEmpty()) && (m_updatestruct.bus1_pre.isEmpty())) || (m_updatestruct.UpdateFile.isEmpty())){
        qDebug()<<"无更新任务";
        timerUpdate->stop();
        currentRunMode = InitMode;
        StartServer();
        return;
    }

    if(m_updatestruct.bus0_flag == 0){
        qDebug()<<"can0 Update**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-";
        Bus0API::Instance()->CommandPreUpdate(m_updatestruct.bus0_pre.at(0),m_updatestruct.Updatecrc,m_updatestruct.filesize);
        m_updatestruct.bus0_flag = 1;
    }

    if(m_updatestruct.bus1_flag == 0){
        Bus1API::Instance()->CommandPreUpdate(m_updatestruct.bus1_pre.at(0),m_updatestruct.Updatecrc,m_updatestruct.filesize);
        m_updatestruct.bus1_flag = 1;
    }
}

void BusServerAPI::Bus0_TmpTask()
{
    if(!Bus0API::Instance()->BusIsOpen()) return;
    // 获取当前任务状态
    QByteArray arr = Bus0API::Instance()->GetTaskState();

    if (arr.isEmpty()) {
        qWarning() << "Bus0 No tasks available for inspection.";
        return; // 如果没有任务，直接返回
    }

    uint8_t id = arr[0]; // 当前的 pre_id
    uint8_t ch = arr[1]; // 当前的通道号 (不需要在这个模式中使用)
    uint8_t statusValue = static_cast<uint8_t>(arr[2]);
    TASKSTATE status = static_cast<TASKSTATE>(statusValue);
    qDebug()<< "Bus0 TmpInspection_Task" << "id = " << id << "ch =" << ch << "state = " << status;
    // 判断当前状态并决定是否需要切换到下一个 pre_id
    switch (status) {
    case TASKSTATE::idle: // 空闲，说明当前任务已经完成
        Bus0API::Instance()->CommandTemInspection(id);
        break;
    case TASKSTATE::Collecting: //正在采集，当前 pre_id 正在工作,切换到下一个 pre_id，不管通道号
        Bus0API::Instance()->switchNextTask();
        break;
    case TASKSTATE::Obtaining:
        break;
    case TASKSTATE::WaitingToObtain:
        break;
    default:
        break;
    }
}

void BusServerAPI::Bus1_TmpTask()
{
    if(!Bus1API::Instance()->BusIsOpen()) return;
    // 获取当前任务状态
    QByteArray arr = Bus1API::Instance()->GetTaskState();

    if (arr.isEmpty()) {
        qWarning() << "Bus1 No tasks available for inspection.";
        return; // 如果没有任务，直接返回
    }

    uint8_t id = arr[0]; // 当前的 pre_id
    uint8_t ch = arr[1]; // 当前的通道号 (不需要在这个模式中使用)
    uint8_t statusValue = static_cast<uint8_t>(arr[2]);
    TASKSTATE status = static_cast<TASKSTATE>(statusValue);
    qDebug()<< "Bus1 TmpInspection_Task" << "id = " << id << "ch =" << ch << "state = " << status;
    // 判断当前状态并决定是否需要切换到下一个 pre_id
    switch (status) {
    case TASKSTATE::idle: // 空闲，说明当前任务已经完成
        Bus1API::Instance()->CommandTemInspection(id);
        break;
    case TASKSTATE::Collecting: //正在采集，当前 pre_id 正在工作,切换到下一个 pre_id，不管通道号
        Bus1API::Instance()->switchNextTask();
        break;
    case TASKSTATE::Obtaining:
        break;
    case TASKSTATE::WaitingToObtain:
        break;
    default:
        break;
    }
}

void BusServerAPI::Bus0_CmpTask()
{
    if(!Bus0API::Instance()->BusIsOpen()) return;
    //读取当前任务
    QByteArray arr = Bus0API::Instance()->GetTaskState();
    if (arr.isEmpty()) {
        qWarning() << "Bus0 No tasks available for inspection.";
        return;
    }
    //判断任务状态
    uint8_t id = arr[0];
    uint8_t ch = arr[1];
    uint8_t statusValue = static_cast<uint8_t>(arr[2]);
    TASKSTATE status = static_cast<TASKSTATE>(statusValue);

    // 检查当前通道是否在离线列表中
    if (TaskAPI::Instance()->existErrorprelist(id, ch)) {
        qWarning() << "Bus0 Current channel" << ch << "in preprocessor" << id << "is faulty, switching to the next channel.";
        Bus0API::Instance()->SetTaskState(id,TASKSTATE::WaitingSwitch);//因为只有空闲才可以切换
        Bus0API::Instance()->switchNextTask();
        //重新发送自检信息确认在线
        Bus0API::Instance()->CommandSelfInspection(id);
        //2025-01-20修改为单通道确认是否正常
//        Bus0API::Instance()->QueryCHState(id,ch);
        return; // 切换后直接返回
    }
    qDebug()<<DATETIMES << "Bus0 Task id = " << id << " ch = " << ch << " status = " << status ;
    switch (status) {
    case idle://空闲
        Bus0API::Instance()->CommandVibInspection(id,ch);
        break;
    case Collecting://正在采集
        Bus0API::Instance()->switchNextTask();
        break;
    case Obtaining://获取中
        //        Bus0API::Instance()->switchNextTask();
        break;
    case WaitingToObtain://等待获取
        Bus0API::Instance()->CommandReadVibData(id,ch);
        break;
    default:
        break;
    }
}

void BusServerAPI::Bus1_CmpTask()
{
    if(!Bus1API::Instance()->BusIsOpen()) return;
    //读取当前任务
    QByteArray arr = Bus1API::Instance()->GetTaskState();
    if (arr.isEmpty()) {
        qWarning() << "Bus1 No tasks available for inspection.";
        return;
    }
    //判断任务状态
    uint8_t id = arr[0];
    uint8_t ch = arr[1];
    uint8_t statusValue = static_cast<uint8_t>(arr[2]);
    TASKSTATE status = static_cast<TASKSTATE>(statusValue);

    // 检查当前通道是否在故障列表中
    if (TaskAPI::Instance()->existErrorprelist(id, ch)) {
        qWarning() << "Bus1 Current channel" << ch << "in preprocessor" << id << "is faulty, switching to the next channel.";
        Bus1API::Instance()->SetTaskState(id,TASKSTATE::WaitingSwitch);//因为只有空闲才可以切换
        Bus1API::Instance()->switchNextTask();
        //重新发送自检信息确认在线
        Bus1API::Instance()->CommandSelfInspection(id);
        //2025-01-20修改为单通道确认是否正常
//        Bus1API::Instance()->QueryCHState(id,ch);
        return; // 切换后直接返回
    }
    qDebug()<<DATETIMES << "Bus1 Task id = " << id << " ch = " << ch << " status = " << status ;
    switch (status) {
    case idle://空闲
        Bus1API::Instance()->CommandVibInspection(id,ch);
        break;
    case Collecting://正在采集
        Bus1API::Instance()->switchNextTask();
        break;
    case Obtaining://获取中
        //        Bus1API::Instance()->switchNextTask();
        break;
    case WaitingToObtain://等待获取
        Bus1API::Instance()->CommandReadVibData(id,ch);
        break;
    default:
        break;
    }
}

void BusServerAPI::SetAlarmState(uint8_t level)
{
    if(level > 2) return;
    //与现在的报警等级做对比
    if(level > DBData::CurrentAlarmLevel){
        //更新报警等级，判断若原先是1，现在是2则需要关闭原来的指示灯
        if(DBData::CurrentAlarmLevel == 1){
            CoreHelper::SetLedState(APPSetting::Led_alarm1,0);
        }

        //记录当前报警状态
        DBData::CurrentAlarmLevel = level;

        //设置新的报警指示灯
        QString ledname;
        switch (level) {
        case 1:
            ledname = APPSetting::Led_alarm1;
            break;
        case 2:
            ledname = APPSetting::Led_alarm2;
            break;
        default:
            break;
        }
        if(ledname.isEmpty()) return;
        CoreHelper::SetLedState(ledname,1);
    }
}

void BusServerAPI::SelfInspection_Task()
{
    Bus0API::Instance()->CommandSelfInspection(0xff);
    Bus1API::Instance()->CommandSelfInspection(0xff);
    QTimer::singleShot(10000,this,SLOT(StartServer()));
}

void BusServerAPI::StartServer()
{
    if(!timerTask->isActive()){
        qDebug()<<"**************Start Server*******************";
        timerTask->setInterval(2000);
        timerTask->start();
    }
}

void BusServerAPI::StopServer()
{
    if(timerTask->isActive()){
        qDebug()<<"**************Stop Server*******************";
        timerTask->stop();
    }
}

void BusServerAPI::PreSoftwareUpdate(const QString filename, const uint32_t crc, uint8_t deviceID)
{
    //软件升级先暂停其他业务
    StopServer();
    ResetState_Task();
    bool start = false;
    m_updatestruct.UpdateFile = filename;
    m_updatestruct.Updatecrc = crc;
    m_updatestruct.filesize = QFileInfo(filename).size();
    qDebug()<<"UpdateFile = " << m_updatestruct.UpdateFile << "Updatecrc = " <<m_updatestruct.Updatecrc << "filesize ="<< m_updatestruct.filesize;
    //查找设备属于哪个通道
    if(deviceID == 0){//全部更新
        m_updatestruct.bus0_pre = Bus0API::Instance()->GetPreDeviceList();
        m_updatestruct.bus1_pre = Bus1API::Instance()->GetPreDeviceList();
        if(!m_updatestruct.bus0_pre.isEmpty()){
            m_updatestruct.bus0_flag = 0;
        }
        if(!m_updatestruct.bus1_pre.isEmpty()){
            m_updatestruct.bus1_flag = 0;
        }
        start = true;
    }else{
        if(Bus0API::Instance()->TaskIsContain(deviceID)){
            m_updatestruct.bus0_pre.append(deviceID);
            m_updatestruct.bus0_flag = 0;
            qDebug()<<"bus0_pre append: " << deviceID;
            start = true;
        }else if(Bus1API::Instance()->TaskIsContain(deviceID)){
            m_updatestruct.bus1_pre.append(deviceID);
            m_updatestruct.bus1_flag = 0;
            start = true;
        }
    }
    if(start){
        timerUpdate->setInterval(1000);
        timerUpdate->start();
    }else{
        StartServer();
    }
}

void BusServerAPI::Slot_SelfInspection(uint8_t pre_id)
{
    if(pre_id == 0){
        Bus0API::Instance()->CommandSelfInspection(pre_id);
        Bus1API::Instance()->CommandSelfInspection(pre_id);
    }else{
        if(Bus0API::Instance()->TaskIsContain(pre_id)){
            Bus0API::Instance()->CommandSelfInspection(pre_id);
        }else if(Bus1API::Instance()->TaskIsContain(pre_id)){
            Bus1API::Instance()->CommandSelfInspection(pre_id);
        }
    }
}

void BusServerAPI::SelfInspection(PRE_SELF_INSPECTION *preselfinspection)
{//自检信息处理
#ifndef DEVICE_SLAVE
    //若是主机，则需要往TRDP发送信息
#endif
    //增加本地日志
    QString str;
    str.append(QString::number(preselfinspection->id) + ";");
    str.append(QString::number(preselfinspection->PreStatus) + ";");
    str.append(QString::number(preselfinspection->CH_Count) + ";");
    for (int i = 0; i < preselfinspection->CH_Count; i++) {
        str.append(QString::number(preselfinspection->CH_Status.at(i)) + ";");
    }
//    str.append(QString::number(preselfinspection->version));
    str.append(preselfinspection->version);
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"自检信息",DATETIME,str);

    //发送至UDP
    QString udpstr;
    udpstr.append(APPSetting::WagonNumber + ";");
    udpstr.append(DATETIME);
    udpstr.append(";");
    udpstr.append(str);

    UDPMulticastAPI::Instance()->SendSelfInspection(udpstr);
    //检查软件版本，如不一致，则更新,根据ID检查，查询共有几个通道
    QList<int> indexs;
    for(int i=0;i< DBData::DeviceInfo_Count;i++){
        if(DBData::DeviceInfo_DeviceID.at(i) == preselfinspection->id){
            indexs.append(i);
        }
    }
    if(!indexs.isEmpty()){
        //对比版本号
        for(int index : indexs){
            QString oldversion = DBData::DeviceInfo_version.at(index);
            if(oldversion != preselfinspection->version){//如果不一致，则更新版本号
                quint8 ch = DBData::DeviceInfo_DeviceCH.at(index);
                M_DataBaseAPI::UpdateVersion(preselfinspection->id,ch,preselfinspection->version);
//                M_DataBaseAPI::UpdateVersion(preselfinspection->id,ch,QString("v%1").arg(preselfinspection->version));
            }
        }
    }
}

void BusServerAPI::TemInspection(PRE_TEM_VALUE *tem_value)
{//温度巡检返回
    //判断温度是否需要报警
    bool NeedSave = false;
    uint8_t alarm[tem_value->CH_Count];
    for(int i=0;i<tem_value->CH_Count;i++){
        alarm[i] = CoreHelper::ObtainTemAlarmLevel(tem_value->AmbientTem,tem_value->PointTem.at(i));
        if(alarm[i] != 0xff){
            NeedSave = true;
            QString info;
            info.append(QString("环境温度：%1,").arg(tem_value->AmbientTem));
            info.append(QString("测点温度：%1").arg(tem_value->PointTem.at(i)));
            //存储报警信息
            int index = DBData::GetDeviceIndex(tem_value->id,i+1);
            int axis = (index == -1? -1 : DBData::DeviceInfo_AxisPosition.at(index));
            QString devicename = (index == -1? "---" : DBData::DeviceInfo_DeviceName.at(index));
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,tem_value->id,i+1,axis,devicename,
                                  "报警信息",alarm[i],DATETIME,info);
            //更新总线设备状态
            UpdateAlarmState(tem_value->id,i+1,alarm[i]-1);
            //更新报警指示灯
            SetAlarmState(alarm[i]-1);
#ifndef DEVICE_SLAVE
            //更新TRDP报文---LMG
#endif
        }
    }

    //存储温度信息  如果报警，则直接存储，若不报警则按周期存储
    bool cun = true;
    if(APPSetting::UseFileInterval){
        if(DBData::tmpfilecount < APPSetting::TMPSaveInterval){
            DBData::tmpfilecount += 1;
        }else{
            DBData::tmpfilecount = 0;
            cun = true;
        }
    }
    if(NeedSave || cun){
        //数据类型_车厢号_前置ID_时间_.bin
        QString filetime = tem_value->TriggerTime.toString("yyyyMMddHHmmss");
        QString filename = QString("tem_%1_%2_0_%3.bin").arg(APPSetting::WagonNumber).arg(tem_value->id).arg(filetime);
        QByteArray array;
        for(double value : tem_value->PointTem){
            array.append(value);
        }
        DataLog::Instance()->AddData(filename,array);
//        DBData::TCPAddSendFile(filename);
    }

    //信息发送至UDP
    QStringList list;
    list << APPSetting::WagonNumber << tem_value->TriggerTime.toString("yyyy-MM-dd HH:mm:ss") << QString::number(tem_value->id);
    list.append(QString::number(tem_value->CH_Count));
    for(int i=0;i<tem_value->CH_Count;i++){
        list.append(QString::number(alarm[i]));
    }
    QString str = list.join(";");

    UDPMulticastAPI::Instance()->SendTemInspection(str);
}

void BusServerAPI::ReturnVIBData(PRE_VIBRATION_DATA *vibration_data)
{//综合巡检返回
    //判断温度是否需要报警
    QString LogContent;
    int grade = -1;//报警等级 0-预警 1-一级报警 2-二级报警
    uint8_t temalarm = CoreHelper::ObtainTemAlarmLevel(vibration_data->AmbientTemValue,vibration_data->PointTemValue);
    int index = DBData::GetDeviceIndex(vibration_data->id,vibration_data->ch);

    if(temalarm != 0xFF){
        QString info;
        info.append(QString("环境温度：%1,").arg(vibration_data->AmbientTemValue));
        info.append(QString("测点温度：%1").arg(vibration_data->PointTemValue));
        info.append(";");
        LogContent.append(info);
        grade = temalarm - 1;
    }
    //判断诊断结果是否需要报警
    QStringList list = vibration_data->AlarmResult.split("|");
    QStringList typelist;
    typelist << "保外" << "保内" << "外环" << "内环" << "滚单" << "大齿轮" << "小齿轮" << "踏面";
    for(int i=0;i<list.size();i++){
        if(list.at(i).toInt() != 0xFF){
            QString info = typelist.at(i) + "|" +list.at(i) + ";";
            LogContent.append(info);
            if(grade < list.at(i).toInt()) grade = list.at(i).toInt();
        }
    }
    //更新报警指示灯
//    if(grade == 1 || grade == 2){
//        qDebug()<<QString("报警了报警了 id=%1 ch=%2 grade = %3").arg(vibration_data->id).arg(vibration_data->ch).arg(grade);
//        CoreHelper::SetAlarmState(grade);
//    }
    //判断趋势是否需要报警,只有一级和二级
    if(vibration_data->RMSAlarmGrade > 0){
        if(grade < vibration_data->RMSAlarmGrade){
            grade = vibration_data->RMSAlarmGrade;
        }
        QString info = QString("RMS值%1级报警;").arg(vibration_data->RMSAlarmGrade);
        LogContent.append(info);
    }

    if(vibration_data->PPAlarmGrade > 0){
        if(grade < vibration_data->PPAlarmGrade){
            grade = vibration_data->PPAlarmGrade;
        }
        QString info = QString("PP值%1级报警;").arg(vibration_data->PPAlarmGrade);
        LogContent.append(info);
    }

    SetAlarmState(grade);
    if(!LogContent.isEmpty()){
        QString devicename = (index == -1? "---" : DBData::DeviceInfo_DeviceName.at(index));
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,vibration_data->id,vibration_data->ch,vibration_data->AxisPosition,
                              devicename,"报警信息",grade,DATETIME,LogContent);
        UpdateAlarmState(vibration_data->id,vibration_data->ch,grade);
#ifndef DEVICE_SLAVE
        //更新TRDP报文---LMG
#endif
    }
    //发送数据至UDP
    list.clear();
    list << APPSetting::WagonNumber << vibration_data->TriggerTime.toString("yyyy-MM-dd HH:mm:ss")
         << QString::number(vibration_data->id)<< QString::number(vibration_data->ch) << vibration_data->name << QString::number(vibration_data->AxisPosition)
         << QString::number(vibration_data->speedAve) << QString::number(vibration_data->AmbientTemValue)
         << QString::number(vibration_data->PointTemValue)
         << QString::number(temalarm) << vibration_data->AlarmResult << QString::number(vibration_data->RMSAlarmGrade) << QString::number(vibration_data->PPAlarmGrade)
         << vibration_data->DimensionStr << vibration_data->DemodulatedStr;
    QString udpstr = list.join(";");

    UDPMulticastAPI::Instance()->SendComprehensiveInspection(udpstr);
}

void BusServerAPI::PreState(uint8_t preid,bool Isbusy)
{//空闲状态返回
    qDebug()<<"preid = " << preid <<"device busy state : " << Isbusy;
    QObject* senderObject = sender();
    if (senderObject == Bus0API::Instance()) {
        if(Isbusy){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("前置%1无法更新").arg(preid));
            UDPMulticastAPI::Instance()->UpdateFailed(preid);
            m_updatestruct.bus0_pre.removeFirst();
            m_updatestruct.bus0_flag = 0;
        }else{
            Bus0API::Instance()->CommandSendFile(m_updatestruct.UpdateFile);
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("前置%1准备更新").arg(preid));
        }
    } else if (senderObject == Bus1API::Instance()) {
        if(Isbusy){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("前置%1无法更新").arg(preid));
            UDPMulticastAPI::Instance()->UpdateFailed(preid);
            m_updatestruct.bus1_pre.removeFirst();
            m_updatestruct.bus1_flag = 0;
        }else{
            Bus1API::Instance()->CommandSendFile(m_updatestruct.UpdateFile);
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("前置%1准备更新").arg(preid));
        }
    }

}

void BusServerAPI::UpdateResults(uint8_t preid,bool IsFail)
{
    QObject* senderObject = sender();
    if (senderObject == Bus0API::Instance()){
        if(IsFail){
            m_updatestruct.bus0_pre.removeFirst();
            m_updatestruct.bus0_flag = 0;
        }else{
            m_updatestruct.bus0_pre.removeFirst();
            m_updatestruct.bus0_flag = 0;
        }
    }else if(senderObject == Bus1API::Instance()){
        if(IsFail){
            m_updatestruct.bus1_flag = 0;
        }else{
            m_updatestruct.bus1_pre.removeFirst();
            m_updatestruct.bus1_flag = 0;
        }
    }
    QString str;
    if(IsFail){
        str = QString("前置%1更新失败").arg(preid);
    }else{
        str = QString("前置%1更新成功").arg(preid);
    }
    qDebug()<<str;
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,str);
}

void BusServerAPI::RebootState(uint8_t preid, bool IsAbnormal)
{
    QString str;
    if(IsAbnormal){
        str = QString("前置%1无法重启").arg(preid);
    }else{
        str = QString("前置%1重启").arg(preid);
    }
    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,str);
}

void BusServerAPI::ReturnChannelStatus(uint8_t preid, uint8_t ch, bool IsAbnormal)
{
    int index = DBData::GetDeviceIndex(preid,ch);
    if(index == -1) return;
    //如果正常则清出故障队列
    if(!IsAbnormal && TaskAPI::Instance()->existErrorprelist(preid,ch)){
        TaskAPI::Instance()->clearErrorPre(preid,ch);

        int axis = DBData::DeviceInfo_AxisPosition.at(index);
        QString name = DBData::DeviceInfo_DeviceName.at(index);
        //UDP发出上线指令
        QStringList list;
        list.append(APPSetting::WagonNumber);
        list.append("前置");
        list.append(QString("%1|%2|%3").arg(preid).arg(ch).arg(axis));
        list.append(name);
        list.append(QString::number(DBData::DeviceState_Online));
        list.append("设备上线");
        list.append(DATETIME);
        UDPMulticastAPI::Instance()->SendBoardStatus(list.join(";"));
    }
}
