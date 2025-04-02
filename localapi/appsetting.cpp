#include "appsetting.h"
#include "corehelper.h"
#include <QSettings>
bool APPSetting::IsHardDiskReady = false;
QString APPSetting::dbfile = "RunningGear.db";
QString APPSetting::configFile = "config.ini";
QString APPSetting::version = "1.0.0";
#ifndef DEVICE_SLAVE
QString APPSetting::WagonType = "Host";
#else
QString APPSetting::WagonType = "Slave";
#endif
QString APPSetting::projectName = "KM4";

int APPSetting::LineNumber = 0;
QString APPSetting::CarNumber = "T1214";
QString APPSetting::WagonNumber = "QN-t0021";
double APPSetting::WheelDiameter = 0.82;
double APPSetting::SpeedCoefficient = 1.00;

QString APPSetting::CanPlugin = "socketcan";
QString APPSetting::can0_Mode = "canfd";
QString APPSetting::can0_Name = "can0";
QString APPSetting::can0_bitrate = "1000000";
QString APPSetting::can0_bitrateSample = "0.75";
QString APPSetting::can0_dbitrate = "2000000";
QString APPSetting::can0_dbitrateSample = "0.80";

QString APPSetting::can1_Mode = "canfd";
QString APPSetting::can1_Name = "can1";
QString APPSetting::can1_bitrate = "1000000";
QString APPSetting::can1_bitrateSample = "0.75";
QString APPSetting::can1_dbitrate = "2000000";
QString APPSetting::can1_dbitrateSample = "0.80";

QString APPSetting::can2_Mode = "can";
QString APPSetting::can2_Name = "can2";
QString APPSetting::can2_bitrate = "1000000";
QString APPSetting::can2_bitrateSample = "0.75";
QString APPSetting::can2_dbitrate = "2000000";
QString APPSetting::can2_dbitrateSample = "0.80";

int APPSetting::CANVibrateDataLen = 262144;
int APPSetting::VibrateSamplingPoints = 65536;
int APPSetting::VibrateSamplingFrequency = 20480;

QString APPSetting::LocalIP_eth0 = "192.168.3.233";
QString APPSetting::LocalMask_eth0 = "255.255.255.0";
QString APPSetting::LocalGateway_eth0 = "192.168.3.1";
QString APPSetting::LocalDNS_eth0 = "192.168.3.1";

QString APPSetting::LocalIP_eth1 = "192.168.1.233";
QString APPSetting::LocalMask_eth1 = "255.255.255.0";
QString APPSetting::LocalGateway_eth1 = "192.168.1.1";
QString APPSetting::LocalDNS_eth1 = "192.168.1.1";

QString APPSetting::TcpServerIP = "192.168.3.123";
int APPSetting::TcpServerPort = 6000;
int APPSetting::TcpListenPort = 6000;
#ifdef DEVICE_SLAVE
QString APPSetting::TcpServerIPStandby = "192.168.0.123";
int APPSetting::TcpServerPortStandby = 6900;
#endif

QString APPSetting::UDPMulticastIP = "239.255.43.21";
int APPSetting::UDPListenPort = 6500;
int APPSetting::HeartbeatInterval = 25000;
int APPSetting::SynchronousInterval = 1000;

int APPSetting::HMIListenPort = 5800;

int APPSetting::baowaiWarning = 25;
int APPSetting::baoneiWarning = 25;
int APPSetting::waihuanWarning = 25;
int APPSetting::neihuanWarning = 25;
int APPSetting::gundanWarning = 25;
int APPSetting::gunshuangWarning = 25;
int APPSetting::treadWarning = 25;
int APPSetting::benzhouchilunWarning = 25;
int APPSetting::linzhouchilunWarning = 25;

int APPSetting::baowaiFirstLevelAlarm = 29;
int APPSetting::baoneiFirstLevelAlarm = 29;
int APPSetting::waihuanFirstLevelAlarm = 29;
int APPSetting::neihuanFirstLevelAlarm = 29;
int APPSetting::gundanFirstLevelAlarm = 29;
int APPSetting::gunshuangFirstLevelAlarm = 29;
int APPSetting::treadFirstLevelAlarm = 60;
int APPSetting::benzhouchilunFirstLevelAlarm = 30;
int APPSetting::linzhouchilunFirstLevelAlarm = 30;

