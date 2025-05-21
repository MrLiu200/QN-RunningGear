#include "bus0api.h"

Bus0API *Bus0API::self = 0;
Bus0API *Bus0API::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker lock(&mutex);
        if(!self){
            self = new Bus0API;
        }
    }
    return self;
}

Bus0API::Bus0API(QObject *parent) : QObject(parent)
{
    IsOpen = false;
    currentpreID = 0;
    currentpreCH = 0;
    currentStartSpeed = 0;
    currentEndSpeed = 0;
    VIBDataLen = 0;
    VibdataArr.clear();
    savefilecount = 0;
}

bool Bus0API::Open()
{
    //打开硬件上的CAN设备
#ifndef Q_OS_WIN
    QProcess process;
    QString SYStemCmdDownCan = QString("ifconfig %1 down").arg(APPSetting::can0_Name);
    system(SYStemCmdDownCan.toLatin1().data());
    QString SYStemCmdconf = QString("ip link set %1 up type can bitrate %2 sample-point %3 dbitrate %4 dsample-point %5 fd on")
            .arg(APPSetting::can0_Name).arg(APPSetting::can1_bitrate)
            .arg(APPSetting::can0_bitrateSample).arg(APPSetting::can0_dbitrate).arg(APPSetting::can0_dbitrateSample);
    process.start(SYStemCmdconf);
    process.waitForFinished(-1);
#endif
    //创建can设备
    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice(APPSetting::CanPlugin, APPSetting::can0_Name,&errorString));
    if (!m_canDevice) {
        qWarning() << QString("Failed to create CAN0 device,reason:%1").arg(errorString);
        IsOpen = false;
    }else{
        IsOpen = true;
    }

    if(IsOpen){
        connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,this, &Bus0API::processErrors);
        connect(m_canDevice.get(), &QCanBusDevice::framesReceived,this, &Bus0API::processReceivedFrames);

        /*设置can参数*/
        //仲裁域波特率APPSetting::can0_bitrate.toInt() 1000000
        m_canDevice->setConfigurationParameter(QCanBusDevice::BitRateKey,QVariant());
        //开启canfd模式
        if(APPSetting::can0_Mode == "canfd"){
            m_canDevice->setConfigurationParameter(QCanBusDevice::CanFdKey,QVariant(true));
        }else if(APPSetting::can0_Mode == "can"){
            m_canDevice->setConfigurationParameter(QCanBusDevice::CanFdKey,QVariant(false));
        }
        //数据域波特率APPSetting::can0_dbitrate.toInt() 4000000
        m_canDevice->setConfigurationParameter(QCanBusDevice::DataBitRateKey,QVariant());
        // 打开设备
        if (!m_canDevice->connectDevice()) {
            qWarning() << "Failed to connect to CAN0 device:" << m_canDevice->errorString();
            m_canDevice.reset();
            IsOpen = false;
        }
    }
    return IsOpen;
}

void Bus0API::processReceivedFrames()
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

void Bus0API::processErrors(QCanBusDevice::CanBusError error) const
{
    switch (error) {
    case QCanBusDevice::ReadError:
    case QCanBusDevice::WriteError:
    case QCanBusDevice::ConnectionError:
    case QCanBusDevice::ConfigurationError:
    case QCanBusDevice::UnknownError:
        qDebug()<<"Bus0API Error: " + m_canDevice->errorString();
        break;
    default:
        break;
    }
}

