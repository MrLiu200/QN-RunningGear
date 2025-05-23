#include "bus2api.h"
#include "corehelper.h"
#include <QProcess>

Bus2API *Bus2API::self = 0;
Bus2API *Bus2API::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker lock(&mutex);
        if(!self){
            self = new Bus2API;
        }
    }
    return self;
}

Bus2API::~Bus2API()
{
    if(timerPISState->isActive()){
        timerPISState->stop();
        qDebug()<<"close timerPISState";
    }

    if(ETH1FileIndex.isOpen()){
        ETH1FileIndex.close();
        qDebug()<<"close ETH1FileIndex";
    }
}

Bus2API::Bus2API(QObject *parent) : QObject(parent)
{
    IsOpen = false;
#ifndef DEVICE_SLAVE
    VersionMap.insert(SPEEDBOARDID,"未知");
    StateMap.insert(SPEEDBOARDID,"自检信息未知");
#endif

    VersionMap.insert(COMMUNICATIONBOARDID,"未知");
    VersionMap.insert(PREIOBOARDID,"未知");

    StateMap.insert(COMMUNICATIONBOARDID,"自检信息未知");
    StateMap.insert(PREIOBOARDID,"自检信息未知");

    UpdateDevice = 0;

    PISState = QString();
    timerPISState = new QTimer(this);
    timerPISState->setInterval(APPSetting::CheckPISStateInterval);
    connect(timerPISState,&QTimer::timeout,this,&Bus2API::GetPISNetState);


    connect(UDPMulticastAPI::Instance(),&UDPMulticastAPI::BackboardCardSoftwareUpdate,this,&Bus2API::BackboardCardSoftwareUpdate);
}

bool Bus2API::Open()
{
#ifndef Q_OS_WIN
    //打开硬件上的CAN设备
    QProcess process;
    QString SYStemCmdconf;
    QString SYStemCmdDownCan = QString("ifconfig %1 down").arg(APPSetting::can2_Name);
    system(SYStemCmdDownCan.toLatin1().data());
    if(APPSetting::can2_Mode == "canfd"){
        SYStemCmdconf = QString("ip link set %1 up type can bitrate %2 sample-point %3 dbitrate %4 dsample-point %5 fd on")
                .arg(APPSetting::can2_Name).arg(APPSetting::can2_bitrate)
                .arg(APPSetting::can2_bitrateSample).arg(APPSetting::can2_dbitrate).arg(APPSetting::can2_dbitrateSample);
    }else if(APPSetting::can2_Mode == "can"){
        SYStemCmdconf = QString("ip link set %1 up type can bitrate %2 fd off").arg(APPSetting::can2_Name).arg(APPSetting::can2_bitrate);
    }
    process.start(SYStemCmdconf);
    process.waitForFinished(-1);
#endif
    //创建can设备
    m_canDevice.reset(QCanBus::instance()->createDevice(APPSetting::CanPlugin, APPSetting::can2_Name));
    if (!m_canDevice) {
        qWarning() << "Failed to create CAN2 device";
        IsOpen = false;
    }else{
        IsOpen = true;
    }

    if(IsOpen){
        connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,this, &Bus2API::processErrors);
        connect(m_canDevice.get(), &QCanBusDevice::framesReceived,this, &Bus2API::processReceivedFrames);

        /*设置can参数*/
        //仲裁域波特率APPSetting::can2_bitrate.toInt()
        m_canDevice->setConfigurationParameter(QCanBusDevice::BitRateKey,QVariant());
        //开启canfd模式
        if(APPSetting::can2_Mode == "canfd"){
            m_canDevice->setConfigurationParameter(QCanBusDevice::CanFdKey,QVariant(true));
            //数据域波特率
            m_canDevice->setConfigurationParameter(QCanBusDevice::DataBitRateKey,QVariant());
        }else if(APPSetting::can2_Mode == "can"){
            m_canDevice->setConfigurationParameter(QCanBusDevice::CanFdKey,QVariant(false));
        }

        // 打开设备
        if (!m_canDevice->connectDevice()) {
            qWarning() << "Failed to connect to CAN2 device:" << m_canDevice->errorString();
            m_canDevice.reset();
            IsOpen = false;
        }
    }
#ifndef DEVICE_SLAVE
    if(IsOpen){
        GetPISNetState();
        timerPISState->start();
    }
