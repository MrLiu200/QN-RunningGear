#include "m_databaseapi.h"
#include "dbdata.h"
M_DataBaseAPI::M_DataBaseAPI(QObject *parent) : QObject(parent)
{

}

QStringList M_DataBaseAPI::getContent(const QString &tableName, const QString &columnNames, const QString &whereSql)
{
    QStringList content;
    QSqlQuery query;

    QString sql = QString("select %1 from %2 %3").arg(columnNames).arg(tableName).arg(whereSql);
    query.exec(sql);

    int columnCount = query.record().count();
    while (query.next()) {
        QString temp = "";
        for (int i = 0; i < columnCount; i++) {
            temp += query.value(i).toString() + ";";
        }

        content.append(temp.mid(0, temp.length() - 1));
    }

    return content;
}

bool M_DataBaseAPI::inputData(int columnCount,
                              const QString &filter,
                              const QString &tableName,
                              QString &fileName,
                              const QString &defaultDir)
{
    bool result = true;
    fileName = CoreHelper::getFileName(filter, defaultDir);

    if (fileName.length() != 0) {
        QFile file(fileName);

        //先读取第一行判断列数是否和目标列数一致,不一致则返回
        if (file.open(QIODevice::ReadOnly | QFile::Text)) {
            QString line = QString::fromUtf8(file.readLine());
            QStringList list = line.split("  ");
            if (list.count() != columnCount) {
                return false;
            }

            file.close();
        }

        //先删除原来的数据
        QString sql = QString("delete from %1").arg(tableName);
        QSqlQuery query;
        query.exec(sql);

        if (file.open(QIODevice::ReadOnly | QFile::Text)) {
            QSqlDatabase::database().transaction();

            while(!file.atEnd()) {
                QString line = QString::fromUtf8(file.readLine());
                QStringList list = line.split("  ");

                if (list.count() == columnCount) {
                    sql = QString("insert into %1 values('").arg(tableName);

                    for (int i = 0; i < columnCount - 1; i++) {
                        sql = sql + list.at(i).trimmed() + "','";
                    }

                    sql = sql + list.at(columnCount - 1).trimmed() + "')";

                    query.clear();
                    query.exec(sql);
                }
            }

            if (!QSqlDatabase::database().commit()) {
                QSqlDatabase::database().rollback();
                result = false;
            }
        }
    }

    return result;
}

bool M_DataBaseAPI::outputData(const QString &defaultName,
                               const QString &filter,
                               const QStringList &content,
                               QString &fileName,
                               const QString &defaultDir)
{
    bool result = true;
    QString name = QString("%1/%2").arg(defaultDir).arg(defaultName);
    fileName = CoreHelper::getSaveName(filter, name);
    outputData(fileName, content);
    return result;
}

bool M_DataBaseAPI::outputData(QString &fileName,
                               const QStringList &content)
{
    if (fileName.length() != 0) {
        int rowCount = content.count();
        if (rowCount == 0) {
            fileName.clear();
            return false;
        }

        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QFile::Text);
        QTextStream stream(&file);

        for (int i = 0; i < rowCount; i++) {
            QStringList list = content.at(i).split(";");
            QString str;
            foreach (QString value, list) {
                str += QString("%1  ").arg(value);
            }

            str = str.mid(0, str.length() - 2);
            stream << str << "\n";
        }
    }

    return true;
}

void M_DataBaseAPI::LoadBearingInfo()
{
    DBData::BearingInfo_Model.clear();
    DBData::BearingInfo_PitchDiameter.clear();
    DBData::BearingInfo_RollingDiameter.clear();
    DBData::BearingInfo_RollerNum.clear();
    DBData::BearingInfo_ContactAngle.clear();

    QString str = "select ModelName, PitchDiameter, RollingDiameter, RollerNum, ContactAngle from BearingInfo;";
    QSqlQuery query;
    query.exec(str);
    while(query.next()){
        DBData::BearingInfo_Model.append(query.value(0).toString());
        DBData::BearingInfo_PitchDiameter.append(query.value(1).toFloat());
        DBData::BearingInfo_RollingDiameter.append(query.value(2).toFloat());
        DBData::BearingInfo_RollerNum.append(query.value(3).toInt());
        DBData::BearingInfo_ContactAngle.append(query.value(4).toFloat());
    }
}