void Bus0API::checkData(QByteArray frameData, quint32 frameID)
{
    int len = frameData.size();
    if(len == 0) return;
    //        qDebug()<<"bus0 checkData:" << CoreHelper::byteArrayToHexStr(frameData);
    if(len == 64){//帧长度为64,识别为数据流
        //        static int test = 0;
        if(frameID == currentpreID){
            VibdataArr.append(frameData);//添加至振动数据阵列
            //            qDebug()<<test++;
            if(VibdataArr.size() == VIBDataLen){
                //                test = 0;
                //                qDebug()<<"bus0 checkData "  << frameID  << ": VibdataArr.size() = "<< VIBDataLen;
                //查找该通道灵敏度
                int index = DBData::GetDeviceIndex(currentpreID,currentpreCH);
                if(index == -1){
                    VibdataArr.clear();
                    SetTaskState(currentpreID,TASKSTATE::WaitingSwitch);
                    UpdateDeviceState(currentpreID,currentpreCH);
                    return;
                }

                float Sensitivity = DBData::DeviceInfo_Sensitivity.at(index);
                int axisposition = DBData::DeviceInfo_AxisPosition.at(index);
                QString devicename = DBData::DeviceInfo_DeviceName.at(index);
                //填充结构体
                PRE_VIBRATION_DATA vibration_data;
                vibration_data.id = currentpreID;
                vibration_data.ch = currentpreCH;
                vibration_data.name = devicename;
                vibration_data.AxisPosition = axisposition;
                vibration_data.speedAve = (currentEndSpeed + currentStartSpeed)/2;
                vibration_data.AmbientTemValue = findTemperatures(currentpreID,0);
                vibration_data.PointTemValue = findTemperatures(currentpreID,currentpreCH);
                vibration_data.TriggerTime = QDateTime::currentDateTime();
                if(!DBData::SpeedStruct.Bus0SpeedVector.isEmpty()){
                    vibration_data.SpeedVector = DBData::SpeedStruct.Bus0SpeedVector;
                }


                //解析出数据 格式：前一半是原始数据，后一半是滤波后的数据,低位在前，高位在后
                QVector <float> RawData;                       //原始数据
                QVector<float> DemodulatedData;                //解调数据
                float Rawsum = 0,Demsum=0,Rawave=0,Demave = 0,tempdouble;
                uint16_t temp;
                for(int i=0;i<VIBDataLen/2;i+=2){
                    temp = ((quint8)(VibdataArr.at(i+1)) << 8) | (quint8)(VibdataArr.at(i));
                    //                    temp = (quint8)(VibdataArr.at(i+1) << 8 | (quint8)(VibdataArr.at(i)));
                    tempdouble = (((temp*3.3*1000)/65535)-1650)/Sensitivity;//换算出电压值(double)(double)/灵敏度
                    RawData.append(tempdouble);
                }
                for(int i=VIBDataLen/2;i<VIBDataLen;i+=2){
                    temp = ((quint8)(VibdataArr.at(i+1)) << 8) | (quint8)(VibdataArr.at(i));
                    //                    temp = (quint8)(VibdataArr.at(i+1) << 8 | (quint8)(VibdataArr.at(i)));
                    tempdouble = (((temp*3.3*1000)/65535)-1650)/Sensitivity;//换算出电压值(double)(double)/灵敏度
                    DemodulatedData.append(tempdouble);
                }
                //求和并找出原始数据最大值
                Rawsum = std::accumulate(RawData.begin(), RawData.end(), 0.0);
                Demsum = std::accumulate(DemodulatedData.begin(), DemodulatedData.end(), 0.0);
                //                double Raw_Max = *(std::max_element(RawData.begin(), RawData.end()));

                //RawData 与 DemodulatedData 长度一样，这里用谁都行
                qint32 datelen = RawData.size();
                //求出平均值
                Rawave = Rawsum/datelen;
                Demave = Demsum/datelen;
                //去直流分量
                for(int i=0;i<datelen;i++){
                    RawData[i] -= Rawave;
                    DemodulatedData[i] -= Demave;
                    DemodulatedData[i] = DemodulatedData[i] * 166;
                }
                RawData[datelen-2] = 0;
                RawData[datelen-1] = 0;
                DemodulatedData[datelen-2] = 0;
                DemodulatedData[datelen-1] = 0;
                //计算量纲值并存储至数据库
                QVector <float> Dimension;                      //量纲值
                Dimension = CoreHelper::CalculateDimension(RawData);
                float rms = Dimension.at(5);


                QStringList DimensionList;
                for(const float value : Dimension){
                    DimensionList.append(QString::number(value));
                }
                vibration_data.DimensionStr = DimensionList.join("|");

                QVector <float> Demodulated;                      //包络量纲值
                Demodulated = CoreHelper::CalculateDimension(DemodulatedData);
                QStringList DemodulatedList;
                for(const float value : Demodulated){
                    DemodulatedList.append(QString::number(value));
                }
                vibration_data.DemodulatedStr = DemodulatedList.join("|");

#ifndef DEVICE_SLAVE
                M_DataBaseAPI::AddDimensionalValue(APPSetting::WagonNumber,currentpreID,currentpreCH,vibration_data.speedAve,
                                                   vibration_data.AmbientTemValue,vibration_data.PointTemValue,
                                                   vibration_data.TriggerTime.toString("yyyy-MM-dd HH:mm:ss"),
                                                   Dimension);
                M_DataBaseAPI::AddDemodulatedValue(APPSetting::WagonNumber,currentpreID,currentpreCH,vibration_data.speedAve,
                                                   vibration_data.AmbientTemValue,vibration_data.PointTemValue,
                                                   vibration_data.TriggerTime.toString("yyyy-MM-dd HH:mm:ss"),
                                                   Demodulated);
#endif
                //hanning window
                for(int i=0;i<datelen;i++){
                    RawData[i] *= 0.5 - 0.5 * cos(2 * M_PI * i / 65535);
                    DemodulatedData[i] *= 0.5 - 0.5 * cos(2 * M_PI * i / 65535);
                }
                //傅里叶变换
                CoreHelper::FFT(RawData);
                CoreHelper::FFT(DemodulatedData);
                //(2/65536)傅里叶变换后 幅值会*数据长度，所以在这里除回来，*2是hanning window幅值补阵
                for (int i=0;i<datelen;i++) {
                    RawData[i] = (RawData[i]*2/65536)*2;
                    DemodulatedData[i] = (DemodulatedData[i]*2/65536)*2;
                }

                //求FFT后平均值
                Rawsum = std::accumulate(RawData.begin(), RawData.end(), 0.0);
                Demsum = std::accumulate(DemodulatedData.begin(), DemodulatedData.end(), 0.0);
                Rawave = Rawsum/datelen;
                Demave = Demsum/datelen;
                //                qDebug()<<"Bus0 Demave:" << Demave;
                //找出测点类型
                int Devicetype = 0;
                QString typestr = DBData::DeviceInfo_DeviceType.at(index);
                if(typestr == "齿轮箱")
                    Devicetype = 1;
                else if(typestr == "电机" || typestr == "电机传动端" || typestr == "电机非传动端")
                    Devicetype = 2;
                //初始化振动算法参数
                //检查量纲是否需要报警
//                CheckDimensionState(currentpreID,currentpreCH,axisposition,Devicetype,Dimension.at(5),Dimension.at(8));
                vibration_data.RMSAlarmGrade = CheckRMSState(Devicetype,Dimension.at(5));
                vibration_data.PPAlarmGrade = CheckPPState(Devicetype,Dimension.at(8));
                //获取轴承参数
                QVector<float> list1 = DBData::QueryBearingParameters(DBData::DeviceInfo_bearing1_model.at(index));
                QVector<float> list2 = DBData::QueryBearingParameters(DBData::DeviceInfo_bearing2_model.at(index));
                QVector<float> list3 = DBData::QueryBearingParameters(DBData::DeviceInfo_bearing3_model.at(index));
                QVector<float> list4 = DBData::QueryBearingParameters(DBData::DeviceInfo_bearing4_model.at(index));
                QStringList bearingmodel;
                bearingmodel.append(DBData::DeviceInfo_bearing1_model.at(index));
                bearingmodel.append(DBData::DeviceInfo_bearing2_model.at(index));
                bearingmodel.append(DBData::DeviceInfo_bearing3_model.at(index));
                bearingmodel.append(DBData::DeviceInfo_bearing4_model.at(index));
                Algorithm_v2::Argument_Init(list1.at(3),list1.at(0),list1.at(1),list1.at(2),
                                            list2.at(3),list2.at(0),list2.at(1),list2.at(2),
                                            list3.at(3),list3.at(0),list3.at(1),list3.at(2),
                                            list4.at(3),list4.at(0),list4.at(1),list4.at(2),
                                            50.0,50.0,vibration_data.speedAve,DBData::DeviceInfo_capstanTeethNum.at(index),
                                            DBData::DeviceInfo_DrivenwheelTeethNum.at(index),
                                            Devicetype);
                //运行算法
                //参数传入由最大值改为均方根

                //                QStringList ret = Algorithm_v2::Algoruthm_Start(RawData,DemodulatedData,RawData.size(),
                //                                                                DemodulatedData.size(),APPSetting::VibrateSamplingFrequency,
                //                                                                Raw_Max,Rawave,Demave);
                QStringList ret = Algorithm_v2::Algoruthm_Start(RawData,DemodulatedData,RawData.size(),
                                                                DemodulatedData.size(),APPSetting::VibrateSamplingFrequency,
                                                                rms,Rawave,Demave);

                //1、收到了结果进行处理并填充结构体
                //2、将数据进行存储并添加至vibfile发送队列
                //添加内容到数据中，然后进行存储
                //将 转速信息、诊断结果 添加至VibdataArr中
                QDataStream stream(&VibdataArr, QIODevice::WriteOnly | QIODevice::Append);

                QString titlestr = "speedAve:";
                stream << titlestr << vibration_data.speedAve;

                titlestr = "SpeedVector:";
                stream << titlestr << vibration_data.SpeedVector.size();
                for (uint32_t value : vibration_data.SpeedVector) {
                    stream << value;
                }

                //                titlestr = "Demodulated:";
                //                stream << titlestr;
                //                for (float value : Demodulated) {
                //                    stream << value;
                //                }

                titlestr = "result:";
                stream << titlestr << ret.size();
                for(QString str : ret){
                    stream << str;
                }

                titlestr = "bearingInfo:";
                stream << titlestr;
                stream << bearingmodel.join("|");

                //填充报警结果
                vibration_data.AlarmResult = ret.at(0);

                //判断是否需要存储数据
                QString time = vibration_data.TriggerTime.toString("yyyyMMddHHmmss");
                QString filename = QString("vib_%1_%2_%3_%4.bin")
                        .arg(APPSetting::WagonNumber).arg(currentpreID).arg(currentpreCH).arg(time);
                if(APPSetting::UseFileInterval){
                    if(savefilecount < APPSetting::VIBSaveInterval){
                        savefilecount += 1;
                    }else{
                        savefilecount = 0;
                        DataLog::Instance()->AddData(filename,VibdataArr);
                    }
                }else{
                    DataLog::Instance()->AddData(filename,VibdataArr);
                }
                //存储数据
                VibdataArr.clear();
                SetTaskState(currentpreID,TASKSTATE::WaitingSwitch);
                UpdateDeviceState(currentpreID,currentpreCH);

                Q_EMIT ReturnVIBData(&vibration_data);
            }
        }else{
            qDebug()<<"currentid = " << currentpreID << " frameid = " << frameID;
        }
    }else{//解析命令
        //        qDebug()<<"bus0 checkData "  << frameID  << ": "<< CoreHelper::byteArrayToHexStr(frameData);
        uint8_t frame[len];
        //根据数据长度，截取正确数据包
        uint8_t temp = frameData[2];
        len = temp + 4;
        for(int i=0; i<len; i++){
            frame[i] = frameData[i];
        }

        uint8_t oldcrc = frame[len-1];
        uint8_t newcrc = CoreHelper::CheckSum(frame,0,len-1);
        if(oldcrc == newcrc){
            uint8_t order = frame[1];
            switch (order) {
            case 0x01://设备自检信息
            {
                PRE_SELF_INSPECTION self_inspection;
                self_inspection.id = frameID;
                self_inspection.PreStatus = frame[PRE_PACKET_DATA_OFFSET];
                self_inspection.CH_Count = frame[PRE_PACKET_DATA_OFFSET + 1];
                for(int i=PRE_PACKET_DATA_OFFSET + 2;i<len-3;i++){
                    self_inspection.CH_Status.append(frame[i]);
                    if(frame[i] == 0){
                        UpdateDeviceState(frameID,i-4);
                    }else{
                        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,
                                              "报警信息",DATETIME,QString("bus0设备(preid:%1,prech:%2) 故障").arg(frameID).arg(i-4));
                        UpdateDeviceState(frameID,i-4,true);
                    }
                }
//                self_inspection.version = TOUINT16(frame[len-3],frame[len-2]);
                self_inspection.version = QString("v%1.%2").arg(frame[len-3]).arg(frame[len-2]);

                addTaskprelist(frameID,frame[PRE_PACKET_DATA_OFFSET + 1],self_inspection.CH_Status);
                //                UpdateDeviceState(frameID,0);
                Q_EMIT SelfInspection(&self_inspection);
            }
                break;
            case 0x02://温度采集信息
            {
                PRE_TEM_VALUE tem_value;
                tem_value.id = frameID;
                tem_value.AmbientTem = CoreHelper::M117ByteToTemperature(frame[PRE_PACKET_DATA_OFFSET],frame[PRE_PACKET_DATA_OFFSET+1]);
                tem_value.CH_Count = frame[PRE_PACKET_DATA_OFFSET + 2];
                for(int i=PRE_PACKET_DATA_OFFSET + 3;i<len-1;i+=2){
                    tem_value.PointTem.append(CoreHelper::DS18B20ByteToTemperature(frame[i],frame[i+1]));
                }
                tem_value.TriggerTime = QDateTime::currentDateTime();
                //温度信息存入链表
                //环境温度、通道1、2、3、4、5…… 温度
                addTemperatures(frameID,0,tem_value.AmbientTem);
                for(int i=0;i<tem_value.PointTem.size();i++){
                    addTemperatures(frameID,i+1,tem_value.PointTem.at(i));
                }
                SetTaskState(frameID,TASKSTATE::WaitingSwitch);
                switchNextTask();
                UpdateDeviceState(frameID,0);
                Q_EMIT TemInspection(&tem_value);
            }
                break;
            case 0x03://振动数据采集完成
            {
                DBData::SpeedStruct.Bus0CanSave = false;
                //返回的其实还是温度数据 + 要接收的数据长度
                //温度信息需要存入链表
                double AmbientTem = CoreHelper::M117ByteToTemperature(frame[PRE_PACKET_DATA_OFFSET],frame[PRE_PACKET_DATA_OFFSET+1]);
                QVector<double> PointTem;
                for(int i=PRE_PACKET_DATA_OFFSET + 3;i<len-5;i+=2){
                    PointTem.append(CoreHelper::DS18B20ByteToTemperature(frame[i],frame[i+1]));
                }
                VIBDataLen = frame[len-5] << 24 | frame[len-4] << 16 | frame[len-3] << 8 | frame[len-2];
                //温度信息存入链表
                //环境温度、通道1、2、3、4、5…… 温度
                addTemperatures(frameID,0,AmbientTem);
                for(int i=0;i<PointTem.size();i++){
                    addTemperatures(frameID,i+1,PointTem.at(i));
                }
                currentEndSpeed = DBData::RunSpeed;
                SetTaskState(frameID,TASKSTATE::WaitingToObtain);
                UpdateDeviceState(frameID,0);
            }
                break;
            case 0x05://前置空闲状态
            {
                uint8_t state= frame[PRE_PACKET_DATA_OFFSET];
                //                qDebug()<< "0x05 命令状态为:"<<state;
                bool IsBusy = state;
                UpdateDeviceState(frameID,0);
                Q_EMIT PreState(frameID,IsBusy);
            }
                break;
            case 0x06://更新结果反馈
            {
                uint8_t state= frame[PRE_PACKET_DATA_OFFSET];
                bool IsFail = state;
                UpdateDeviceState(frameID,0);
                Q_EMIT UpdateResults(frameID,IsFail);
            }
                break;
            case 0x07://返回重启状态
            {
                uint8_t state= frame[PRE_PACKET_DATA_OFFSET];
                bool IsAbnormal = state;
                UpdateDeviceState(frameID,0);
                Q_EMIT RebootState(frameID,IsAbnormal);
            }
                break;
            case 0x08://返回单通道状态
            {
                uint8_t PackageCH= frame[PRE_PACKET_DATA_OFFSET];
                uint8_t state= frame[PRE_PACKET_DATA_OFFSET+1];
                Q_EMIT ReturnChannelStatus(frameID,PackageCH,state);
            }
                break;
            default:
                break;
            }
        }
    }
}