#endif
    return IsOpen;
}

void Bus2API::processReceivedFrames()
{
    if (!IsOpen)
        return;
    while (m_canDevice->framesAvailable()) {
        const QCanBusFrame frame = m_canDevice->readFrame();
        if (frame.frameType() == QCanBusFrame::DataFrame){
            //提取出数据和帧ID
            quint32 id = frame.frameId();
            QByteArray data = frame.payload();
            if(id != CAN_Default_FrameID){
                checkData(data,id);
            }
        }
    }
}

void Bus2API::processErrors(QCanBusDevice::CanBusError error) const
{
    switch (error) {
    case QCanBusDevice::ReadError:
    case QCanBusDevice::WriteError:
    case QCanBusDevice::ConnectionError:
    case QCanBusDevice::ConfigurationError:
    case QCanBusDevice::UnknownError:
        qDebug()<<"Bus2API Error: " + m_canDevice->errorString();
        break;
    default:
        break;
    }
}

void Bus2API::checkData(QByteArray frameData, quint32 frameID)
{
    int alllen = frameData.size();
//    qDebug()<< "can2 checkdata:" << CoreHelper::byteArrayToHexStr(frameData);
    if(alllen < 4) return;
    if(frameData[0] != CAN_Default_FrameID) return; //如果目标板卡非本板卡，则不处理

    //取出有效字节
    int len =0;
    if(alllen > 2){
        uint8_t datalen = frameData[2];
        len = datalen + 4;
        if(len > 8){
            len = 8;
        }
    }
    uint8_t frame[len];
    for(int i=0; i<len; i++){
        frame[i] = frameData[i];
    }
    uint8_t oldcrc = frame[len-1];
    uint8_t newcrc = CoreHelper::CheckSum(frame,0,len-1);
    if(oldcrc == newcrc){
        switch (frame[1]) {
        case CommandByte_SelfInspection://自检信息
        {
            uint8_t state = frame[3];
            SetStateMap(frameID,state);
            //2025-05-21修改，版本号为vx.y，通信协议中高字节为x，低字节为y
            QString version = QString("v%1.%2").arg(frame[4]).arg(frame[5]);
//            uint16_t version = ((frame[4] << 8) | frame[5]);
            SetVersionMap(frameID,version);
        }
            break;
        case CommandByte_Heartbeat://心跳上报
        {
            for(auto &str : DBData::OtherBoardStatus ){
                QStringList list = str.split("|");
                uint32_t id = list.at(0).toUInt();
                uint8_t oldstatus = list.at(1).toInt();
                if(id == frameID){
                    str = QString("%1|%2|%3").arg(frameID).arg(DBData::DeviceState_Online).arg(DATETIME);
                    if(oldstatus == DBData::DeviceState_Offline){
                        QString devicename;
                        if(id == COMMUNICATIONBOARDID){
                            devicename = "通信板卡";
                        }else if(id == SPEEDBOARDID){
                            devicename = "转速板卡";
                        }else if(id == PREIOBOARDID){
                            devicename = "前置IO板卡";
                        }
                        QString content = QString("背板设备(canid：%1) 上线").arg(frameID);
                        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,content);
                        QStringList list;
                        list.append(APPSetting::WagonNumber);
                        list.append("OtherBoard");
                        list.append(QString("%1").arg(id));
                        list.append(devicename);
                        list.append(QString::number(DBData::DeviceState_Online));
                        list.append(content);
                        list.append(DATETIME);
                        UDPMulticastAPI::Instance()->SendBoardStatus(list.join(";"));
                    }
                    break;
                }
            }
        }
            break;
#ifndef DEVICE_SLAVE
        case CommandByte_Speed://转速更新
        {
            if(frameID != SPEEDBOARDID || (!APPSetting::UseExternalSpeed)) return;
            uint32_t speed = ((frame[3] << 24) | (frame[4] << 16) |(frame[5] << 8) | frame[6]); // 合成速度数据
            //             qDebug()<<"+++++++++CommandByte_Speed:" << speed;
            double speedValue = static_cast<double>(speed) / 1000000.0; // 转换为浮点数
            //            qDebug()<<"speedValue == " << speedValue;

            if (speedValue > 0.0) {
                double result = (60.0 / speedValue)*APPSetting::SpeedCoefficient; // 直接倒数乘法计算,再乘以转速系数
                //                qDebug()<<"+++++++++CommandByte_Speed:" << result;
                DBData::RunSpeed = static_cast<uint32_t>(qRound(result)); // 四舍五入存储
                //                qDebug()<<"+++++++++CommandByte_Speed: DBData::RunSpeed " << DBData::RunSpeed;
            } else {
                //                qWarning() << "Speed value is zero or invalid!";
                DBData::RunSpeed = 0; // 设置默认值
            }
            if(DBData::SpeedStruct.Bus0CanSave){
                DBData::SpeedStruct.Bus0SpeedVector.append(DBData::RunSpeed);
            }
            if(DBData::SpeedStruct.Bus1CanSave){
                DBData::SpeedStruct.Bus1SpeedVector.append(DBData::RunSpeed);
            }
        }
            break;
#endif
        case CommandByte_Version://软件版本
        {
#if 0
            uint32_t version = ((frame[3] << 24) | (frame[4] << 16) | (frame[5] << 8) | frame[6]);
            SetVersionMap(frameID,version);
#else
            QString version = QString("v%1.%2").arg(frame[4]).arg(frame[5]);
            SetVersionMap(frameID,version);
#endif
            Q_EMIT VersionReturn(frameID,version);
        }
            break;
        case CommandByte_UpdateRequest://更新请求
        {
            uint8_t state = frame[3];
            qDebug()<<"bus2 CommandByte_UpdateRequest state :" << state;
            if(state == UpdateState_Abnormal){//如果无法更新需要触发信号
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("背板(canid = %1无法更新").arg(UpdateDevice));
                UDPMulticastAPI::Instance()->UpdateFailed(UpdateDevice);
            }else if(state == UpdateState_OK){//发送文件
                SendUpdateFile(frameID,UpdateFile);
            }
        }
            break;
        case CommandByte_SendUpdateFile://发送更新文件
        {
            uint8_t state = frame[3];
            qDebug()<<"bus2 CommandByte_SendUpdateFile state :" << state;
            if(state == UpdateState_Abnormal){//如果接收文件不成功，则重新发送请求
                UpdateAPPRequest(frameID);
            }else if(state == UpdateState_OK){
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行信息",DATETIME,QString("背板(canid = %1)软件更新").arg(UpdateDevice));
                UpdateDevice = 0;
                UpdateCRC = 0;
                UpdateFile = "";
            }
        }
            break;
        case CommandByte_RebootRequest://重启请求
        {
            uint8_t state = frame[3];
            if(state == UpdateState_Abnormal){//如果接收文件不成功，则重新发送请求
                Q_EMIT RebootReturn(frameID,UpdateState_Abnormal);
            }else if(state == UpdateState_OK){
                Q_EMIT RebootReturn(frameID,UpdateState_OK);
            }

        }
            break;
        case CommandByte_PowerControl://电源控制
        {
            bool state = frame[3];
            if(state != APPSetting::IOPowerEnable){
                APPSetting::IOPowerEnable = state;
                APPSetting::WriteConfig();
                QString str = QString("前置IO板卡电源 %1 失败")
                        .arg(APPSetting::IOPowerEnable = true? "使能" : "失能");
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,str);
                //发送错误信息至UDP
                QStringList list;
                list.append(APPSetting::WagonNumber);
                list.append("OtherBoard");
                list.append(QString("%1").arg(PREIOBOARDID));
                list.append("前置IO板卡");
                list.append("2");
                list.append(str);
                list.append(DATETIME);
                UDPMulticastAPI::Instance()->SendBoardStatus(list.join(";"));
//                qDebug()<<"str = " << str;
            }else{
                QString str = QString("前置IO板卡电源 %1 成功")
                        .arg(APPSetting::IOPowerEnable = true? "使能" : "失能");
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,str);
                qDebug()<<"str = " << str;
            }
        }
            break;

        default:
            break;
        }
    }
}