void M_DataBaseAPI::LoadTask()
{
    DBData::Task_Action.clear();
    DBData::Task_TriggerTime.clear();

    QSqlQuery query;
    QString TaskStr = "select Action, TriggerTime from TimedTasksInfo";
    query.exec(TaskStr);

    while(query.next()){
        DBData::Task_Action.append(query.value(0).toString());
        DBData::Task_TriggerTime.append(query.value(1).toString());
    }
}

void M_DataBaseAPI::LoadDeviceInfo()
{
    DBData::DeviceInfo_Count = 0;
    DBData::DeviceInfo_DeviceID.clear();
    DBData::DeviceInfo_DeviceCH.clear();
    DBData::DeviceInfo_DeviceName.clear();
    DBData::DeviceInfo_DeviceType.clear();

    DBData::DeviceInfo_bearing1Name.clear();
    DBData::DeviceInfo_bearing1_model.clear();

    DBData::DeviceInfo_bearing2Name.clear();
    DBData::DeviceInfo_bearing2_model.clear();

    DBData::DeviceInfo_bearing3Name.clear();
    DBData::DeviceInfo_bearing3_model.clear();

    DBData::DeviceInfo_bearing4Name.clear();
    DBData::DeviceInfo_bearing4_model.clear();

    DBData::DeviceInfo_capstanName.clear();
    DBData::DeviceInfo_capstanTeethNum.clear();
    DBData::DeviceInfo_DrivenwheelName.clear();
    DBData::DeviceInfo_DrivenwheelTeethNum.clear();
    DBData::DeviceInfo_IsEnable.clear();
    DBData::DeviceInfo_version.clear();

    QSqlQuery query;
    QString DeviceInfoStr = "select DeviceID, DeviceCH, DeviceName, DeviceType, DeviceSensitivity, ShaftDiameter, bearing1Name, bearing1_model,"
                            "bearing2Name, bearing2_model, bearing3Name, bearing3_model, bearing4Name, bearing4_model,"
                            "capstanName, capstanTeethNum, DrivenwheelName, DrivenwheelTeethNum, version, IsEnable from DeviceInfo";//where IsEnable = 1

    query.exec(DeviceInfoStr);//找出使能的通道

    while(query.next()){
        DBData::DeviceInfo_DeviceID.append(query.value(0).toInt());
        DBData::DeviceInfo_DeviceCH.append(query.value(1).toInt());
        DBData::DeviceInfo_DeviceName.append(query.value(2).toString());
        DBData::DeviceInfo_DeviceType.append(query.value(3).toString());
        DBData::DeviceInfo_Sensitivity.append(query.value(4).toFloat());
        DBData::DeviceInfo_ShaftDiameter.append(query.value(5).toFloat());

        DBData::DeviceInfo_bearing1Name.append(query.value(6).toString());
        DBData::DeviceInfo_bearing1_model.append(query.value(7).toString());
        DBData::DeviceInfo_bearing2Name.append(query.value(8).toString());
        DBData::DeviceInfo_bearing2_model.append(query.value(9).toString());
        DBData::DeviceInfo_bearing3Name.append(query.value(10).toString());
        DBData::DeviceInfo_bearing3_model.append(query.value(11).toString());
        DBData::DeviceInfo_bearing4Name.append(query.value(12).toString());
        DBData::DeviceInfo_bearing4_model.append(query.value(13).toString());

        DBData::DeviceInfo_capstanName.append(query.value(14).toString());
        DBData::DeviceInfo_capstanTeethNum.append(query.value(15).toInt());
        DBData::DeviceInfo_DrivenwheelName.append(query.value(16).toString());
        DBData::DeviceInfo_DrivenwheelTeethNum.append(query.value(17).toInt());

        DBData::DeviceInfo_version.append(query.value(18).toString());
        DBData::DeviceInfo_IsEnable.append(query.value(19).toBool());
        DBData::DeviceInfo_Count++;
    }
}