QByteArray Bus0API::GetSendPackage(uint8_t pre_id, uint8_t order, QByteArray data)
{
    QByteArray array;
    array.append(pre_id);
    array.append(order);
    array.append(data.size());
    array.append(data);
    array.append(CoreHelper::CheckSum(array,0,array.size()));

    if (array.size() % 4 != 0) {
        array.resize(array.size() + (4 - (array.size() % 4))); // 填充到下一个 4 字节倍数
    }
    qDebug()<<"can0 send :" << CoreHelper::byteArrayToHexStr(array);
    return array;
}

void Bus0API::SendData(const QCanBusFrame &frame)
{
    if(!m_canDevice) return;
    m_canDevice->writeFrame(frame);
}

double Bus0API::findTemperatures(uint8_t preid, uint8_t prech)
{
    double ret = 0.0;
    int Temcount = temperaturelist.size();
    for(int i=0;i<Temcount;i++){
        if(preid == temperaturelist.at(i).id){
            if(prech == temperaturelist.at(i).ch){
                ret = temperaturelist.at(i).value;
                break;
            }
        }
    }
    return ret;
}

void Bus0API::addTemperatures(uint8_t preid, uint8_t prech, double value)
{
    // 遍历列表查找是否存在符合条件的元素
    bool found = false;

    for (int i = 0; i < temperaturelist.size(); ++i) {
        TEMPERATURESTRUCT& temp = temperaturelist[i];

        // 如果找到了符合条件的元素，更新 value
        if (temp.id == preid && temp.ch == prech) {
            temp.value = value;
            found = true;
            break;  // 找到后退出循环
        }
    }

    // 如果没有找到符合条件的元素，则添加一个新元素
    if (!found) {
        TEMPERATURESTRUCT newTemp;
        newTemp.id = preid;
        newTemp.ch = prech;
        newTemp.value = value;
        temperaturelist.append(newTemp);
    }
}