void Bus2API::SendData(const QCanBusFrame &frame)
{
    if(!m_canDevice) return;
    m_canDevice->writeFrame(frame);
    //    if(ok){
    //        qDebug()<<"can2 send ok";
    //    }else{
    //        qDebug()<<"can2 send fail";
    //    }
}

QByteArray Bus2API::GetSendPackage(uint8_t FrameID, uint8_t order, QByteArray data)
{
    if(data.size() > 4) return QByteArray();
    QByteArray array;
    array.append(FrameID);
    array.append(order);
    array.append(data.size());
    array.append(data);
    array.append(CoreHelper::CheckSum(array,0,array.size()));
//    qDebug()<<"bus2 getSendPackage" << CoreHelper::byteArrayToHexStr(array);
    return array;
}

void Bus2API::BackboardCardSoftwareUpdate(const QString filename, const uint32_t crc, uint8_t devicetype)
{
    SetUpdateFile(filename,crc);
    uint8_t frameID = 0;
    switch(devicetype){
#ifndef DEVICE_SLAVE
    case 0x02:
        frameID = SPEEDBOARDID;
        break;
#endif
    case 0x03:
        frameID = PREIOBOARDID;
        break;
    case 0x04:
        frameID = COMMUNICATIONBOARDID;
        qDebug()<<"Update COMMUNICATIONBOARDID";
        break;
    default:
        frameID = 0;
        break;
    }
    if(frameID != 0){
        UpdateDevice = frameID;
        UpdateAPPRequest(frameID);
    }

}