int APPSetting::baowaiSecondaryAlarm = 36;
int APPSetting::baoneiSecondaryAlarm = 36;
int APPSetting::waihuanSecondaryAlarm = 1000;
int APPSetting::neihuanSecondaryAlarm = 1000;
int APPSetting::gundanSecondaryAlarm = 36;
int APPSetting::gunshuangSecondaryAlarm = 36;
int APPSetting::treadSecondaryAlarm = 65;
int APPSetting::benzhouchilunSecondaryAlarm = 75;
int APPSetting::linzhouchilunSecondaryAlarm = 77;

int APPSetting::AbsoluteTemperatureThreshold = 90;
int APPSetting::RelativeTemperatureThreshold = 50;
int APPSetting::AbsoluteTemperatureWarning = 80;
int APPSetting::MemorySpaceRatio = 70;

double APPSetting::Axlebox_Rms_FirstLevelAlarm = 1000.00;
double APPSetting::Gearbox_Rms_FirstLevelAlarm = 1000.00;
double APPSetting::Motor_Rms_FirstLevelAlarm = 1000.00;

double APPSetting::Axlebox_Rms_SecondaryAlarm = 1000.00;
double APPSetting::Gearbox_Rms_SecondaryAlarm = 1000.00;
double APPSetting::Motor_Rms_SecondaryAlarm = 1000.00;

double APPSetting::Axlebox_PP_FirstLevelAlarm = 1000.00;
double APPSetting::Gearbox_PP_FirstLevelAlarm = 1000.00;
double APPSetting::Motor_PP_FirstLevelAlarm = 1000.00;

double APPSetting::Axlebox_PP_SecondaryAlarm = 1000.00;
double APPSetting::Gearbox_PP_SecondaryAlarm = 1000.00;
double APPSetting::Motor_PP_SecondaryAlarm = 1000.00;

bool APPSetting::UseDimensionalAlarm = true;
bool APPSetting::UseVibrateWarning = true;
bool APPSetting::UseVibrateFirstLevelAlarm = true;
bool APPSetting::UseVibrateSecondaryAlarm = true;
bool APPSetting::UseTemperatureAlarm = true;
bool APPSetting::UseOfflineAlarm = true;
bool APPSetting::UseExternalSpeed = true;

int APPSetting::TmpTaskSpeedHourMax = 20;
int APPSetting::LowPowerTaskSpeedHourMax = 5;
bool APPSetting::UseFileInterval = false;
int APPSetting::VIBSaveInterval = 10;
int APPSetting::TMPSaveInterval = 20;
QString APPSetting::PackagingDate = QDate::currentDate().toString("yyyy-MM-dd");

int APPSetting::TimeoutInterval_Pre = 60;
int APPSetting::TimeoutInterval_Slave = 60;
int APPSetting::TimeoutInterval_OtherBrade = 60;
int APPSetting::TimeoutInterval_UDPUpdateFile = 180000;

QString APPSetting::FtpDirName = "/udisk/ftptemp";

QString APPSetting::Led_Run = "AppRun";
QString APPSetting::Led_PreError = "Pre";
QString APPSetting::Led_alarm1 = "AlarmLevel_1";
QString APPSetting::Led_alarm2 = "AlarmLevel_2";

bool APPSetting::IOPowerEnable = true;

int APPSetting::CheckCanInterval = 5000;
int APPSetting::CheckSlaceInterval = 5000;
int APPSetting::CheckOtherBoardInterval = 5000;
int APPSetting::CheckOtherTaskInterval = 5000;
int APPSetting::CheckPISStateInterval = 3000;
int APPSetting::CanBusTaskInterval = 2000;
int APPSetting::CanBusUpdateTaskInterval = 1000;
int APPSetting::TcpSendFileInterval = 1500;
int APPSetting::WatchDogInterval = 3000;