void Bus0API::addTaskprelist(uint8_t preid, uint8_t channelsNumber,QVector<uint8_t> CH_Status)
{
    // 检查 channelsNumber 是否有效或者传入参数不匹配
    if (channelsNumber <= 0 || channelsNumber != CH_Status.size()) {
        qWarning() << "Invalid channelsNumber:" << channelsNumber;
        return;
    }
    if(!Bus0PreList.contains(preid)){
        Bus0PreList.append(preid);
    }
    // 如果存在该ID，则检查是否有新的通道加入
    for (auto &task : task_pre_list.prelist) {
        if (task.Pre_id == preid) {
            //对比
            QSet<uint8_t> channelSet(task.channellist.begin(), task.channellist.end());
            for (int i = 0; i < CH_Status.size(); ++i) {
                if (CH_Status.at(i) == 0) {
                    uint8_t value = i + 1;  // 下标 + 1
                    if (!channelSet.contains(value)) {
                        //判断数据库中是否已经使能该通道
                        int deviceindex = DBData::GetDeviceIndex(preid,i);
                        if(deviceindex != -1){//数据库中存在,判断是否使能
                            if(DBData::DeviceInfo_IsEnable.at(deviceindex)){
                                task.channellist.append(value);    // 添加到 channellist
                            }
                        }
                    }
                }
            }
            qWarning() << "Pre_id already exists:" << preid;
            return; // 跳过添加
        }
    }

    TASK_CHANNEL_LIST channellist;
    channellist.Pre_id = preid;
    channellist.status = TASKSTATE::idle;
    //    channellist.working_ch = (channelsNumber > 0) ? 1 : 0;
    channellist.working_ch = 0;
    for(int i=1;i<channelsNumber+1;i++){
        int deviceindex = DBData::GetDeviceIndex(preid,i);
        if(deviceindex != -1){//数据库中存在,判断是否使能
            if(DBData::DeviceInfo_IsEnable.at(deviceindex)){
                if(CH_Status.at(i-1) == 0){
                    channellist.channellist.append(i);
                    //判断工作通道是否已添加，若未添加则需要更新
                    if(channellist.working_ch == 0){
                        channellist.working_ch = i;
                    }
                    qDebug()<<"can 0 add Task" << preid << ": channel = " << i;
                }
            }
        }
    }
    //    qDebug()<<"can 0 add Task" << preid << ": channelsNumber = " << channelsNumber;
    //判断是否有工作通道，若无则不必添加至前置列表
    if(channellist.channellist.isEmpty()) return;
    task_pre_list.prelist.append(channellist);
    task_pre_list.working_pre = preid;
    task_pre_list.working_index = task_pre_list.prelist.size()-1;
}