void Bus2API::GetPISNetState()
{
    //检查网络状态
    if (!ETH1FileIndex.isOpen()) {
        ETH1FileIndex.setFileName("/sys/class/net/eth1/operstate");
        if (!ETH1FileIndex.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open network status file!";
            return;
        }
    }else{
        ETH1FileIndex.seek(0);
    }
    QByteArray statusData = ETH1FileIndex.readAll().trimmed();
//    qDebug()<<"eth1 state = " << statusData;
    if(PISState != statusData){
        //2025-02-22新增加了一个重启UDP功能，还不知道在运行时重启会不会有问题，需要验证
        if(!PISState.isEmpty()){
            QTimer::singleShot(1000, [](){
                UDPMulticastAPI::Instance()->test20250522();
            });

        }
        PISState = statusData;
        if(statusData == "up"){
            SetPISLedState(1);
        }else if(statusData == "down"){
            SetPISLedState(0);
        }
    }
#if 0
    QFile file("/sys/class/net/eth1/operstate");
    if(file.open(QIODevice::ReadOnly)){
        QTextStream stream(&file);
        QString status  = stream.readLine().trimmed();
        if(PISState != status){
            PISState = status;
            if(status == "up"){
                SetPISLedState(1);
            }else if(status == "down"){
                SetPISLedState(0);
            }
        }
        file.close();
    }
#endif
}

void Bus2API::GetSelfInspection(uint8_t FrameID)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(FrameID,CommandByte_SelfInspection,QByteArray()));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);

    SendData(frame);
}

void Bus2API::GetVersion(uint8_t frameID)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(frameID,CommandByte_Version,QByteArray()));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);

    SendData(frame);
}

void Bus2API::UpdateAPPRequest(uint8_t frameID)
{
    if(UpdateCRC == 0 || UpdateFile.isEmpty()) return;
    QByteArray array;
    array.append(((uint8_t)(UpdateCRC >> 24) & 0xff));
    array.append(((uint8_t)(UpdateCRC >> 16) & 0xff));
    array.append(((uint8_t)(UpdateCRC >> 8) & 0xff));
    array.append(((uint8_t)UpdateCRC & 0xff));
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(frameID,CommandByte_UpdateRequest,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);

    SendData(frame);

    //修改，要先发送数据长度
    QFileInfo fileinfo(UpdateFile);
    qint64 filesize = fileinfo.size();
    qDebug()<<"filesize = " << filesize;
    array.clear();
    array.append(((uint8_t)(filesize >> 24) & 0xff));
    array.append(((uint8_t)(filesize >> 16) & 0xff));
    array.append(((uint8_t)(filesize >> 8) & 0xff));
    array.append(((uint8_t)filesize & 0xff));


    frame.setPayload(GetSendPackage(frameID,CommandByte_SendUpdateFile,array));
    SendData(frame);
}

void Bus2API::SendUpdateFile(uint8_t frameID, QString filename)
{
//    qDebug()<<"bus2 SendUpdateFile";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading";
        return;
    }
#if 0
    qint64 filesize = file.size();

    QByteArray array;
    array.append(((uint8_t)(filesize >> 24) & 0xff));
    array.append(((uint8_t)(filesize >> 16) & 0xff));
    array.append(((uint8_t)(filesize >> 8) & 0xff));
    array.append(((uint8_t)filesize & 0xff));
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(frameID,CommandByte_SendUpdateFile,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);

    SendData(frame);