void APPSetting::ReadConfig()
{
    if(!checkConfig()){
        return;
    }
    QSettings set(APPSetting::configFile,QSettings::IniFormat);

    set.beginGroup("softwareConfig");
    APPSetting::version = set.value("version").toString();
    APPSetting::WagonType = set.value("WagonType").toString();
    set.endGroup();

    set.beginGroup("CarConfig");
    APPSetting::projectName = set.value("projectName").toString();
    APPSetting::LineNumber = set.value("LineNumber").toInt();
    APPSetting::CarNumber = set.value("CarNumber").toString();
    APPSetting::WagonNumber = set.value("WagonNumber").toString();
    APPSetting::WheelDiameter = set.value("WheelDiameter").toDouble();
    APPSetting::SpeedCoefficient = set.value("SpeedCoefficient").toDouble();
    set.endGroup();

    set.beginGroup("CanConfig");
    APPSetting::CanPlugin = set.value("CanPlugin").toString();

    APPSetting::can0_Mode = set.value("can0_Mode").toString();
    APPSetting::can0_Name = set.value("can0_Name").toString();
    APPSetting::can0_bitrate = set.value("can0_bitrate").toString();
    APPSetting::can0_bitrateSample = set.value("can0_bitrateSample").toString();
    APPSetting::can0_dbitrate = set.value("can0_dbitrate").toString();
    APPSetting::can0_dbitrateSample = set.value("can0_dbitrateSample").toString();

    APPSetting::can1_Mode = set.value("can1_Mode").toString();
    APPSetting::can1_Name = set.value("can1_Name").toString();
    APPSetting::can1_bitrate = set.value("can1_bitrate").toString();
    APPSetting::can1_bitrateSample = set.value("can1_bitrateSample").toString();
    APPSetting::can1_dbitrate = set.value("can1_dbitrate").toString();
    APPSetting::can1_dbitrateSample = set.value("can1_dbitrateSample").toString();

    APPSetting::can2_Mode = set.value("can2_Mode").toString();
    APPSetting::can2_Name = set.value("can2_Name").toString();
    APPSetting::can2_bitrate = set.value("can2_bitrate").toString();
    APPSetting::can2_bitrateSample = set.value("can2_bitrateSample").toString();
    APPSetting::can2_dbitrate = set.value("can2_dbitrate").toString();
    APPSetting::can2_dbitrateSample = set.value("can2_dbitrateSample").toString();

    APPSetting::CANVibrateDataLen = set.value("CANVibrateDataLen").toInt();
    APPSetting::VibrateSamplingPoints = set.value("VibrateSamplingPoints").toInt();
    APPSetting::VibrateSamplingFrequency = set.value("VibrateSamplingFrequency").toInt();
    set.endGroup();

    set.beginGroup("LocalNetConfig");
    APPSetting::LocalIP_eth0 = set.value("LocalIP_eth0").toString();
    APPSetting::LocalMask_eth0 = set.value("LocalMask_eth0").toString();
    APPSetting::LocalGateway_eth0 = set.value("LocalGateway_eth0").toString();
    APPSetting::LocalDNS_eth0 = set.value("LocalDNS_eth0").toString();
    APPSetting::LocalIP_eth1 = set.value("LocalIP_eth1").toString();
    APPSetting::LocalMask_eth1 = set.value("LocalMask_eth1").toString();
    APPSetting::LocalGateway_eth1 = set.value("LocalGateway_eth1").toString();
    APPSetting::LocalDNS_eth1 = set.value("LocalDNS_eth1").toString();
    set.endGroup();

    set.beginGroup("TCPConfig");
    APPSetting::TcpServerIP = set.value("TcpServerIP").toString();
    APPSetting::TcpServerPort = set.value("TcpServerPort").toInt();
    APPSetting::TcpListenPort = set.value("TcpListenPort").toInt();
#ifdef DEVICE_SLAVE
    APPSetting::TcpServerIPStandby = set.value("TcpServerIPStandby").toString();
    APPSetting::TcpServerPortStandby = set.value("TcpServerPortStandby").toInt();
#endif
    set.endGroup();

    set.beginGroup("UDPConfig");
    APPSetting::UDPMulticastIP = set.value("UDPMulticastIP").toString();
    APPSetting::UDPListenPort = set.value("UDPListenPort").toInt();
    APPSetting::HeartbeatInterval = set.value("HeartbeatInterval").toInt();
    APPSetting::SynchronousInterval = set.value("SynchronousInterval").toInt();
    set.endGroup();

    set.beginGroup("HMIConfig");
    APPSetting::HMIListenPort = set.value("HMIListenPort").toInt();
    set.endGroup();

    set.beginGroup("WarningConfig");
    APPSetting::baowaiWarning = set.value("baowaiWarning").toInt();
    APPSetting::baoneiWarning = set.value("baoneiWarning").toInt();
    APPSetting::waihuanWarning = set.value("waihuanWarning").toInt();
    APPSetting::neihuanWarning = set.value("neihuanWarning").toInt();
    APPSetting::gundanWarning = set.value("gundanWarning").toInt();
    APPSetting::gunshuangWarning = set.value("gunshuangWarning").toInt();
    APPSetting::treadWarning = set.value("treadWarning").toInt();
    APPSetting::benzhouchilunWarning = set.value("benzhouchilunWarning").toInt();
    APPSetting::linzhouchilunWarning = set.value("linzhouchilunWarning").toInt();
    set.endGroup();

    set.beginGroup("FirstLevelAlarmConfig");
    APPSetting::baowaiFirstLevelAlarm = set.value("baowaiFirstLevelAlarm").toInt();
    APPSetting::baoneiFirstLevelAlarm = set.value("baoneiFirstLevelAlarm").toInt();
    APPSetting::waihuanFirstLevelAlarm = set.value("waihuanFirstLevelAlarm").toInt();
    APPSetting::neihuanFirstLevelAlarm = set.value("neihuanFirstLevelAlarm").toInt();
    APPSetting::gundanFirstLevelAlarm = set.value("gundanFirstLevelAlarm").toInt();
    APPSetting::gunshuangFirstLevelAlarm = set.value("gunshuangFirstLevelAlarm").toInt();
    APPSetting::treadFirstLevelAlarm = set.value("treadFirstLevelAlarm").toInt();
    APPSetting::benzhouchilunFirstLevelAlarm = set.value("benzhouchilunFirstLevelAlarm").toInt();
    APPSetting::linzhouchilunFirstLevelAlarm = set.value("linzhouchilunFirstLevelAlarm").toInt();
    set.endGroup();

    set.beginGroup("SecondaryAlarmConfig");
    APPSetting::baowaiSecondaryAlarm = set.value("baowaiSecondaryAlarm").toInt();
    APPSetting::baoneiSecondaryAlarm = set.value("baoneiSecondaryAlarm").toInt();
    APPSetting::waihuanSecondaryAlarm = set.value("waihuanSecondaryAlarm").toInt();
    APPSetting::neihuanSecondaryAlarm = set.value("neihuanSecondaryAlarm").toInt();
    APPSetting::gundanSecondaryAlarm = set.value("gundanSecondaryAlarm").toInt();
    APPSetting::gunshuangSecondaryAlarm = set.value("gunshuangSecondaryAlarm").toInt();
    APPSetting::treadSecondaryAlarm = set.value("treadSecondaryAlarm").toInt();
    APPSetting::benzhouchilunSecondaryAlarm = set.value("benzhouchilunSecondaryAlarm").toInt();
    APPSetting::linzhouchilunSecondaryAlarm = set.value("linzhouchilunSecondaryAlarm").toInt();
    set.endGroup();

    set.beginGroup("OtherAlarmConfig");
    APPSetting::AbsoluteTemperatureThreshold = set.value("AbsoluteTemperatureThreshold").toInt();
    APPSetting::RelativeTemperatureThreshold = set.value("RelativeTemperatureThreshold").toInt();
    APPSetting::AbsoluteTemperatureWarning = set.value("AbsoluteTemperatureWarning").toInt();
    APPSetting::MemorySpaceRatio = set.value("MemorySpaceRatio").toInt();
    set.endGroup();

    set.beginGroup("DimensionalAlarmConfig");
    APPSetting::Axlebox_Rms_FirstLevelAlarm = set.value("Axlebox_Rms_FirstLevelAlarm").toDouble();
    APPSetting::Gearbox_Rms_FirstLevelAlarm = set.value("Gearbox_Rms_FirstLevelAlarm").toDouble();
    APPSetting::Motor_Rms_FirstLevelAlarm = set.value("Motor_Rms_FirstLevelAlarm").toDouble();
    APPSetting::Axlebox_Rms_SecondaryAlarm = set.value("Axlebox_Rms_SecondaryAlarm").toDouble();
    APPSetting::Gearbox_Rms_SecondaryAlarm = set.value("Gearbox_Rms_SecondaryAlarm").toDouble();
    APPSetting::Motor_Rms_SecondaryAlarm = set.value("Motor_Rms_SecondaryAlarm").toDouble();
    APPSetting::Axlebox_PP_FirstLevelAlarm = set.value("Axlebox_PP_FirstLevelAlarm").toDouble();
    APPSetting::Gearbox_PP_FirstLevelAlarm = set.value("Gearbox_PP_FirstLevelAlarm").toDouble();
    APPSetting::Motor_PP_FirstLevelAlarm = set.value("Motor_PP_FirstLevelAlarm").toDouble();
    APPSetting::Axlebox_PP_SecondaryAlarm = set.value("Axlebox_PP_SecondaryAlarm").toDouble();
    APPSetting::Gearbox_PP_SecondaryAlarm = set.value("Gearbox_PP_SecondaryAlarm").toDouble();
    APPSetting::Motor_PP_SecondaryAlarm = set.value("Motor_PP_SecondaryAlarm").toDouble();
    set.endGroup();

    set.beginGroup("AlarmSwitchConfig");
    APPSetting::UseDimensionalAlarm = set.value("UseDimensionalAlarm").toBool();
    APPSetting::UseVibrateWarning = set.value("UseVibrateWarning").toBool();
    APPSetting::UseVibrateFirstLevelAlarm = set.value("UseVibrateFirstLevelAlarm").toBool();
    APPSetting::UseVibrateSecondaryAlarm = set.value("UseVibrateSecondaryAlarm").toBool();
    APPSetting::UseTemperatureAlarm = set.value("UseTemperatureAlarm").toBool();
    APPSetting::UseOfflineAlarm = set.value("UseOfflineAlarm").toBool();
    APPSetting::UseExternalSpeed = set.value("UseExternalSpeed").toBool();
    set.endGroup();

    set.beginGroup("LogConfig");
    APPSetting::TmpTaskSpeedHourMax = set.value("TmpTaskSpeedHourMax").toInt();
    APPSetting::LowPowerTaskSpeedHourMax = set.value("LowPowerTaskSpeedHourMax").toInt();
    APPSetting::UseFileInterval = set.value("UseFileInterval").toBool();
    APPSetting::VIBSaveInterval = set.value("VIBSaveInterval").toInt();
    APPSetting::TMPSaveInterval = set.value("TMPSaveInterval").toInt();
    APPSetting::PackagingDate = set.value("PackagingDate").toString();
    set.endGroup();

    set.beginGroup("TaskIntervalConfig");
    APPSetting::TimeoutInterval_Pre = set.value("TimeoutInterval_Pre").toInt();
    APPSetting::TimeoutInterval_Slave = set.value("TimeoutInterval_Slave").toInt();
    APPSetting::TimeoutInterval_OtherBrade = set.value("TimeoutInterval_OtherBrade").toInt();
    APPSetting::TimeoutInterval_UDPUpdateFile = set.value("TimeoutInterval_UDPUpdateFile").toInt();
    set.endGroup();

    set.beginGroup("FTPConfig");
    APPSetting::FtpDirName = set.value("FtpDirName").toString();
    set.endGroup();

    set.beginGroup("LEDConfig");
    APPSetting::Led_Run = set.value("Led_Run").toString();
    APPSetting::Led_PreError = set.value("Led_PreError").toString();
    APPSetting::Led_alarm1 = set.value("Led_alarm1").toString();
    APPSetting::Led_alarm2 = set.value("Led_alarm2").toString();
    set.endGroup();

    set.beginGroup("IOPowerConfig");
    APPSetting::IOPowerEnable = set.value("IOPowerEnable").toBool();
    set.endGroup();

    set.beginGroup("TimerIntervalConfig");
    APPSetting::CheckCanInterval = set.value("CheckCanInterval").toInt();
    APPSetting::CheckSlaceInterval = set.value("CheckSlaceInterval").toInt();
    APPSetting::CheckOtherBoardInterval = set.value("CheckOtherBoardInterval").toInt();
    APPSetting::CheckOtherTaskInterval = set.value("CheckOtherTaskInterval").toInt();
    APPSetting::CheckPISStateInterval = set.value("CheckPISStateInterval").toInt();
    APPSetting::CanBusTaskInterval = set.value("CanBusTaskInterval").toInt();
    APPSetting::CanBusUpdateTaskInterval = set.value("CanBusUpdateTaskInterval").toInt();
    APPSetting::TcpSendFileInterval = set.value("TcpSendFileInterval").toInt();
    APPSetting::WatchDogInterval = set.value("WatchDogInterval").toInt();
    set.endGroup();
}