void Bus0API::UpdateDeviceState(uint8_t preid, uint8_t prech, bool IsFault)
{
    //如果ch = 0，则表示这个id的所有通道均要更新
    int count = DBData::PreStatus.size();
    //数据格式：车厢号|前置处理器编号|通道编号|是否在线|报警状态|最后一次通信的时间
    for(int i=0; i < count; i++){
        QStringList list = DBData::PreStatus.at(i).split("|");
        QString Wagon = list.at(0);
        int oldid = list.at(1).toInt();
        int oldch = list.at(2).toInt();
        DBData::DeviceState oldState = static_cast<DBData::DeviceState>(list.at(3).toInt());
        DBData::DeviceState oldAlarm = static_cast<DBData::DeviceState>(list.at(4).toInt());
        if(preid == oldid){
            if((prech == oldch) || (prech == 0)){
                //上线需要增加日志
                if(oldState == DBData::DeviceState_Offline){
                    TaskAPI::Instance()->removeErrorprelist(oldid,oldch);
                    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"报警信息",DATETIME,QString("bus0设备(preid:%1,prech:%2) 上线").arg(preid).arg(prech));
                }
                DBData::DeviceState newstate = IsFault ? DBData::DeviceState_Error : DBData::DeviceState_Online;
                QString str = QString("%1|%2|%3|%4|%5|%6").arg(Wagon).arg(oldid).arg(oldch).arg(newstate).arg(oldAlarm).arg(DATETIME);
                DBData::PreStatus[i] = str;
                if(prech != 0){
                    break;
                }
            }
        }
    }
}

