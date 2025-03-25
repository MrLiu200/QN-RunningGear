#include "dbdata.h"
#include <QFile>
SPEEDSTRUCT DBData::SpeedStruct = {false,QVector<uint32_t>(),false,QVector<uint32_t>()};
QList<QString> DBData::PreStatus = QList<QString>();
#ifndef DEVICE_SLAVE
QList<QString> DBData::SlaveStatus = QList<QString>();
#endif
QList<QString> DBData::OtherBoardStatus = QList<QString>();

QList<QString> DBData::Task_Action = QList<QString>();
QList<QString> DBData::Task_TriggerTime = QList<QString>();

QList<QString> DBData::BearingInfo_Model = QList<QString>();
QList<float> DBData::BearingInfo_PitchDiameter = QList<float>();
QList<float> DBData::BearingInfo_RollingDiameter = QList<float>();
QList<int> DBData::BearingInfo_RollerNum = QList<int>();
QList<float> DBData::BearingInfo_ContactAngle = QList<float>();


int DBData::DeviceInfo_Count = 0;
QList<int> DBData::DeviceInfo_DeviceID = QList<int>();
QList<int> DBData::DeviceInfo_DeviceCH = QList<int>();
QList<QString> DBData::DeviceInfo_DeviceName = QList<QString>();
QList<QString> DBData::DeviceInfo_DeviceType = QList<QString>();
QList<float> DBData::DeviceInfo_Sensitivity = QList<float>();
QList<float> DBData::DeviceInfo_ShaftDiameter = QList<float>();
QList<QString> DBData::DeviceInfo_bearing1Name = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing1_model = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing2Name = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing2_model = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing3Name = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing3_model = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing4Name = QList<QString>();
QList<QString> DBData::DeviceInfo_bearing4_model = QList<QString>();
QList<QString> DBData::DeviceInfo_capstanName = QList<QString>();
QList<int> DBData::DeviceInfo_capstanTeethNum = QList<int>();
QList<QString> DBData::DeviceInfo_DrivenwheelName = QList<QString>();
QList<int> DBData::DeviceInfo_DrivenwheelTeethNum = QList<int>();
QList<bool> DBData::DeviceInfo_IsEnable = QList<bool>();
QList<QString> DBData::DeviceInfo_version = QList<QString>();

QList<QString> DBData::TcpSendList = QList<QString>();
QMutex DBData::mutex;

int DBData::vibfilecount = 0;
int DBData::tmpfilecount = 0;
uint32_t DBData::RunSpeed = 600;

//QList<int> DBData::ZhouLimit_Warning = QList<int>();
QList<int> DBData::ZhouLimit_First = QList<int>();
QList<int> DBData::ZhouLimit_Secondary = QList<int>();
//QList<int> DBData::OtherLimit_Warning = QList<int>();
QList<int> DBData::OtherLimit_First = QList<int>();
QList<int> DBData::OtherLimit_Secondary = QList<int>();


qint8 DBData::CurrentAlarmLevel = -1;
bool DBData::PreIsAlarm = false;
//bool DBData::IOPowerEnable = true;

QVector<float> DBData::QueryBearingParameters(QString model)
{
    //数据顺序：中径、滚径、滚子数、接触角
    QVector<float> ret;
    int index = -1;
    int bearingnum = BearingInfo_Model.size();
    for(int i=0;i<bearingnum;i++){
        if(BearingInfo_Model.at(i) == model){
            index = i;
            break;
        }
    }
    if(index != -1){
        ret.append(BearingInfo_PitchDiameter.at(index));
        ret.append(BearingInfo_RollingDiameter.at(index));
        ret.append(BearingInfo_RollerNum.at(index));
        ret.append(BearingInfo_ContactAngle.at(index));
    }else{//新增如果没有该型号，则传入默认第一个型号的参数
        if(bearingnum!=0){
            ret.append(BearingInfo_PitchDiameter.at(0));
            ret.append(BearingInfo_RollingDiameter.at(0));
            ret.append(BearingInfo_RollerNum.at(0));
            ret.append(BearingInfo_ContactAngle.at(0));
        }
    }
    return ret;
}

int DBData::GetDeviceIndex(int Preid, int PreCH)
{
    int index = -1;
    for(int i=0;i<DeviceInfo_Count;i++){
        if(Preid == DeviceInfo_DeviceID.at(i)){
            if(PreCH == DeviceInfo_DeviceCH.at(i)){
                index = i;
            }
        }
    }
    return index;
}

void DBData::TCPAddSendFile(QString filename)
{
    QMutexLocker lock(&mutex);
    TcpSendList.append(filename);
}

void DBData::TCPDeleteSendFile(int index)
{
    QMutexLocker lock(&mutex);
    //获取文件名称
    QString filename = TcpSendList.at(index);
    TcpSendList.removeAt(index);
#ifdef DEVICE_SLAVE
    //从机需要删除源文件
//    if(QFile::exists(filename)){
//        QFile::remove(filename);
//    }
#endif
}


QList<QString> DBData::LinkDeviceInfo_Type = QList<QString>();
QList<QString> DBData::LinkDeviceInfo_IP = QList<QString>();
QList<int> DBData::LinkDeviceInfo_Port = QList<int>();
QList<QString> DBData::LinkDeviceInfo_WagonNumber = QList<QString>();