void APPSetting::WriteConfig()
{
    QSettings set(APPSetting::configFile,QSettings::IniFormat);

    set.beginGroup("softwareConfig");
    set.setValue("version",APPSetting::version);
    set.setValue("WagonType",APPSetting::WagonType);
    set.endGroup();

    set.beginGroup("CarConfig");
    set.setValue("projectName",APPSetting::projectName);
    set.setValue("LineNumber",APPSetting::LineNumber);
    set.setValue("CarNumber",APPSetting::CarNumber);
    set.setValue("WagonNumber",APPSetting::WagonNumber);
    set.setValue("WheelDiameter",APPSetting::WheelDiameter);
    set.setValue("SpeedCoefficient",APPSetting::SpeedCoefficient);
    set.endGroup();

    set.beginGroup("CanConfig");
    set.setValue("CanPlugin",APPSetting::CanPlugin);

    set.setValue("can0_Mode",APPSetting::can0_Mode);
    set.setValue("can0_Name",APPSetting::can0_Name);
    set.setValue("can0_bitrate",APPSetting::can0_bitrate);
    set.setValue("can0_bitrateSample",APPSetting::can0_bitrateSample);
    set.setValue("can0_dbitrate",APPSetting::can0_dbitrate);
    set.setValue("can0_dbitrateSample",APPSetting::can0_dbitrateSample);

    set.setValue("can1_Mode",APPSetting::can1_Mode);
    set.setValue("can1_Name",APPSetting::can1_Name);
    set.setValue("can1_bitrate",APPSetting::can1_bitrate);
    set.setValue("can1_bitrateSample",APPSetting::can1_bitrateSample);
    set.setValue("can1_dbitrate",APPSetting::can1_dbitrate);
    set.setValue("can1_dbitrateSample",APPSetting::can1_dbitrateSample);

    set.setValue("can2_Mode",APPSetting::can2_Mode);
    set.setValue("can2_Name",APPSetting::can2_Name);
    set.setValue("can2_bitrate",APPSetting::can2_bitrate);
    set.setValue("can2_bitrateSample",APPSetting::can2_bitrateSample);
    set.setValue("can2_dbitrate",APPSetting::can2_dbitrate);
    set.setValue("can2_dbitrateSample",APPSetting::can2_dbitrateSample);

    set.setValue("CANVibrateDataLen",APPSetting::CANVibrateDataLen);
    set.setValue("VibrateSamplingPoints",APPSetting::VibrateSamplingPoints);
    set.setValue("VibrateSamplingFrequency",APPSetting::VibrateSamplingFrequency);
    set.endGroup();

    set.beginGroup("LocalNetConfig");
    set.setValue("LocalIP_eth0",APPSetting::LocalIP_eth0);
    set.setValue("LocalMask_eth0",APPSetting::LocalMask_eth0);
    set.setValue("LocalGateway_eth0",APPSetting::LocalGateway_eth0);
    set.setValue("LocalDNS_eth0",APPSetting::LocalDNS_eth0);
    set.setValue("LocalIP_eth1",APPSetting::LocalIP_eth1);
    set.setValue("LocalMask_eth1",APPSetting::LocalMask_eth1);
    set.setValue("LocalGateway_eth1",APPSetting::LocalGateway_eth1);
    set.setValue("LocalDNS_eth1",APPSetting::LocalDNS_eth1);
    set.endGroup();

    set.beginGroup("TCPConfig");
    set.setValue("TcpServerIP",APPSetting::TcpServerIP);
    set.setValue("TcpServerPort",APPSetting::TcpServerPort);
    set.setValue("TcpListenPort",APPSetting::TcpListenPort);
#ifdef DEVICE_SLAVE
    set.setValue("TcpServerIPStandby",APPSetting::TcpServerIPStandby);
    set.setValue("TcpServerPortStandby",APPSetting::TcpServerPortStandby);
#endif
    set.endGroup();

    set.beginGroup("UDPConfig");
    set.setValue("UDPMulticastIP",APPSetting::UDPMulticastIP);
    set.setValue("UDPListenPort",APPSetting::UDPListenPort);
    set.setValue("HeartbeatInterval",APPSetting::HeartbeatInterval);
    set.setValue("SynchronousInterval",APPSetting::SynchronousInterval);
    set.endGroup();

    set.beginGroup("HMIConfig");
    set.setValue("HMIListenPort",APPSetting::HMIListenPort);
    set.endGroup();

    set.beginGroup("WarningConfig");
    set.setValue("baowaiWarning",APPSetting::baowaiWarning);
    set.setValue("baoneiWarning",APPSetting::baoneiWarning);
    set.setValue("waihuanWarning",APPSetting::waihuanWarning);
    set.setValue("neihuanWarning",APPSetting::neihuanWarning);
    set.setValue("gundanWarning",APPSetting::gundanWarning);
    set.setValue("gunshuangWarning",APPSetting::gunshuangWarning);
    set.setValue("treadWarning",APPSetting::treadWarning);
    set.setValue("benzhouchilunWarning",APPSetting::benzhouchilunWarning);
    set.setValue("linzhouchilunWarning",APPSetting::linzhouchilunWarning);
    set.endGroup();

    set.beginGroup("FirstLevelAlarmConfig");
    set.setValue("baowaiFirstLevelAlarm",APPSetting::baowaiFirstLevelAlarm);
    set.setValue("baoneiFirstLevelAlarm",APPSetting::baoneiFirstLevelAlarm);
    set.setValue("waihuanFirstLevelAlarm",APPSetting::waihuanFirstLevelAlarm);
    set.setValue("neihuanFirstLevelAlarm",APPSetting::neihuanFirstLevelAlarm);
    set.setValue("gundanFirstLevelAlarm",APPSetting::gundanFirstLevelAlarm);
    set.setValue("gunshuangFirstLevelAlarm",APPSetting::gunshuangFirstLevelAlarm);
    set.setValue("treadFirstLevelAlarm",APPSetting::treadFirstLevelAlarm);
    set.setValue("benzhouchilunFirstLevelAlarm",APPSetting::benzhouchilunFirstLevelAlarm);
    set.setValue("linzhouchilunFirstLevelAlarm",APPSetting::linzhouchilunFirstLevelAlarm);
    set.endGroup();

    set.beginGroup("SecondaryAlarmConfig");
    set.setValue("baowaiSecondaryAlarm",APPSetting::baowaiSecondaryAlarm);
    set.setValue("baoneiSecondaryAlarm",APPSetting::baoneiSecondaryAlarm);
    set.setValue("waihuanSecondaryAlarm",APPSetting::waihuanSecondaryAlarm);
    set.setValue("neihuanSecondaryAlarm",APPSetting::neihuanSecondaryAlarm);
    set.setValue("gundanSecondaryAlarm",APPSetting::gundanSecondaryAlarm);
    set.setValue("gunshuangSecondaryAlarm",APPSetting::gunshuangSecondaryAlarm);
    set.setValue("treadSecondaryAlarm",APPSetting::treadSecondaryAlarm);
    set.setValue("benzhouchilunSecondaryAlarm",APPSetting::benzhouchilunSecondaryAlarm);
    set.setValue("linzhouchilunSecondaryAlarm",APPSetting::linzhouchilunSecondaryAlarm);
    set.endGroup();

    set.beginGroup("OtherAlarmConfig");
    set.setValue("AbsoluteTemperatureThreshold",APPSetting::AbsoluteTemperatureThreshold);
    set.setValue("RelativeTemperatureThreshold",APPSetting::RelativeTemperatureThreshold);
    set.setValue("AbsoluteTemperatureWarning",APPSetting::AbsoluteTemperatureWarning);
    set.setValue("MemorySpaceRatio",APPSetting::MemorySpaceRatio);
    set.endGroup();

    set.beginGroup("DimensionalAlarmConfig");
    set.setValue("Axlebox_Rms_FirstLevelAlarm",APPSetting::Axlebox_Rms_FirstLevelAlarm);
    set.setValue("Gearbox_Rms_FirstLevelAlarm",APPSetting::Gearbox_Rms_FirstLevelAlarm);
    set.setValue("Motor_Rms_FirstLevelAlarm",APPSetting::Motor_Rms_FirstLevelAlarm);
    set.setValue("Axlebox_Rms_SecondaryAlarm",APPSetting::Axlebox_Rms_SecondaryAlarm);
    set.setValue("Gearbox_Rms_SecondaryAlarm",APPSetting::Gearbox_Rms_SecondaryAlarm);
    set.setValue("Motor_Rms_SecondaryAlarm",APPSetting::Motor_Rms_SecondaryAlarm);
    set.setValue("Axlebox_PP_FirstLevelAlarm",APPSetting::Axlebox_PP_FirstLevelAlarm);
    set.setValue("Gearbox_PP_FirstLevelAlarm",APPSetting::Gearbox_PP_FirstLevelAlarm);
    set.setValue("Motor_PP_FirstLevelAlarm",APPSetting::Motor_PP_FirstLevelAlarm);
    set.setValue("Axlebox_PP_SecondaryAlarm",APPSetting::Axlebox_PP_SecondaryAlarm);
    set.setValue("Gearbox_PP_SecondaryAlarm",APPSetting::Gearbox_PP_SecondaryAlarm);
    set.setValue("Motor_PP_SecondaryAlarm",APPSetting::Motor_PP_SecondaryAlarm);
    set.endGroup();

    set.beginGroup("AlarmSwitchConfig");
    set.setValue("UseDimensionalAlarm",APPSetting::UseDimensionalAlarm);
    set.setValue("UseVibrateWarning",APPSetting::UseVibrateWarning);
    set.setValue("UseVibrateFirstLevelAlarm",APPSetting::UseVibrateFirstLevelAlarm);
    set.setValue("UseVibrateSecondaryAlarm",APPSetting::UseVibrateSecondaryAlarm);
    set.setValue("UseTemperatureAlarm",APPSetting::UseTemperatureAlarm);
    set.setValue("UseOfflineAlarm",APPSetting::UseOfflineAlarm);
    set.setValue("UseExternalSpeed",APPSetting::UseExternalSpeed);
    set.endGroup();

    set.beginGroup("LogConfig");
    set.setValue("TmpTaskSpeedHourMax",APPSetting::TmpTaskSpeedHourMax);
    set.setValue("LowPowerTaskSpeedHourMax",APPSetting::LowPowerTaskSpeedHourMax);
    set.setValue("UseFileInterval",APPSetting::UseFileInterval);
    set.setValue("VIBSaveInterval",APPSetting::VIBSaveInterval);
    set.setValue("TMPSaveInterval",APPSetting::TMPSaveInterval);
    set.setValue("PackagingDate",APPSetting::PackagingDate);
    set.endGroup();

    set.beginGroup("TaskIntervalConfig");
    set.setValue("TimeoutInterval_Pre",APPSetting::TimeoutInterval_Pre);
    set.setValue("TimeoutInterval_Slave",APPSetting::TimeoutInterval_Slave);
    set.setValue("TimeoutInterval_OtherBrade",APPSetting::TimeoutInterval_OtherBrade);
    set.setValue("TimeoutInterval_UDPUpdateFile",APPSetting::TimeoutInterval_UDPUpdateFile);
    set.endGroup();

    set.beginGroup("FTPConfig");
    set.setValue("FtpDirName",APPSetting::FtpDirName);
    set.endGroup();

    set.beginGroup("LEDConfig");
    set.setValue("Led_Run",APPSetting::Led_Run);
    set.setValue("Led_PreError",APPSetting::Led_PreError);
    set.setValue("Led_alarm1",APPSetting::Led_alarm1);
    set.setValue("Led_alarm2",APPSetting::Led_alarm2);
    set.endGroup();

    set.beginGroup("IOPowerConfig");
    set.setValue("IOPowerEnable",APPSetting::IOPowerEnable);
    set.endGroup();

    set.beginGroup("TimerIntervalConfig");
    set.setValue("CheckCanInterval",APPSetting::CheckCanInterval);
    set.setValue("CheckSlaceInterval",APPSetting::CheckSlaceInterval);
    set.setValue("CheckOtherBoardInterval",APPSetting::CheckOtherBoardInterval);
    set.setValue("CheckOtherTaskInterval",APPSetting::CheckOtherTaskInterval);
    set.setValue("CheckPISStateInterval",APPSetting::CheckPISStateInterval);
    set.setValue("CanBusTaskInterval",APPSetting::CanBusTaskInterval);
    set.setValue("CanBusUpdateTaskInterval",APPSetting::CanBusUpdateTaskInterval);
    set.setValue("TcpSendFileInterval",APPSetting::TcpSendFileInterval);
    set.setValue("WatchDogInterval",APPSetting::WatchDogInterval);
    set.endGroup();
}