#endif

    //循环发送数据文件
    QByteArray block;
    QCanBusFrame framefile;
    framefile.setFrameId(CAN_Default_FrameID);
    framefile.setExtendedFrameFormat(false);
    framefile.setFlexibleDataRateFormat(false);
    framefile.setBitrateSwitch(false);

    do{
        block.clear();
        block = file.read(0x08);//每次读取8个数据
        // 如果读取的内容为空，则跳过
        if (block.isEmpty()) {
            break;
        }
        // 如果读取的内容小于 8 字节，且文件没有结束，读取剩余的内容
        if (block.size() < 8 && !file.atEnd()) {
            QByteArray remainingData = file.readAll(); // 读取剩余全部内容
            block.append(remainingData); // 将剩余内容附加到当前数据块
        }

        framefile.setPayload(block);
        SendData(framefile);
        QThread::usleep(800);

    }while(!file.atEnd());

    file.close();
}

void Bus2API::RebootDevice(uint8_t frameID)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(frameID,0x07,QByteArray()));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);

    SendData(frame);
}

QByteArray Bus2API::GetVersionMap(uint8_t key)
{
    QByteArray arr;
    if(key == 0xFF){//上位机获取，按地址返回
        if(VersionMap.contains(SPEEDBOARDID)){//速度板卡
            arr.append(VersionMap.value(SPEEDBOARDID));
        }
        if(VersionMap.contains(COMMUNICATIONBOARDID)){//通信板卡
            arr.append(VersionMap.value(COMMUNICATIONBOARDID));
        }
        if(VersionMap.contains(PREIOBOARDID)){//前置IO板卡
            arr.append(VersionMap.value(PREIOBOARDID));
        }
    }else{
        if(VersionMap.contains(key)){
            arr.append(VersionMap.value(key));
        }
    }
    return arr;
}

//void Bus2API::SetVersionMap(uint8_t key, uint16_t value)
void Bus2API::SetVersionMap(uint8_t key, QString version)
{
    if(VersionMap.contains(key)){
        VersionMap.insert(key,version);
    }
}

QByteArray Bus2API::GetStateMap(uint8_t key)
{
    QByteArray arr;
    if(key == 0xFF){//上位机获取，按地址返回
        if(StateMap.contains(SPEEDBOARDID)){//速度板卡
            arr.append(StateMap.value(SPEEDBOARDID));
        }
        if(StateMap.contains(COMMUNICATIONBOARDID)){//通信板卡
            arr.append(StateMap.value(COMMUNICATIONBOARDID));
        }
        if(StateMap.contains(PREIOBOARDID)){//前置IO板卡
            arr.append(StateMap.value(PREIOBOARDID));
        }
    }else{
        if(StateMap.contains(key)){
            arr.append(StateMap.value(key));
        }
    }
    return arr;
}

void Bus2API::SetStateMap(uint8_t key, uint8_t value)
{
    QString valuestr;
    switch(value){
    case 0x00:
        valuestr = "无故障";
        break;
    case 0x01:
        valuestr = "APP0分区CRC错误";
        break;
    case 0x02:
        valuestr = "APP1分区CRC错误";
        break;
    case 0x03:
        valuestr = "处于OTA模式";
        break;
    default:
        valuestr = "自检结果未知";
        break;
    }

    if(StateMap.contains(key)){
        StateMap.insert(key,valuestr);
    }
}

void Bus2API::SetUpdateFile(QString file, uint32_t crc)
{
    this->UpdateFile = file;
    this->UpdateCRC = crc;
}

void Bus2API::SetPreIOPowerState(bool state)
{
    APPSetting::IOPowerEnable = state;
    APPSetting::WriteConfig();

    QByteArray array;
    array.append(state);
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PREIOBOARDID,CommandByte_PowerControl,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);

    SendData(frame);
}

void Bus2API::SetPISLedState(uint8_t state)
{
    QByteArray array;
    array.append(state);
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(COMMUNICATIONBOARDID,CommandByte_PISLedControl,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(false);
    frame.setBitrateSwitch(false);
//    qDebug()<<"SetPISLedState : " << state;
    SendData(frame);
}

bool Bus2API::ReadPISIsConnect()
{
    if(PISState == "up"){
        return true;
    }else{
        return false;
    }
}