void M_DataBaseAPI::LoadLinkDeviceInfo()
{
    DBData::LinkDeviceInfo_IP.clear();
    DBData::LinkDeviceInfo_Type.clear();
    DBData::LinkDeviceInfo_Port.clear();
    DBData::LinkDeviceInfo_WagonNumber.clear();
    QString sql = "select Type, IP, Port, WagonNumber from LinkDeviceInfo";
    QSqlQuery query;
    query.exec(sql);
    while (query.next()) {
        DBData::LinkDeviceInfo_Type.append(query.value(0).toString());
        DBData::LinkDeviceInfo_IP.append(query.value(1).toString());
        DBData::LinkDeviceInfo_Port.append(query.value(2).toInt());
        DBData::LinkDeviceInfo_WagonNumber.append(query.value(3).toString());
    }
}

bool M_DataBaseAPI::saveDeviceInfoToFile(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "Failed to open file for writing!";
        return false;
    }

    QTextStream out(&file);

    //写入文件的列名（适用于 CSV 格式）
//    out << "DeviceID, DeviceCH, DeviceName, DeviceType, DeviceSensitivity, ShaftDiameter, "
//           "bearing1Name, bearing1_model, bearing2Name, bearing2_model, bearing3Name, bearing3_model, "
//           "bearing4Name, bearing4_model, capstanName, capstanTeethNum, DrivenwheelName, DrivenwheelTeethNum, "
//           "version, IsEnable" << Qt::endl;

    //先写入轴承信息
    int size = DBData::BearingInfo_Model.size();
    out << "BearingInfo" << Qt::endl;
    out << size << Qt::endl;
    for(int i=0;i<DBData::BearingInfo_Model.size();i++){
        out << DBData::BearingInfo_Model.at(i) << ","
            << DBData::BearingInfo_PitchDiameter.at(i) << ","
            << DBData::BearingInfo_RollingDiameter.at(i) << ","
            << DBData::BearingInfo_RollerNum.at(i) << ","
            << DBData::BearingInfo_ContactAngle.at(i)
            << Qt::endl;
    }

    int count = DBData::DeviceInfo_Count;
    out << "PreInfo" << Qt::endl;
    out << count << Qt::endl;
    // 遍历 DBData 并写入每一行

    for (int i = 0; i < count; ++i) {
        out << DBData::DeviceInfo_DeviceID.at(i) << ","
            << DBData::DeviceInfo_DeviceCH.at(i) << ","
            << DBData::DeviceInfo_DeviceName.at(i) << ","
            << DBData::DeviceInfo_DeviceType.at(i) << ","
            << DBData::DeviceInfo_Sensitivity.at(i) << ","
            << DBData::DeviceInfo_ShaftDiameter.at(i) << ","
            << DBData::DeviceInfo_bearing1Name.at(i) << ","
            << DBData::DeviceInfo_bearing1_model.at(i) << ","
            << DBData::DeviceInfo_bearing2Name.at(i) << ","
            << DBData::DeviceInfo_bearing2_model.at(i) << ","
            << DBData::DeviceInfo_bearing3Name.at(i) << ","
            << DBData::DeviceInfo_bearing3_model.at(i) << ","
            << DBData::DeviceInfo_bearing4Name.at(i) << ","
            << DBData::DeviceInfo_bearing4_model.at(i) << ","
            << DBData::DeviceInfo_capstanName.at(i) << ","
            << DBData::DeviceInfo_capstanTeethNum.at(i) << ","
            << DBData::DeviceInfo_DrivenwheelName.at(i) << ","
            << DBData::DeviceInfo_DrivenwheelTeethNum.at(i) << ","
            << DBData::DeviceInfo_version.at(i) << ","
            << DBData::DeviceInfo_IsEnable.at(i)
            << Qt::endl;
    }

    // 关闭文件
    file.flush();
    file.close();
    return true;
}