void APPSetting::newConfig()
{
//    qDebug()<<"create newConfig" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    //若由要初始化的信息在此初始化
    WriteConfig();
}

bool APPSetting::checkConfig()
{
    //如果配置文件大小为0,则以初始值继续运行,并生成配置文件
       QFile file(APPSetting::configFile);

       if (file.size() == 0) {
           newConfig();
           return false;
       }

       //如果配置文件不完整,则以初始值继续运行,并生成配置文件
       if (file.open(QFile::ReadOnly)) {
           bool ok = true;

           while (!file.atEnd()) {
               QString line = file.readLine();
               line = line.replace("\r", "");
               line = line.replace("\n", "");
               QStringList list = line.split("=");

               if (list.count() == 2) {
                   if (list.at(1) == "") {
//                       qDebug()<<"list.at(0) = "<< list.at(0);
                       ok = false;
                       break;
                   }
               }
           }

           if (!ok) {
               newConfig();
               return false;
           }
       } else {
           newConfig();
           return false;
       }

       return true;
}

void APPSetting::UpdateNetworkconfig()
{
    CoreHelper::SetNetWorkConfig(APPSetting::LocalIP_eth0,APPSetting::LocalGateway_eth0,APPSetting::LocalDNS_eth0,"eth0");
    CoreHelper::SetNetWorkConfig(APPSetting::LocalIP_eth1,APPSetting::LocalGateway_eth1,APPSetting::LocalDNS_eth1,"eth1");
}
