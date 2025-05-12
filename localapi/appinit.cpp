#include "appinit.h"
#include "corehelper.h"
#include "datalog.h"
#include "appsetting.h"
#include "busserverapi.h"
#include "datalog.h"
#include "dbdata.h"
#include "tcpreceivefilethread.h"
#include "tcpsendfilethread.h"
#include "bus2api.h"
#include "uppercomputer.h"
#include "udpmulticastapi.h"
#include "appdog.h"
#include "udpslotapi.h"
#include "m_packaging.h"
APPInit *APPInit::self = 0;
APPInit *APPInit::Instance()
{
    if(!self){
        QMutex mutex;
        QMutexLocker locker(&mutex);
        if(!self){
            self = new APPInit;
        }
    }
    return self;
}

APPInit::APPInit(QObject *parent) : QObject(parent)
{

}

void APPInit::start()
{
    //注册 QVector<float> 使其能在信号槽传递
    qRegisterMetaType<QVector<float>>("QVector<float>");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint32_t>("uint32_t");

    /*******加载配置文件 start*******/
    APPSetting::configFile = QString("%1/config/%2.ini").arg(CoreHelper::APPPath()).arg(CoreHelper::APPName());
    APPSetting::ReadConfig();
    RefreshAlgorithmLimit();
    /*******加载配置文件 end*******/
    //检查网络配置
    NetCheck();

    /*******安装日志钩子 start*******/
//    DataLog::Instance()->InstallHook();
    /*******安装日志钩子 end*******/
    qDebug()<<"*******************Software Initialization Start*****************";
    qDebug()<<"*******************Software Initialization Start*****************";
    qDebug()<<"*******************Software Initialization Start*****************";
    qDebug()<<"*******************Software Initialization Start*****************";

#ifndef DEVICE_SLAVE
    /*******初始化外部存储设备 start*******/
    //主机才会挂载外部硬盘
    APPSetting::IsHardDiskReady = CoreHelper::ExternalStorageInit();
#endif
//    qDebug()<<"APPSetting::IsHardDiskReady = " << APPSetting::IsHardDiskReady;
    /*******初始化外部存储设备 end*******/



    /*******初始化数据库 start*******/
    //数据库是否要放在硬盘内，这是个思考的问题
    APPSetting::dbfile = CoreHelper::APPPath() + "/RunningGear.db";
    QFile dbFile(APPSetting::dbfile);
    if (!dbFile.exists() || dbFile.size() == 0) {
        qDebug()<<"get new sql file";
        dbFile.remove();
        CoreHelper::copyFile(":/db/RunningGear.db", APPSetting::dbfile);
    }

    //程序启动后打开数据库连接,在任何窗体需要的地方调用数据库
    QSqlDatabase dbConn;
    dbConn = QSqlDatabase::addDatabase("QSQLITE");
    dbConn.setDatabaseName(APPSetting::dbfile);
    bool ok = dbConn.open();

    if(ok){
        M_DataBaseAPI::LoadDeviceInfo();
        M_DataBaseAPI::LoadBearingInfo();
        M_DataBaseAPI::LoadLinkDeviceInfo();
        M_DataBaseAPI::LoadTask();
        //新增发送配置文件至主机
        QString configname = CoreHelper::APPPath() + "/config/config_" + APPSetting::projectName + "_" + APPSetting::CarNumber + "_" + APPSetting::WagonNumber + ".csv";
//        QString configname = CoreHelper::APPPath() + "/config/config_"  + APPSetting::CarNumber + "_" + APPSetting::WagonNumber + ".csv";
        if(M_DataBaseAPI::saveDeviceInfoToFile(configname)){
            DBData::TcpSendList.append(configname);
            qDebug()<<"create new config csv success";
        }else{
            qDebug()<<"create new config csv fail";
        }
    }else{
        qDebug()<<"open sql fail";
    }
    /*******初始化数据库 end*******/

    /*******初始化日志 start*******/
    if(DataLog::Instance()->ParameterInit()){
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"日志初始化成功");
    }else{
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"日志初始化失败");
    }
    /*******初始化日志 end*******/

    /*******日志压缩 start***********/