void Bus0API::CheckDimensionState(quint8 id, quint8 ch, quint8 axis, int type, float rmsvalue, float ppvalue)
{
    if(!APPSetting::UseDimensionalAlarm) return;
    switch (type) {
    case 0://轴箱
    {
        if(rmsvalue >= APPSetting::Axlebox_Rms_SecondaryAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",2,DATETIME,QString("(轴箱测点)RMS值二级报警:%1").arg(rmsvalue));
        }else if(rmsvalue >= APPSetting::Axlebox_Rms_FirstLevelAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",1,DATETIME,QString("(轴箱测点)RMS值一级报警:%1").arg(rmsvalue));
        }

        if(ppvalue >= APPSetting::Axlebox_PP_SecondaryAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",2,DATETIME,QString("(轴箱测点)PP值二级报警:%1").arg(ppvalue));
        }else if(ppvalue >= APPSetting::Axlebox_PP_FirstLevelAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",1,DATETIME,QString("(轴箱测点)PP值一级报警:%1").arg(ppvalue));
        }
    }
        break;
    case 1:
    {
        if(rmsvalue >= APPSetting::Gearbox_Rms_SecondaryAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",2,DATETIME,QString("(齿轮箱测点)RMS值二级报警:%1").arg(rmsvalue));
        }else if(rmsvalue >= APPSetting::Gearbox_Rms_FirstLevelAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",1,DATETIME,QString("(齿轮箱测点)RMS值一级报警:%1").arg(rmsvalue));
        }

        if(ppvalue >= APPSetting::Gearbox_PP_SecondaryAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",2,DATETIME,QString("(齿轮箱测点)PP值二级报警:%1").arg(ppvalue));
        }else if(ppvalue >= APPSetting::Gearbox_PP_FirstLevelAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",1,DATETIME,QString("(齿轮箱测点)PP值一级报警:%1").arg(ppvalue));
        }
    }
        break;
    case 2:
    {
        if(rmsvalue >= APPSetting::Motor_Rms_SecondaryAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",2,DATETIME,QString("(电机测点)RMS值二级报警:%1").arg(rmsvalue));
        }else if(rmsvalue >= APPSetting::Motor_Rms_FirstLevelAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",1,DATETIME,QString("(电机测点)RMS值一级报警:%1").arg(rmsvalue));
        }

        if(ppvalue >= APPSetting::Motor_PP_SecondaryAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",2,DATETIME,QString("(电机测点)PP值二级报警:%1").arg(ppvalue));
        }else if(ppvalue >= APPSetting::Motor_PP_FirstLevelAlarm){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,id,ch,axis,"null","报警信息",1,DATETIME,QString("(电机测点)PP值一级报警:%1").arg(ppvalue));
        }
    }
        break;
    default:
        break;
    }
}

quint8 Bus0API::CheckRMSState(int type, float rmsvalue)
{
    quint8 result = 0;
    if(!APPSetting::UseDimensionalAlarm){
        return result;
    }
    switch (type){
    case 0:
    {
        if(rmsvalue >= APPSetting::Axlebox_Rms_SecondaryAlarm){
            result = 2;
        }else if(rmsvalue >= APPSetting::Axlebox_Rms_FirstLevelAlarm){
            result = 1;
        }
    }
        break;
    case 1:
    {
        if(rmsvalue >= APPSetting::Gearbox_Rms_SecondaryAlarm){
            result = 2;
        }else if(rmsvalue >= APPSetting::Gearbox_Rms_FirstLevelAlarm){
            result = 1;
        }
    }
        break;
    case 2:
    {
        if(rmsvalue >= APPSetting::Motor_Rms_SecondaryAlarm){
            result = 2;
        }else if(rmsvalue >= APPSetting::Motor_Rms_FirstLevelAlarm){
            result = 1;
        }
    }
        break;
    default:
        break;
    }
    return result;
}

quint8 Bus0API::CheckPPState(int type, float ppvalue)
{
    quint8 result = 0;
    if(!APPSetting::UseDimensionalAlarm){
        return result;
    }
    switch (type) {
    case 0://轴箱
    {
        if(ppvalue >= APPSetting::Axlebox_PP_SecondaryAlarm){
            result = 2;
        }else if(ppvalue >= APPSetting::Axlebox_PP_FirstLevelAlarm){
            result = 1;
        }
    }
        break;
    case 1:
    {
        if(ppvalue >= APPSetting::Gearbox_PP_SecondaryAlarm){
            result = 2;
        }else if(ppvalue >= APPSetting::Gearbox_PP_FirstLevelAlarm){
            result = 1;
        }
    }
        break;
    case 2:
    {
        if(ppvalue >= APPSetting::Motor_PP_SecondaryAlarm){
            result = 2;
        }else if(ppvalue >= APPSetting::Motor_PP_FirstLevelAlarm){
            result = 1;
        }
    }
        break;
    default:
        break;
    }
    return result;
}