void M_DataBaseAPI::addLog(const QString &carNumber, const QString &WagonNumber, const int &DeviceID, const int &DeviceCh, const QString &DeviceName, const QString &LogType, const int &AlarmGrade, const QString &TriggerTime, const QString &LogContent)
{
    QString sql = "insert into LogInfo(CarNumber,WagonNumber,DeviceID,DeviceCH,DeviceName,LogType,AlarmGrade,TriggerTime,LogContent) values('";
    sql += carNumber + "','";
    sql += WagonNumber +"','";
    sql += QString::number(DeviceID) + "','";
    sql += QString::number(DeviceCh) + "','";
    sql += DeviceName + "','";
    sql += LogType + "','";
    sql += QString::number(AlarmGrade) + "','";
    sql += TriggerTime + "','";
    sql += LogContent + "')";
//    qDebug()<< sql;
    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::addLog(const QString &carNumber, const QString &WagonNumber, const QString &LogType, const QString &TriggerTime, const QString &LogContent)
{
    addLog(carNumber,WagonNumber,0,0,"000",LogType,-1,TriggerTime,LogContent);
}

void M_DataBaseAPI::AddDimensionalValue(QString Wagon, qint8 id, qint8 ch, uint32_t speed, double Ambienttem, double pointtem, QString time, QVector<float> Value)
{
    QString sql = "insert into DimensionalValue values('";
    sql += Wagon + "','";
    sql += QString::number(id) + "','";
    sql += QString::number(ch) + "','";
    sql += QString::number(speed) + "','";
    sql += QString::number(Ambienttem) + "','";
    sql += QString::number(pointtem) + "','";
    sql += time + "','";
    for(int i=0;i<Value.size()-1;i++){
        sql += QString::number(Value.at(i)) + "','";
    }
    sql += QString::number(Value.at(Value.size()-1)) + "')";


    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::AddDemodulatedValue(QString Wagon, qint8 id, qint8 ch, uint32_t speed, double Ambienttem, double pointtem, QString time, QVector<float> Value)
{
    QString sql = "insert into DemodulatedValue values('";
    sql += Wagon + "','";
    sql += QString::number(id) + "','";
    sql += QString::number(ch) + "','";
    sql += QString::number(speed) + "','";
    sql += QString::number(Ambienttem) + "','";
    sql += QString::number(pointtem) + "','";
    sql += time + "','";
    for(int i=0;i<Value.size()-1;i++){
        sql += QString::number(Value.at(i)) + "','";
    }
    sql += QString::number(Value.at(Value.size()-1)) + "')";


    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::DeleteDimensionalValue(QString timeStart, QString timeEnd)
{
    if((!timeStart.isEmpty()) && (!timeEnd.isEmpty())){
        QString sql = "delete from DimensionalValue where datetime(TriggerTime)>='";
        sql += timeStart;
        sql += "' and datetime(TriggerTime)<='";
        sql += timeEnd + "'";
        QSqlQuery query;
        query.exec(sql);
        qDebug()<<"[" << DATETIME << "]"<<"    "<<sql;
    }
}

QString M_DataBaseAPI::GetDimensionalCSV(QString timeStart, QString timeEnd)
{
    QString ret = "";
    if((!timeStart.isEmpty()) && (!timeEnd.isEmpty())){
        QDateTime start = QDateTime::fromString(timeStart,"yyyy-MM-dd HH:mm:ss");
        QDateTime end = QDateTime::fromString(timeEnd,"yyyy-MM-dd HH:mm:ss");
        if(start.secsTo(end) > 0){
            QString sql = "select * from DimensionalValue where datetime(TriggerTime)>='";
            sql += timeStart;
            sql += "' and datetime(TriggerTime)<='";
            sql += timeEnd + "'";
            QSqlQuery query;
            query.exec(sql);
            QString filename = QString("%1_Dimensional_%2.csv").arg(CoreHelper::APPPath()).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"));
            QFile file(filename);
            if(file.open(QFile::WriteOnly|QFile::Text)){
                return ret;
            }
            QTextStream textstarem(&file);
            while (query.next()) {
                for(int i=0;i<query.record().count();i++){
                    textstarem << query.value(i).toString() << "\t";
                }
                textstarem << "\n";
            }
            file.flush();
            file.close();
            ret = filename;
        }
    }
    return ret;
}

void M_DataBaseAPI::AddDeviceInfo(int DeviceId, int DeviceCH, QString DeviceName, QString DeviceType, float DeviceSensitivity, float ShaftDiameter, QString Bearing1Typedef,
                                  QString Bearing1_model, QString Bearing2Typedef, QString Bearing2_model, QString Bearing3Typedef, QString Bearing3_model,
                                  QString Bearing4Typedef, QString Bearing4_model, QString capstanName, int capstanTeethNum, QString DrivenwheelName, int DrivenwheelTeethNum,
                                  QString version, bool Enable)
{
    QString AddSql = "insert into DeviceInfo(DeviceID, DeviceCH, DeviceName, DeviceType, DeviceSensitivity, ShaftDiameter, bearing1Name, bearing1_model,"
                     "bearing2Name, bearing2_model, bearing3Name, bearing3_model, bearing4Name, bearing4_model,"
                     "capstanName, capstanTeethNum, DrivenwheelName, DrivenwheelTeethNum, version, IsEnable) values('";
    AddSql += QString::number(DeviceId) + "','";
    AddSql += QString::number(DeviceCH) + "','";
    AddSql += DeviceName + "','";
    AddSql += DeviceType + "','";
    AddSql += QString::number(DeviceSensitivity) + "','";
    AddSql += QString::number(ShaftDiameter) + "','";
    AddSql += Bearing1Typedef + "','";
    AddSql += Bearing1_model + "','";
    AddSql += Bearing2Typedef + "','";
    AddSql += Bearing2_model + "','";
    AddSql += Bearing3Typedef + "','";
    AddSql += Bearing3_model + "','";
    AddSql += Bearing4Typedef + "','";
    AddSql += Bearing4_model + "','";
    AddSql += capstanName + "','";
    AddSql += QString::number(capstanTeethNum) + "','";
    AddSql += DrivenwheelName + "','";
    AddSql += QString::number(DrivenwheelTeethNum) + "','";
    AddSql += version + "','";
    AddSql += QString::number(Enable) + "');";
//    qDebug()<<AddSql;

    QSqlQuery query;
    query.exec(AddSql);
}

void M_DataBaseAPI::DeleteDeviceInfo(int DeviceId, int DeviceCH)
{
    QString sql = QString("delete from DeviceInfo where DeviceID = %1 and DeviceCH = %2;").arg(DeviceId).arg(DeviceCH);
    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::AddBearingInfo(QString modelname, float PitchDiameter, float RollingDiameter, int RollerNum, float ContactAngle)
{
    QString str = "insert into BearingInfo(ModelName, PitchDiameter, RollingDiameter, RollerNum, ContactAngle) values('";
    str += modelname + "','";
    str += QString::number(PitchDiameter) + "','";
    str += QString::number(RollingDiameter) + "','";
    str += QString::number(RollerNum) + "','";
    str += QString::number(ContactAngle) + "');";

    QSqlQuery query;
    query.exec(str);
}

void M_DataBaseAPI::DeleteBearingInfo(QString BearingName)
{
    QString sql = QString("delete from BearingInfo where ModelName = %1;").arg(BearingName);
    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::AddTaskInfo(QString Action, QString TriggerTime)
{
    QString sql = "insert into TimedTasksInfo(Action, TriggerTime) values('";
    sql += Action + "','";
    sql += TriggerTime + "')";

    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::DeleteTasks(QString action)
{
    QString sql = "delete from TimedTasksInfo where Action = '";
    sql += action + "'";

    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::AddLinkDeviceInfo(QString Type, QString IP, int Port, QString WagonNumber)
{
    QString sql = "insert into LinkDeviceInfo(Type,IP,Port,WagonNumber) values('";
    sql += Type + "','";
    sql += IP + "','";
    sql += QString::number(Port) + "','";
    sql += WagonNumber +"')";

    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::DeleteLinkDeviceInfo(QString IP)
{
    QString sql = "delete from LinkDeviceInfo where IP = '";
    sql += IP + "'";

    QSqlQuery query;
    query.exec(sql);
}

void M_DataBaseAPI::ClearTable(QString tabname)
{
    if(tabname.isEmpty()) return;
    QString selectstr = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tabname);
    QSqlQuery query;
    if(query.exec(selectstr) && query.next()){
        qDebug()<<"clear table " << tabname;
        QString sql = QString("delete from %1").arg(tabname);
        query.exec(sql);
    }

}

void M_DataBaseAPI::UpdateVersion(quint8 id, quint8 ch, QString newversion)
{
    if(id < 1 || ch < 1 || newversion.isEmpty()) return;

    QString updateSql = "UPDATE DeviceInfo SET version = :newversion WHERE DeviceID = :id and DeviceCH = :ch";
    QSqlQuery query;
    query.prepare(updateSql);
    query.bindValue(":newversion",newversion);
    query.bindValue(":id",id);
    query.bindValue(":ch",ch);

    query.exec();
}