#ifndef DEVICE_SLAVE
    //获取当前日期，与要打包的日期做对比
    QDate olddate = QDate::fromString(APPSetting::PackagingDate, "yyyy-MM-dd");
    if(olddate.isValid() && APPSetting::IsHardDiskReady){//判断日期有效
        QDate today = QDate::currentDate();
        if(olddate < today){
            qDebug()<<"开启日志打包线程";
            //获取日志文件夹
            //构建打包文件名称
            QString SourceDir = DataLog::Instance()->GetRunDataPath();
            QString TargetDir = DataLog::Instance()->GetPreviousPath();
            QString TargetFile = QString("%1/%2_data.zip").arg(TargetDir).arg(APPSetting::PackagingDate);
            QString packagingdate = APPSetting::PackagingDate;
            M_Packaging *m_packaging = new M_Packaging(SourceDir,TargetFile,packagingdate);
            connect(m_packaging,&M_Packaging::PackagingError,[](QString ErrorStr){
                M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,ErrorStr);
            });
            connect(m_packaging,&M_Packaging::PackagingFinish,[m_packaging](QString Tfilename){
                QMetaObject::invokeMethod(m_packaging, [m_packaging, Tfilename](){
                    APPSetting::PackagingDate = QDate::currentDate().toString("yyyy-MM-dd");
                    APPSetting::WriteConfig();
                    M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,Tfilename + "压缩成功");
                    m_packaging->quit();
                    m_packaging->wait();
                    m_packaging->deleteLater();
                });

                //更新打包日期并添加日志


            });
            m_packaging->start();
        }
    }
#endif
    /*******日志压缩 end***********/
    /*******初始化指示灯面板 start*******/
    CoreHelper::SetLedState(APPSetting::Led_PreError,1);
    CoreHelper::SetLedState(APPSetting::Led_alarm1,0);
    CoreHelper::SetLedState(APPSetting::Led_alarm2,0);
    /*******初始化指示灯面板 end*******/
    /*******初始化看门狗 start*******/
//    AppDog::Instance()->start();
    /*******初始化看门狗 end*******/

    /*******UDP初始化 start*******/

    if(UDPMulticastAPI::Instance()->startJoin()){
//        UDPMulticastAPI::Instance()->start();
        qDebug()<<"UDP 初始化成功";
        UDPSlotAPI::Instance()->Init();
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"UDP 初始化成功");
    }else{
        qDebug()<<"UDP 初始化失败";
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"UDP 初始化失败");
    }
    /*******UDP初始化 end*******/

    /*******TCP初始化 start*******/
#ifndef DEVICE_SLAVE
    if(APPSetting::IsHardDiskReady){
        if(TcpReceiveFileThread::Instance()->startListen()){
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"Tcp 监听成功");
        }else{
            M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"Tcp 监听失败");
        }
    }
#endif
    TcpSendFileThread *tcpsendfilethread = new TcpSendFileThread;
    tcpsendfilethread->start();

    /*******TCP初始化 end*******/


#ifndef DEVICE_SLAVE
    /*******上位机TCP初始化 start*******/
    UpperComputer::Instance()->StartListen();
    /*******上位机TCP初始化 end*******/
#endif


    /*******can总线初始化 start*******/
    BusServerAPI::Instance()->Init();
    if(Bus2API::Instance()->Open()){
        if(!APPSetting::IOPowerEnable){//检查前置IO板卡电源是否已经供电
            Bus2API::Instance()->SetPreIOPowerState(true);
        }
        Bus2API::Instance()->GetSelfInspection(0xFF);
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"can2 初始化成功");
    }else{
        M_DataBaseAPI::addLog(APPSetting::CarNumber,APPSetting::WagonNumber,"运行日志",DATETIME,"can2 初始化失败");
    }
    /*******can总线初始化 end*******/

    /*******定时任务初始化 start*******/
    TaskAPI::Instance()->Start();
    /*******定时任务初始化 end*******/
    qDebug()<<"*******************Software Initialization End*****************";

}