void Bus0API::CommandSelfInspection(uint8_t PreID)
{
    QByteArray array;
#if 0
    array.append(QDateTime::currentDateTime().date().year()-2000);//年
    array.append(QDateTime::currentDateTime().date().month());//月
    array.append(QDateTime::currentDateTime().date().day());//日
    array.append(QDateTime::currentDateTime().time().hour());//时
    array.append(QDateTime::currentDateTime().time().minute());//分
    array.append(QDateTime::currentDateTime().time().second());//秒
#else
    //2025-03-03修改为发送嵌入式主机类型 0x00 表示主机或备用主机 0x01表示从机
    if(APPSetting::WagonType == "Slave"){
        array.append(0x01);
    }else if(APPSetting::WagonType == "StandbyHost" || APPSetting::WagonType == "Host"){
        array.append(data_zero);
    }else{
        array.append(0xFF);
    }
#endif
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PreID,0x01,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);

    SendData(frame);
}

void Bus0API::CommandTemInspection(uint8_t PreID)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PreID,0x02,QByteArray()));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);

    SendData(frame);
    SetTaskState(PreID,TASKSTATE::Collecting);
}

void Bus0API::CommandVibInspection(uint8_t PreID, uint8_t PreCH)
{
    QByteArray array;
    array.append(PreCH);
    array.append((uint8_t)(APPSetting::VibrateSamplingFrequency >> 24) & 0xff);
    array.append((uint8_t)(APPSetting::VibrateSamplingFrequency >> 16) & 0xff);
    array.append((uint8_t)(APPSetting::VibrateSamplingFrequency >> 8) & 0xff);
    array.append((uint8_t)APPSetting::VibrateSamplingFrequency & 0xff);

    array.append((uint8_t)(APPSetting::VibrateSamplingPoints >> 24) & 0xff);
    array.append((uint8_t)(APPSetting::VibrateSamplingPoints >> 16) & 0xff);
    array.append((uint8_t)(APPSetting::VibrateSamplingPoints >> 8) & 0xff);
    array.append((uint8_t)APPSetting::VibrateSamplingPoints & 0xff);

    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PreID,0x03,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);

    currentStartSpeed = DBData::RunSpeed;
    SendData(frame);
    SetTaskState(PreID,TASKSTATE::Collecting);
    DBData::SpeedStruct.Bus0SpeedVector.clear();
    DBData::SpeedStruct.Bus0CanSave = true;
}

void Bus0API::CommandReadVibData(uint8_t PreID, uint8_t PreCH)
{
    QByteArray array;
    array.append(PreCH);

    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PreID,0x04,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);

    SendData(frame);
    currentpreID = PreID;
    currentpreCH = PreCH;
    VibdataArr.clear();
    SetTaskState(PreID,TASKSTATE::Obtaining);
}

void Bus0API::CommandPreUpdate(uint8_t PreID, uint32_t crc, int len)
{
    QByteArray array;
    array.append((uint8_t)(len >> 24) & 0xff);
    array.append((uint8_t)(len >> 16) & 0xff);
    array.append((uint8_t)(len >> 8) & 0xff);
    array.append((uint8_t)len & 0xff);

    array.append((uint8_t)(crc >> 24) & 0xff);
    array.append((uint8_t)(crc >> 16) & 0xff);
    array.append((uint8_t)(crc >> 8) & 0xff);
    array.append((uint8_t)crc & 0xff);

    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PreID,0x05,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);
    SendData(frame);
}

void Bus0API::CommandSendFile(QString filename)
{
    QFile file(filename);
    qint64 fileSize = file.size();
    //检测文件是否存在已经文件能否打开
    if (fileSize == 0 || !file.open(QIODevice::ReadOnly)) {
        return;
    }
    QByteArray block;
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    do{
        block.clear();
        block = file.read(0x40);//每次读取64个数据
        // 如果读取的内容为空，则跳过
        if (block.isEmpty()) {
            break;
        }
        frame.setPayload(block);
        SendData(frame);
        //        QThread::usleep(800);
        QThread::msleep(5);

    }while(!file.atEnd());
    file.close();
}

void Bus0API::CommandReboot(uint8_t PreID)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(PreID,0x07,QByteArray()));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);

    SendData(frame);
}

QByteArray Bus0API::GetTaskState()
{
    if (task_pre_list.prelist.isEmpty()) {
        return QByteArray();
    }
    QByteArray array;

    const TASK_CHANNEL_LIST &task = task_pre_list.prelist.at(task_pre_list.working_index); // 获取当前任务
    array.append(task.Pre_id);
    array.append(task.working_ch);
    array.append(static_cast<uint8_t>(task.status));
    return array;
}

QByteArray Bus0API::switchNextTask()
{
    QByteArray array;
    if (task_pre_list.prelist.isEmpty()) {
        qWarning() << "No tasks in the prelist.";
        return QByteArray(); // 或者直接返回空数组
    }

    const int precount = task_pre_list.prelist.size();

    int index = task_pre_list.working_index;
    for (int i = 0; i < precount; i++) {
        int currentIndex = (index + i + 1) % precount; // 环形查找索引
        auto &task = task_pre_list.prelist[currentIndex];
        if ((task.status != TASKSTATE::Collecting) && (task.status != TASKSTATE::Obtaining)) { // 找到非工作状态的任务
            array.append(task.Pre_id);
            array.append(task.working_ch);
            array.append(static_cast<uint8_t>(task.status));
            task_pre_list.working_pre = task.Pre_id;
            task_pre_list.working_index = currentIndex;
            //               qDebug()<<"switchNextTask" << "working_pre = " << task_pre_list.working_pre << "working_index = " << task_pre_list.working_index
            //                      << "new state = "<< task.status;
            break;
        }
    }

    return array;
}

void Bus0API::switchNextChannel(uint8_t preid)
{
    if (task_pre_list.prelist.isEmpty()) {
        qWarning() << "No tasks in the prelist.";
        return; // 直接返回
    }

    for (auto &task : task_pre_list.prelist) { // 遍历使用引用
        if (task.Pre_id == preid && task.status == TASKSTATE::WaitingSwitch) { //只有等待切换状态才可以切换通道
            if (task.channellist.isEmpty()) {
                qDebug()<<"no channel in the Pre";
                break; // 通道列表为空，不执行操作
            }

#if 1
            task.working_index = (task.working_index + 1) % task.channellist.size(); // 环形切换
            task.working_ch = task.channellist.at(task.working_index);
            task.status = TASKSTATE::idle; // 重置状态
            qDebug()<<"bus0 switchNextChannel" << "working_id = " << task.Pre_id << "working_ch = " << task.working_ch << "new state = "
                   << task.status;
            break;
#else
            //增加通道是否故障判断
            // 环形切换，检查当前通道是否在故障列表中
            int startIndex = task.working_index;
            for (int i = 0; i < task.channellist.size(); ++i) {
                int currentIndex = (startIndex + i + 1) % task.channellist.size();
                task.working_index = currentIndex;
                task.working_ch = task.channellist.at(currentIndex);
                qDebug()<<"switchNextChannel: working_index = " << task.working_index << "working_ch = " << task.working_ch;
                // 检查当前通道是否在故障列表中
                if (!TaskAPI::Instance()->existErrorprelist(task.Pre_id, task.working_ch)) {
                    task.status = TASKSTATE::idle; // 重置状态
                    break;
                } else {
                    qWarning() << "Channel" << task.working_ch << "in preprocessor" << task.Pre_id << "is faulty, skipping to next channel.";
                }
            }
            break;
#endif
        }else{
            //            qDebug()<< " task.status = " << task.status;
        }
    }
}

void Bus0API::SetTaskState(uint8_t preid, TASKSTATE state)
{
    if (task_pre_list.prelist.isEmpty()) {
        qWarning() << "No tasks in the prelist.";
        return; // 直接返回
    }

    for (int i = 0; i < task_pre_list.prelist.size(); ++i) {
        TASK_CHANNEL_LIST &channel = task_pre_list.prelist[i]; // 引用当前的通道结构体

        // 判断是否是目标前置ID
        if (channel.Pre_id == preid) {
            // 更新状态
            channel.status = state;
            //            qDebug()<<"SetTaskState  preid = " << preid << "new state = " << state;
            //如果新状态为等待切换的话，需要切换下一个通道
            if(state == TASKSTATE::WaitingSwitch){
                switchNextChannel(preid);
            }
            return; // 找到后退出
        }
    }
}

bool Bus0API::TaskIsContain(uint8_t preid)
{
#if 0
    int len = task_pre_list.prelist.size();
    for(int i=0;i<len;i++){
        if(preid == task_pre_list.prelist.at(i).Pre_id){
            return true;
        }
    }
    return false;
#else
    return Bus0PreList.contains(preid);
#endif
}

QList<uint8_t> Bus0API::GetPreDeviceList()
{
    QList<uint8_t> retlist;
    int len = task_pre_list.prelist.size();
    for(int i=0;i<len;i++){
        retlist.append(task_pre_list.prelist.at(i).Pre_id);
    }
    return retlist;
}

bool Bus0API::BusIsOpen()
{
    return this->IsOpen;
}

void Bus0API::LowPowerMode()
{
    for (int pre = 0; pre < task_pre_list.prelist.size(); pre++) {
        TASK_CHANNEL_LIST &device = task_pre_list.prelist[pre]; // 直接访问设备
        for (int ch = 0; ch < device.channellist.size(); ch++) {
            int id = device.Pre_id;
            int channel = device.channellist[ch];
            if (!TaskAPI::Instance()->existErrorprelist(id, ch)) {
                UpdateDeviceState(id, channel);
            }
        }
    }
}

void Bus0API::ResetTaskState()
{
    if (task_pre_list.prelist.isEmpty()) {
        return;
    }

    VibdataArr.clear();
    for (int pre = 0; pre < task_pre_list.prelist.size(); pre++) {
        TASK_CHANNEL_LIST &device = task_pre_list.prelist[pre]; // 直接访问设备
        device.status = TASKSTATE::idle;
    }

    currentpreID = 0;
    currentpreCH = 0;
    currentStartSpeed = 0;
    currentEndSpeed = 0;
    VIBDataLen = 0;
    VibdataArr.clear();
}

void Bus0API::QueryCHState(uint8_t id, uint8_t ch)
{
    QByteArray array;
    array.append(ch);

    QCanBusFrame frame;
    frame.setFrameId(CAN_Default_FrameID);
    frame.setPayload(GetSendPackage(id,0x08,array));
    frame.setExtendedFrameFormat(false);
    frame.setFlexibleDataRateFormat(true);
    frame.setBitrateSwitch(false);
    SendData(frame);
}