void APPInit::RefreshAlgorithmLimit()
{
//    DBData::ZhouLimit_Warning.append(APPSetting::baowaiWarning);
//    DBData::ZhouLimit_Warning.append(APPSetting::baoneiWarning);
//    DBData::ZhouLimit_Warning.append(APPSetting::waihuanWarning);
//    DBData::ZhouLimit_Warning.append(APPSetting::neihuanWarning);
//    DBData::ZhouLimit_Warning.append(APPSetting::gundanWarning);

    DBData::ZhouLimit_First.append(APPSetting::baowaiFirstLevelAlarm);
    DBData::ZhouLimit_First.append(APPSetting::baoneiFirstLevelAlarm);
    DBData::ZhouLimit_First.append(APPSetting::waihuanFirstLevelAlarm);
    DBData::ZhouLimit_First.append(APPSetting::neihuanFirstLevelAlarm);
    DBData::ZhouLimit_First.append(APPSetting::gundanFirstLevelAlarm);

    DBData::ZhouLimit_Secondary.append(APPSetting::baowaiSecondaryAlarm);
    DBData::ZhouLimit_Secondary.append(APPSetting::baoneiSecondaryAlarm);
    DBData::ZhouLimit_Secondary.append(APPSetting::waihuanSecondaryAlarm);
    DBData::ZhouLimit_Secondary.append(APPSetting::neihuanSecondaryAlarm);
    DBData::ZhouLimit_Secondary.append(APPSetting::gundanSecondaryAlarm);

//    DBData::OtherLimit_Warning.append(APPSetting::treadWarning);
//    DBData::OtherLimit_Warning.append(APPSetting::benzhouchilunWarning);
//    DBData::OtherLimit_Warning.append(APPSetting::linzhouchilunWarning);

    DBData::OtherLimit_First.append(APPSetting::treadFirstLevelAlarm);
    DBData::OtherLimit_First.append(APPSetting::benzhouchilunFirstLevelAlarm);
    DBData::OtherLimit_First.append(APPSetting::linzhouchilunFirstLevelAlarm);

    DBData::OtherLimit_Secondary.append(APPSetting::treadSecondaryAlarm);
    DBData::OtherLimit_Secondary.append(APPSetting::benzhouchilunSecondaryAlarm);
    DBData::OtherLimit_Secondary.append(APPSetting::linzhouchilunSecondaryAlarm);
}

void APPInit::NetCheck()
{
    QNetworkInterface iface = QNetworkInterface::interfaceFromName("eth0"); // 网卡名称
    QString eth0IP,eth1IP;
    bool ShouldRestart = false;
    if (iface.isValid()) {
        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                eth0IP = entry.ip().toString();
                break;
            }
        }
    }

    QNetworkInterface iface1 = QNetworkInterface::interfaceFromName("eth1"); // 网卡名称
    if (iface1.isValid()) {
        QList<QNetworkAddressEntry> entries1 = iface1.addressEntries();
        for (const QNetworkAddressEntry &entry1 : entries1) {
            if (entry1.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                eth1IP = entry1.ip().toString();
                break;
            }
        }
    }

    //重启网卡 systemctl restart systemd-networkd
    if(eth0IP.isEmpty() || (eth0IP != APPSetting::LocalIP_eth0)){
        ShouldRestart = true;
        CoreHelper::SetNetWorkConfig(APPSetting::LocalIP_eth0,APPSetting::LocalGateway_eth0,APPSetting::LocalDNS_eth0,"eth0");
    }
    if(eth1IP.isEmpty() || (eth1IP != APPSetting::LocalIP_eth1)){
        ShouldRestart = true;
        CoreHelper::SetNetWorkConfig(APPSetting::LocalIP_eth1,APPSetting::LocalGateway_eth1,APPSetting::LocalDNS_eth1,"eth1");
    }

    if(ShouldRestart){
        system("systemctl restart systemd-networkd");
    }
    QThread::sleep(3);
}
