#include "algorithm_v2.h"
#include "appsetting.h"
static struct ALGPRITHM_PARAMETERS Parameters;
static struct FAULTTYPE FaultType;
static struct TWOTYPEARGDOUBLE Resolving;
static struct TWOTYPEARGINT FFT_RESULT_LEN;
static TWOTYPEARGDOUBLE Average;
static struct PEAK m_ArrMaxPoint[FaultTypeNum][Rank_5];
static struct PEAK m_ArrMaxPoint_en[FaultTypeNum][Rank_5];
static struct PEAK m_ArrMaxPoint_zhou[FaultTypeNum][Rank_5];
static struct PEAK m_ArrMaxPoint_other[FaultType_OtherNum][Rank_5];

bool FaultResult[FaultTypeNum];
bool FaultResult_zhou[FaultTypeNum];//(保外、保内、外环、内环、滚单) *4
bool FaultResult_other[FaultType_OtherNum];//踏面、齿轮、大齿轮、小齿轮
static float m_dblArrMapBaseFre[FaultTypeNum];
static float m_dblArrMapBaseFre_other[FaultType_OtherNum];
static float dblRangeArr[FaultTypeNum][Rank_5];//包络数据DB
static float dblRangeArr_tz[FaultTypeNum][Rank_5];
static float dblRangeArr_zhou[FaultTypeNum][Rank_5];//频谱数据DB
static float dblRangeArr_zhou_tz[FaultTypeNum][Rank_5];
static float dblRangeArr_Ta[FaultTypeNum][Rank_5];
static bool m[2][3][21];
static float dbrange[2][3];
Algorithm_v2::Algorithm_v2(QObject *parent) : QObject(parent)
{

}

void Algorithm_v2::Argument_Init(float dblAngle1, float dblMidDia1, float dblRollerDia1, int nRollerAmount1, float dblAngle2,
                                 float dblMidDia2, float dblRollerDia2, int nRollerAmount2, float dblAngle3, float dblMidDia3,
                                 float dblRollerDia3, int nRollerAmount3, float dblAngle4, float dblMidDia4, float dblRollerDia4,
                                 int nRollerAmount4, float HighShaftDiameter, float LowShaftDiameter, float DspeedRatio, int gear_big,
                                 int gear_small, char pointType)
{
    std::fill(&dbrange[0][0], &dbrange[0][0] + 2 * 3 , 0);
    std::fill(&m[0][0][0], &m[0][0][0] + 2 * 3 * 21, true);
    std::fill(&dblRangeArr[0][0], &dblRangeArr[0][0] + FaultTypeNum * Rank_5 , 0);
    std::fill(&dblRangeArr_tz[0][0], &dblRangeArr_tz[0][0] + FaultTypeNum * Rank_5 , 0);
    std::fill(&dblRangeArr_zhou[0][0], &dblRangeArr_zhou[0][0] + FaultTypeNum * Rank_5 , 0);
    std::fill(&dblRangeArr_zhou_tz[0][0], &dblRangeArr_zhou_tz[0][0] + FaultTypeNum * Rank_5 , 0);
    std::fill(&dblRangeArr_Ta[0][0], &dblRangeArr_Ta[0][0] + FaultTypeNum * Rank_5 , 0);

    Parameters.bearing1.Angle = dblAngle1 * M_PI /180;
    Parameters.bearing1.MidDia = dblMidDia1;
    Parameters.bearing1.RollerDia = dblRollerDia1;
    Parameters.bearing1.RollerAmount = nRollerAmount1;
//    qDebug()<<"bearing1.Angle = " <<Parameters.bearing1.Angle;
//    qDebug()<<"bearing1.MidDia = " <<Parameters.bearing1.MidDia;
//    qDebug()<<"bearing1.RollerDia = " <<Parameters.bearing1.RollerDia;
//    qDebug()<<"bearing1.RollerAmount = " <<Parameters.bearing1.RollerAmount;

    Parameters.bearing2.Angle = dblAngle2 * M_PI /180;
    Parameters.bearing2.MidDia = dblMidDia2;
    Parameters.bearing2.RollerDia = dblRollerDia2;
    Parameters.bearing2.RollerAmount = nRollerAmount2;
//    qDebug()<<"bearing2.Angle = " <<Parameters.bearing2.Angle;
//    qDebug()<<"bearing2.MidDia = " <<Parameters.bearing2.MidDia;
//    qDebug()<<"bearing2.RollerDia = " <<Parameters.bearing2.RollerDia;
//    qDebug()<<"bearing2.RollerAmount = " <<Parameters.bearing2.RollerAmount;

    Parameters.bearing3.Angle = dblAngle3 * M_PI /180;
    Parameters.bearing3.MidDia = dblMidDia3;
    Parameters.bearing3.RollerDia = dblRollerDia3;
    Parameters.bearing3.RollerAmount = nRollerAmount3;

    Parameters.bearing4.Angle = dblAngle4 * M_PI /180;
    Parameters.bearing4.MidDia = dblMidDia4;
    Parameters.bearing4.RollerDia = dblRollerDia4;
    Parameters.bearing4.RollerAmount = nRollerAmount4;

    Parameters.ShaftDiameter.HighDiameter = HighShaftDiameter;
    Parameters.ShaftDiameter.LowDiameter = LowShaftDiameter;

    Parameters.speed_struct.Rev = DspeedRatio;
    Parameters.gear_number.BullGear = gear_big;
    Parameters.gear_number.Pinion = gear_small;
    Parameters.PointType = pointType;

    Parameters.speed_struct.SpeedRatio = (float)gear_big/gear_small;
//    qDebug()<<"DspeedRatio == " << DspeedRatio;
}

QStringList Algorithm_v2::Algoruthm_Start(QVector<float> OriginalData, QVector<float> DemodulationData, int OriginalLen,
                                          int DemodulationLen, int SamplingFrequency, float OriginalMax, float OriginalAVE,
                                          float DemodulationAVE)
{
    QStringList list,ret;

    if(OriginalMax < RANGE_JUDGE_LIMIT || (!APPSetting::UseVibrateFirstLevelAlarm && !APPSetting::UseVibrateSecondaryAlarm && !APPSetting::UseVibrateWarning)) {
        ret.append("255|255|255|255|255|255|255|255");
        return ret;
    }
    float O_time = (float)OriginalLen/SamplingFrequency;
    float D_time = (float)DemodulationLen/SamplingFrequency;

    Resolving.Original = (float)1/O_time;
    Resolving.Demodulation = (float)1/D_time;
    FFT_RESULT_LEN.Original = OriginalLen/2;
    FFT_RESULT_LEN.Demodulation = DemodulationLen/2;

    //数据处理，hanning后幅值减半，需*2
//    for(int i=1;i<OriginalLen;i++){
//        OriginalData[i] *= 2;
//    }
//    for(int i=1;i<DemodulationLen;i++){
//        DemodulationData[i] *= 2;
//    }
    //去直流分量
    OriginalData[0] = 0;
    DemodulationData[0] = 0;

    Average.Original = OriginalAVE;
    Average.Demodulation = DemodulationAVE;

    //需要3种判断方式：1：通过包络判断；2：通过频谱故障频率判断；3：通过频谱的故障频率边频判断
    //1、对所有故障判断计算
    InitMavinSys(Parameters.PointType,Parameters.speed_struct.Rev);
    ComputeFaultFre();
    //2、对轴承故障使用包络输入
    SearchMaxPoint_en(DemodulationData);
    Parameters.speed_struct.Lnum = round(Parameters.speed_struct.Lspeed)/Resolving.Demodulation;//低速轴转速谱线号
    Parameters.speed_struct.Hnum = round(Parameters.speed_struct.Hspeed)/Resolving.Demodulation;//高速轴转速谱线号

//    SideFrenJudge_en(DemodulationData);
    MavinAlert_en(Average.Demodulation,Parameters.speed_struct.Rev,Parameters.ShaftDiameter.LowDiameter);

    //包络检测出轴承故障才能进频谱搜索轴承故障
    //通过频谱检测中后期轴承故障
    bool serchzhou = false;
    memset(FaultResult_zhou,false,sizeof(FaultResult_zhou));
    for(int i=0;i<FaultTypeNum;i++){
        if(FaultResult[i]){
            FaultResult_zhou[i] = FaultResult[i];
            serchzhou = true;
        }
    }
    if(serchzhou){
        SearchMaxPoint_zhou(OriginalData,Average.Original);
        MavinAlert_zhou(Average.Original,Parameters.speed_struct.Rev,Parameters.ShaftDiameter.LowDiameter);
    }

    //3、对频谱故障使用正常频谱输入
    if(FaultResult_other[FaultType.FaultOther.TaMian]){
        SearchMaxPoint_ta(OriginalData,Average.Original);
        MavinAlert_ta(Average.Original);
    }
    //4.对齿轮啮合故障使用正常频谱输入
    bool other_Secondary[2] = {false,false};//大齿轮
    if(FaultResult_other[FaultType.FaultOther.ChiLun]){
        SearchMaxPoint(OriginalData,Average.Original);
        SideFrenJudge(OriginalData,Average.Original);
        int ntypecount[2],ntcount = 0;
        memset(ntypecount,0,sizeof(ntypecount));
        for(int ntype=0;ntype<2;ntype++){
            if (!FaultResult_other[ntype+2]) continue;
                ntcount=0;
            for(int nRank=0;nRank<Rank_3;nRank++){
                for(int i=0;i<21;i++){
                    if(i!=10){
                        if(m[ntype][nRank][i] == 1){
                            ntcount += 1;
                        }
                    }
                }
            }
            ntypecount[ntype] = ntcount;
//            qDebug()<< "ntype =  " << ntype  << "ntcount = " << ntypecount[ntype];
#if 1
            bool saveok = false;
            if(((ntype+2) == FaultType.FaultOther.DaChiLun) && (ntypecount[ntype] >= 50)){
//                qDebug()<< "DaChiLun : "<< ntypecount[ntype];
                saveok = true;
            }else if(((ntype+2) == FaultType.FaultOther.XiaoChiLun) && (ntypecount[ntype] >= 28)){
//                qDebug()<< "XiaoChiLun : "<< ntypecount[ntype];
                saveok = true;
            }
            if(saveok){
                //定义一个数组给大小齿轮二级故障
                other_Secondary[ntype] = true;
            }
#endif
        }
//        if((!FaultResult_other[FaultType.FaultOther.DaChiLun]) && (!FaultResult_other[FaultType.FaultOther.XiaoChiLun])){
//            FaultResult_other[FaultType.FaultOther.ChiLun] = false;
//        }
    }
    //算法完成之后，需要进行整合并返回给调用方
    //此处用什么方式呢？QString还是QVertor<bool> 总共42个，并且我需要纪录下来是哪个轴承报警，然后去外部对应哪个轴承名字
    /*1. 返回 保外、保内、外环、内环、滚单、大齿轮、小齿轮、踏面 是否报警，是一级还是二级报警
     *2. 若当前是有报警的，需要返回是哪个轴承在报警，也就是说一个测点有4个轴承，需要记录下来是哪个轴承的什么报警(保内、保外……)
     * 3. 也就是说需要将FaultResult、FaultResult_other遍历一遍，才能知道是哪个轴承在报警？？？？、FaultResult_zhou、other_Secondary大小齿轮和踏面只需要知道是几级报警即可
     * 4.用QStringList list.(0) 传出是否报警，总共8个，顺序跟1.相同，中间用 | 间隔。
     * list.(1)-list.(N) 是否报警，2|保外报警,3|保内报警 0|踏面？5|大齿轮？ 6|小齿轮？
*/

    quint8 FaultRet[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};//0：预警，1：一级报警， 2：二级报警
    QStringList dblist;
    //遍历轴承，若存在报警则添加日志
    for(int i=0;i<FaultTypeNum;i++){
        //先判断二级报警
        //判断DB值为N级报警
        quint8 db_result = 0xff,result = 0xff;
        dblist.clear();
        QVector<float> dbvalue;

        for(int j=0;j<Rank_5;j++){
//            float dbvalue = dblRangeArr[i][j];
            dbvalue.append(dblRangeArr[i][j]);

            dblist.append(QString::number(dblRangeArr[i][j]));
        }

        //排序
        std::sort(dbvalue.begin(), dbvalue.end());
        //取出db值的中位数
        float finalvalue = dbvalue.at(2);
//        qDebug()<<"finalvalue = " << finalvalue;
        //判断DB值是否报警
        if(finalvalue > DBData::ZhouLimit_Secondary.at(i%5)){
            db_result = 2;
        }else if(finalvalue > DBData::ZhouLimit_First.at(i%5)){
            db_result = 1;
        }else if(finalvalue > 0){
            db_result = 0;
        }

        //综合出诊断结果
        if((FaultResult_zhou[i]) || (db_result == 2)){//如果二级结果数组或者DB的值大于2级限定值
            if(APPSetting::UseVibrateSecondaryAlarm){
               result = 2;
            }
        }else if((FaultResult[i]) && (db_result == 1)){//再判断1级报警
            if(APPSetting::UseVibrateFirstLevelAlarm){
                result = 1;
            }
        }else if((FaultResult[i]) && (db_result == 0)){
            if(APPSetting::UseVibrateWarning){
               result = 0;
            }
        }

        if(result != 0xff){//存在报警
            if(((FaultRet[i%5] != 0xff) && (FaultRet[i%5] < result)) || (FaultRet[i%5] == 0xff)){//如果没记录或记录下的报警等级比当前低
                FaultRet[i%5] = result;
            }
            //判断i的范围
            int Bearing_Number = 0;//轴承编号
            if(i < 5) Bearing_Number = 1;
            else if(i < 10) Bearing_Number = 2;
            else if(i < 15) Bearing_Number = 3;
            else if(i < 20) Bearing_Number = 4;

            QString str;
            switch(i%5){
            case 0:
                str = QString("保外|%1级报警").arg(result);
                break;
            case 1:
                str = QString("保内|%1级报警").arg(result);
                break;
            case 2:
                str = QString("外环|%1级报警").arg(result);
                break;
            case 3:
                str = QString("内环|%1级报警").arg(result);
                break;
            case 4:
                str = QString("滚单|%1级报警").arg(result);
                break;
            }

            QString s = QString("%1|%2|%3").arg(Bearing_Number).arg(str).arg(dblist.join(","));
            list.append(s);
        }
    }
    //判断其他是否存在报警
    //踏面
    if(FaultResult_other[0]){
        quint8 tamianresult = 0;
        QString tastr = "踏面|预警";
        //存储DB值
        dblist.clear();
        QVector<float> dbvalue;
        for(int nRank=0;nRank<Rank_5;nRank++){

            float dbTa = dblRangeArr_Ta[FaultType.FaultOther.TaMian][nRank];
            dbvalue.append(dbTa);

            dblist.append(QString::number(dbTa));
        }
        //排序
        std::sort(dbvalue.begin(), dbvalue.end());
        //取出db值的中位数
        float finalvalue = dbvalue.at(2);
        if(finalvalue > DBData::OtherLimit_Secondary[0]){
            if(APPSetting::UseVibrateSecondaryAlarm){
                tamianresult = 2;
                tastr = "踏面|二级报警";
            }
        }else if(finalvalue > DBData::OtherLimit_First[0]){
            if(APPSetting::UseVibrateFirstLevelAlarm){
                tamianresult = 1;
                tastr = "踏面|一级报警";
            }
        }

        bool save = true;
        if((tamianresult == 0) && !APPSetting::UseVibrateWarning){
            save = false;
        }
        if(save){
            FaultRet[7] = tamianresult;
            QString s = QString("0|%1|%2").arg(tastr).arg(dblist.join(","));
            list.append(s);
        }
    }

    for(int ntype=0;ntype<2;ntype++){
        quint8 db_result = 0xff,result = 0xff;
        dblist.clear();
        QVector<float> dbvalues;
        for(int j=0;j<Rank_3;j++){
            float dbvalue = dbrange[ntype][j];
            dbvalues.append(dbvalue);
            dblist.append(QString::number(dbvalue));
        }

        //排序
        std::sort(dbvalues.begin(), dbvalues.end());
        //取出db值的中位数
        float finalvalue = dbvalues.at(1);

        if(finalvalue > DBData::OtherLimit_Secondary[ntype + 1]){
            db_result = 2;
        }else if(finalvalue > DBData::OtherLimit_First[ntype + 1]){
            db_result = 1;
        }else if(finalvalue > 0){
            db_result = 0;
        }
//        qDebug()<< "other_Secondary = " << other_Secondary[ntype];
        if((other_Secondary[ntype]) || (db_result == 2)){//如果二级结果数组或者DB的值大于2级限定值
             if(APPSetting::UseVibrateSecondaryAlarm){
                 result = 2;
             }
        }else if((FaultResult_other[ntype+2]) && (db_result == 1)){//再判断1级报警
             if(APPSetting::UseVibrateFirstLevelAlarm){
                 result = 1;
             }
        }else if((FaultResult_other[ntype+2]) && (db_result == 0)){
             if(APPSetting::UseVibrateWarning){
                 result = 0;
             }
        }
         if(result != 0xff){//存在报警
             if(ntype == 0){//大齿轮
                 FaultRet[5] = result;
                 QString s = QString("%1|大齿轮|%2级报警|%3").arg(0).arg(result).arg(dblist.join(","));
                 list.append(s);
             }else if(ntype == 1){//小齿轮
                 FaultRet[6] = result;
                 QString s = QString("%1|小齿轮|%2级报警|%3").arg(0).arg(result).arg(dblist.join(","));
                 list.append(s);
             }
         }

    }
    QString str = QString("%1|%2|%3|%4|%5|%6|%7|%8").arg(FaultRet[0]).arg(FaultRet[1]).arg(FaultRet[2]).arg(FaultRet[3]).arg(FaultRet[4]).arg(FaultRet[5]).arg(FaultRet[6]).arg(FaultRet[7]);

    ret.append(str);
    if(list.size() != 0){
        ret.append(list);
    }

    return ret;
//    qDebug()<<"************************************************";
//    qDebug()<<"************************************************";
}

void Algorithm_v2::InitMavinSys(char pointType, float Rev)
{
    memset(m_ArrMaxPoint,0,sizeof(m_ArrMaxPoint));
    memset(m_ArrMaxPoint_en,0,sizeof(m_ArrMaxPoint_en));
    memset(m_ArrMaxPoint_zhou,0,sizeof(m_ArrMaxPoint_zhou));
    memset(m_ArrMaxPoint_other,0,sizeof(m_ArrMaxPoint_other));
    memset(FaultResult,true,sizeof(FaultResult));
    memset(FaultResult_other,true,sizeof(FaultResult_other));

//    std::fill(&m_ArrMaxPoint[0][0], &m_ArrMaxPoint[0][0] + FaultTypeNum * Rank_5, 0);

    if(pointType == ZhouXiang){
        FaultResult_other[FaultType.FaultOther.DaChiLun] = false;
        FaultResult_other[FaultType.FaultOther.XiaoChiLun] = false;
        FaultResult_other[FaultType.FaultOther.ChiLun] = false;

        Parameters.speed_struct.Lspeed = Rev/60;
        Parameters.speed_struct.Hspeed = Parameters.speed_struct.Lspeed;

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing1.Angle == 0) && (Parameters.bearing1.MidDia == 0) && (Parameters.bearing1.RollerDia == 0)
                && (Parameters.bearing1.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing1.BaoWai] = false;
            FaultResult[FaultType.FaultBearing1.BaoNei] = false;
            FaultResult[FaultType.FaultBearing1.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing1.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing1.GunDan] = false;
        }

        if((Parameters.bearing2.Angle == 0) && (Parameters.bearing2.MidDia == 0) && (Parameters.bearing2.RollerDia == 0)
                && (Parameters.bearing2.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing2.BaoWai] = false;
            FaultResult[FaultType.FaultBearing2.BaoNei] = false;
            FaultResult[FaultType.FaultBearing2.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing2.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing2.GunDan] = false;
        }

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing3.Angle == 0) && (Parameters.bearing3.MidDia == 0) && (Parameters.bearing3.RollerDia == 0)
                && (Parameters.bearing3.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing3.BaoWai] = false;
            FaultResult[FaultType.FaultBearing3.BaoNei] = false;
            FaultResult[FaultType.FaultBearing3.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing3.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing3.GunDan] = false;
        }

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing4.Angle == 0) && (Parameters.bearing4.MidDia == 0) && (Parameters.bearing4.RollerDia == 0)
                && (Parameters.bearing4.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing4.BaoWai] = false;
            FaultResult[FaultType.FaultBearing4.BaoNei] = false;
            FaultResult[FaultType.FaultBearing4.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing4.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing4.GunDan] = false;
        }
    }else if(pointType == ChiLunXiang){
        FaultResult_other[FaultType.FaultOther.TaMian] = false;

        Parameters.speed_struct.Lspeed = Rev/60;
        Parameters.speed_struct.Hspeed = Parameters.speed_struct.Lspeed * Parameters.speed_struct.SpeedRatio;

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing1.Angle == 0) && (Parameters.bearing1.MidDia == 0) && (Parameters.bearing1.RollerDia == 0)
                && (Parameters.bearing1.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing1.BaoWai] = false;
            FaultResult[FaultType.FaultBearing1.BaoNei] = false;
            FaultResult[FaultType.FaultBearing1.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing1.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing1.GunDan] = false;
        }

        if((Parameters.bearing2.Angle == 0) && (Parameters.bearing2.MidDia == 0) && (Parameters.bearing2.RollerDia == 0)
                && (Parameters.bearing2.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing2.BaoWai] = false;
            FaultResult[FaultType.FaultBearing2.BaoNei] = false;
            FaultResult[FaultType.FaultBearing2.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing2.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing2.GunDan] = false;
        }

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing3.Angle == 0) && (Parameters.bearing3.MidDia == 0) && (Parameters.bearing3.RollerDia == 0)
                && (Parameters.bearing3.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing3.BaoWai] = false;
            FaultResult[FaultType.FaultBearing3.BaoNei] = false;
            FaultResult[FaultType.FaultBearing3.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing3.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing3.GunDan] = false;
        }

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing4.Angle == 0) && (Parameters.bearing4.MidDia == 0) && (Parameters.bearing4.RollerDia == 0)
                && (Parameters.bearing4.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing4.BaoWai] = false;
            FaultResult[FaultType.FaultBearing4.BaoNei] = false;
            FaultResult[FaultType.FaultBearing4.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing4.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing4.GunDan] = false;
        }
    } else if(pointType == Dianji){
        FaultResult_other[FaultType.FaultOther.DaChiLun] = false;
        FaultResult_other[FaultType.FaultOther.XiaoChiLun] = false;
        FaultResult_other[FaultType.FaultOther.ChiLun] = false;
        FaultResult_other[FaultType.FaultOther.TaMian] = false;

        Parameters.speed_struct.Lspeed = Rev*Parameters.speed_struct.SpeedRatio/60;
        Parameters.speed_struct.Hspeed = Parameters.speed_struct.Lspeed;

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing1.Angle == 0) && (Parameters.bearing1.MidDia == 0) && (Parameters.bearing1.RollerDia == 0)
                && (Parameters.bearing1.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing1.BaoWai] = false;
            FaultResult[FaultType.FaultBearing1.BaoNei] = false;
            FaultResult[FaultType.FaultBearing1.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing1.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing1.GunDan] = false;
        }

        if((Parameters.bearing2.Angle == 0) && (Parameters.bearing2.MidDia == 0) && (Parameters.bearing2.RollerDia == 0)
                && (Parameters.bearing2.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing2.BaoWai] = false;
            FaultResult[FaultType.FaultBearing2.BaoNei] = false;
            FaultResult[FaultType.FaultBearing2.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing2.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing2.GunDan] = false;
        }

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing3.Angle == 0) && (Parameters.bearing3.MidDia == 0) && (Parameters.bearing3.RollerDia == 0)
                && (Parameters.bearing3.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing3.BaoWai] = false;
            FaultResult[FaultType.FaultBearing3.BaoNei] = false;
            FaultResult[FaultType.FaultBearing3.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing3.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing3.GunDan] = false;
        }

        //通过是否有输入参数判断是否有这个轴承，没有就把后面的置false
        if((Parameters.bearing4.Angle == 0) && (Parameters.bearing4.MidDia == 0) && (Parameters.bearing4.RollerDia == 0)
                && (Parameters.bearing4.RollerAmount == 0)){
            FaultResult[FaultType.FaultBearing4.BaoWai] = false;
            FaultResult[FaultType.FaultBearing4.BaoNei] = false;
            FaultResult[FaultType.FaultBearing4.NeiHuan] = false;
            FaultResult[FaultType.FaultBearing4.WaiHuan] = false;
            FaultResult[FaultType.FaultBearing4.GunDan] = false;
        }
    }/*end Dianji*/
    //m_bearing.m_dblAngle = pi * m_bearing.m_dblAngle / 180.0;%%转换成弧度；这里不转弧度，后面直接用角度计算
}

void Algorithm_v2::ComputeFaultFre()
{
    double m_dblArrBaseFre[20],fTemp,BaseFre,m_dblArrBaseFre_other[4];
    memset(m_dblArrBaseFre,20,0.00);
    memset(m_dblArrBaseFre_other,4,0.00);
    if(FaultResult[FaultType.FaultBearing1.BaoWai]){
        fTemp = (double)(Parameters.bearing1.RollerDia * cos(Parameters.bearing1.Angle));//滚径*轴承接触角的余弦值
        BaseFre = Parameters.speed_struct.Lspeed;
        m_dblArrBaseFre[FaultType.FaultBearing1.BaoWai] = (Parameters.bearing1.MidDia - fTemp) / (2.0 * Parameters.bearing1.MidDia) *BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing1.BaoNei] = (Parameters.bearing1.MidDia + fTemp) / (2.0 * Parameters.bearing1.MidDia) * BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing1.WaiHuan] = m_dblArrBaseFre[FaultType.FaultBearing1.BaoWai] *Parameters.bearing1.RollerAmount;
//        m_dblArrBaseFre[FaultType.FaultBearing1.WaiHuan] = 60;
        m_dblArrBaseFre[FaultType.FaultBearing1.NeiHuan] = m_dblArrBaseFre[FaultType.FaultBearing1.BaoNei] * Parameters.bearing1.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing1.GunDan] = (Parameters.bearing1.MidDia * Parameters.bearing1.MidDia - fTemp * fTemp) / (2.0 * Parameters.bearing1.MidDia * Parameters.bearing1.RollerDia) * BaseFre;
    }
    if(FaultResult[FaultType.FaultBearing2.BaoWai]){
        fTemp = (double)(Parameters.bearing2.RollerDia * cos(Parameters.bearing2.Angle));//滚径*轴承接触角的余弦值
        BaseFre = Parameters.speed_struct.Lspeed;
        m_dblArrBaseFre[FaultType.FaultBearing2.BaoWai] = (Parameters.bearing2.MidDia - fTemp) / (2.0 * Parameters.bearing2.MidDia) *BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing2.BaoNei] = (Parameters.bearing2.MidDia + fTemp) / (2.0 * Parameters.bearing2.MidDia) * BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing2.WaiHuan] = m_dblArrBaseFre[FaultType.FaultBearing2.BaoWai] *Parameters.bearing2.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing2.NeiHuan] = m_dblArrBaseFre[FaultType.FaultBearing2.BaoNei] * Parameters.bearing2.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing2.GunDan] = (Parameters.bearing2.MidDia * Parameters.bearing2.MidDia - fTemp * fTemp) / (2.0 * Parameters.bearing2.MidDia * Parameters.bearing2.RollerDia) * BaseFre;
    }

    if(FaultResult[FaultType.FaultBearing3.BaoWai]){
        fTemp = (double)(Parameters.bearing3.RollerDia * cos(Parameters.bearing3.Angle));//滚径*轴承接触角的余弦值
        BaseFre = Parameters.speed_struct.Hspeed;
        m_dblArrBaseFre[FaultType.FaultBearing3.BaoWai] = (Parameters.bearing3.MidDia - fTemp) / (2.0 * Parameters.bearing3.MidDia) *BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing3.BaoNei] = (Parameters.bearing3.MidDia + fTemp) / (2.0 * Parameters.bearing3.MidDia) * BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing3.WaiHuan] = m_dblArrBaseFre[FaultType.FaultBearing3.BaoWai] *Parameters.bearing3.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing3.NeiHuan] = m_dblArrBaseFre[FaultType.FaultBearing3.BaoNei] * Parameters.bearing3.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing3.GunDan] = (Parameters.bearing3.MidDia * Parameters.bearing3.MidDia - fTemp * fTemp) / (2.0 * Parameters.bearing3.MidDia * Parameters.bearing3.RollerDia) * BaseFre;
    }

    if(FaultResult[FaultType.FaultBearing4.BaoWai]){
        fTemp = (double)(Parameters.bearing4.RollerDia * cos(Parameters.bearing4.Angle));//滚径*轴承接触角的余弦值
        BaseFre = Parameters.speed_struct.Hspeed;
        m_dblArrBaseFre[FaultType.FaultBearing4.BaoWai] = (Parameters.bearing4.MidDia - fTemp) / (2.0 * Parameters.bearing4.MidDia) *BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing4.BaoNei] = (Parameters.bearing4.MidDia + fTemp) / (2.0 * Parameters.bearing4.MidDia) * BaseFre;
        m_dblArrBaseFre[FaultType.FaultBearing4.WaiHuan] = m_dblArrBaseFre[FaultType.FaultBearing4.BaoWai] *Parameters.bearing4.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing4.NeiHuan] = m_dblArrBaseFre[FaultType.FaultBearing4.BaoNei] * Parameters.bearing4.RollerAmount;
        m_dblArrBaseFre[FaultType.FaultBearing4.GunDan] = (Parameters.bearing4.MidDia * Parameters.bearing4.MidDia - fTemp * fTemp) / (2.0 * Parameters.bearing4.MidDia * Parameters.bearing4.RollerDia) * BaseFre;
    }
    //齿轮啮合频率
    m_dblArrBaseFre_other[FaultType.FaultOther.TaMian] = Parameters.speed_struct.Lspeed;
    m_dblArrBaseFre_other[FaultType.FaultOther.ChiLun] = Parameters.speed_struct.Lspeed * Parameters.gear_number.BullGear;//大小齿轮啮合频率相同，所在轴转频*齿数
    m_dblArrBaseFre_other[FaultType.FaultOther.DaChiLun] = Parameters.speed_struct.Lspeed;
    m_dblArrBaseFre_other[FaultType.FaultOther.XiaoChiLun] = Parameters.speed_struct.Hspeed;

    for(int i=0;i<FaultTypeNum;i++){
//        qDebug()<< "m_dblArrBaseFre[" << i << "]" << m_dblArrBaseFre[i];
        m_dblArrMapBaseFre[i] = (float)(m_dblArrBaseFre[i]/Resolving.Original);
        if(i<FaultType_OtherNum){
//            qDebug()<< "m_dblArrBaseFre_other[" << i << "]" << m_dblArrBaseFre_other[i];
            m_dblArrMapBaseFre_other[i] = (float)(m_dblArrBaseFre_other[i]/Resolving.Original);
        }
    }

}

void Algorithm_v2::SearchMaxPoint_en(QVector<float> Data)
{
    float m_nArrPuNum[FaultTypeNum][Rank_5],m_nArrSearchWidth[FaultTypeNum][Rank_5];
    int nStartPos = 0,count = 0,nEndPos = 0,nMaxPos = 0;
    float fAverage = 0.0;
    float dblMaxVal = 0.0;
    for(int i=0;i<FaultTypeNum;i++){
        for(int j=0;j<Rank_5;j++){
            m_nArrPuNum[i][j] = round((j+1)*m_dblArrMapBaseFre[i]);//计算1-5阶的故障谱线号
            if(m_nArrPuNum[i][j] > FFT_RESULT_LEN.Demodulation){
                m_nArrPuNum[i][j] = 0;
                continue;
            }else{
                m_nArrSearchWidth[i][j] = round(0.5 + m_nArrPuNum[i][j] * 0.02);//改为了谱线号的0.02
                if(m_nArrSearchWidth[i][j] < 1){
                    m_nArrSearchWidth[i][j] = 1;
                }else if(m_nArrSearchWidth[i][j] >= 32){//防止搜索过多 32*0.3125=10hz（可修改）也就是最多搜索10hz范围
                    m_nArrSearchWidth[i][j] = 32;
                }
#if 0
                //搜索轴承
                /* 获取搜索极值点的起始点位置 ***************************/
                nStartPos = m_nArrPuNum[i][j] - m_nArrSearchWidth[i][j];
                if (nStartPos < 1)
                {
                    nStartPos = 1;
                }
                /* 获取搜索极值点的结束点位置 ***************************/
                nEndPos = m_nArrPuNum[i][j] + m_nArrSearchWidth[i][j];
                if (nEndPos > FFT_RESULT_LEN.Original - 1)
                {
                    nEndPos = FFT_RESULT_LEN.Original - 1;//当故障谱线号加宽度大于fmax-1时以famx-1为结束
                }
                /* 获取极值点的位置 **********************************/
                fAverage = dblMaxVal = 0.0;
                for (int k = nStartPos; k <= nEndPos; k++)
                {
                    fAverage += Data[k];
                    if (Data[k] > dblMaxVal)
                    {
                        dblMaxVal = Data[k];
                        nMaxPos = k;
                    }
                }
                /* 记录每一类每一阶的极值点处的谱线号 ********************/
                m_ArrMaxPoint[i][j].number = nMaxPos;
                FaultResult[i] = true;
                DiscreteAnalyse(i, j,nMaxPos,FFT_RESULT_LEN.Original,Data,Average.Demodulation);//判断主频的离散性
                if (((nMaxPos == nStartPos) && (Data[nStartPos] < Data[nStartPos - 1])) || ((nMaxPos == nEndPos) && (Data[nEndPos] < Data[nEndPos + 1]))){
                    FaultResult[i] = false;
                    continue;
                }
                if(FaultResult[i]){
                    count +=1;
                    m_ArrMaxPoint[i][j].value = dblMaxVal;
                }
#endif
            }
        }
    }

    for(int i=0;i<FaultTypeNum;i++){
        count = 0;
        for(int j=0;j<Rank_5;j++){
            //搜索轴承
            /* 获取搜索极值点的起始点位置 ***************************/
            nStartPos = m_nArrPuNum[i][j] - m_nArrSearchWidth[i][j];
            if (nStartPos < 1)
            {
                nStartPos = 1;
            }
            /* 获取搜索极值点的结束点位置 ***************************/
            nEndPos = m_nArrPuNum[i][j] + m_nArrSearchWidth[i][j];
            if (nEndPos > FFT_RESULT_LEN.Original - 1)
            {
                nEndPos = FFT_RESULT_LEN.Original - 1;//当故障谱线号加宽度大于fmax-1时以famx-1为结束
            }
            /* 获取极值点的位置 **********************************/
            fAverage = dblMaxVal = 0.0;
            for (int k = nStartPos; k <= nEndPos; k++)
            {
                fAverage += Data[k];
                if (Data[k] > dblMaxVal)
                {
                    dblMaxVal = Data[k];
                    nMaxPos = k;
                }
            }
            /* 记录每一类每一阶的极值点处的谱线号 ********************/
            m_ArrMaxPoint_en[i][j].number = nMaxPos;
            FaultResult[i] = true;
            DiscreteAnalyse(i, j,nMaxPos,FFT_RESULT_LEN.Original,Data,Average.Demodulation,FaultResult);//判断主频的离散性
            if (((nMaxPos == nStartPos) && (Data[nStartPos] < Data[nStartPos - 1])) || ((nMaxPos == nEndPos) && (Data[nEndPos] < Data[nEndPos + 1]))){
                FaultResult[i] = false;
                continue;
            }
            if(FaultResult[i]){
                count +=1;
                m_ArrMaxPoint_en[i][j].value = dblMaxVal;
            }
        }

        //做完离散性进行true的次数判定，超过3次算true
        if(count > 2){
            FaultResult[i] = true;
        }else{
            FaultResult[i] =false;
        }
    }
}

void Algorithm_v2::DiscreteAnalyse(int ntype, int nRank, int nMaxPos, int FFT_RESULT_LEN,QVector<float>data ,float ave,bool fault[])
{
    float dblDiscreteLimite = 0;	  /* 判定是否具备离散性的界限值 */
    static float dblRankBuChanA = 0; /* 阶数补偿值 */
    float dblXiShu = 0;			  /* 阶数补偿时的系数 */

    long nDiscreteStartPos = nMaxPos; /* 搜索起始点位置 */
    long nDiscreteEndPos = nMaxPos;	  /* 搜索结束点位置 */

    float dblFindBuChan = 1; /* 搜索点数限制式补偿 */
    long lDataLen = 0;		  /* 数据长度 */
    float dblAveRange = 0;	  /* 幅值平均值 */
    float *pdblRange = NULL; /* 指向从起始点到结束点幅值数据区的指针 */
    long nMidPos = 0;
    long i = 0;
    float dblBuChan = 0;	   /* 补偿值 */
    float dblMaxRange = 0;	   /* 搜索出的新区域减去平均值后的幅值极值*/
    float dblAveRangeNew = 0; /*搜索出的新区域减去平均值后的幅值平均值*/

    /* 能 量 值 太 低
    ***************************************************************************/
    if (data[nMaxPos] < 5.0 * ave) // ye 越高阶值约低
    {
        fault[ntype] = false;
        return;
    }


    if (nRank == 1)
    {
        dblRankBuChanA = 0;
    }

    /* 双 边 能 流 落 差 较 大
    *********************************************************************/
    if(nMaxPos<3){
        fault[ntype] = false;
        return;
    }

    if ((data[nMaxPos] / data[nMaxPos - 1] > 1.5) &&
        (data[nMaxPos] / data[nMaxPos - 2] > 2.4) &&
        (data[nMaxPos] / data[nMaxPos + 1] > 1.5) &&
        (data[nMaxPos] / data[nMaxPos + 2] > 2.4))
    {
        dblDiscreteLimite += 5;
    }
    else if ((data[nMaxPos] / data[nMaxPos - 1] > 1.1) &&
             (data[nMaxPos] / data[nMaxPos - 2] > 3.0) &&
             (data[nMaxPos] / data[nMaxPos + 1] > 1.1) &&
             (data[nMaxPos] / data[nMaxPos + 2] > 3.0))
    {
        dblDiscreteLimite += 5;
    }

    while (nDiscreteStartPos > 0) /* 搜索起始点位置 */
    {
        if (data[nDiscreteStartPos] < data[nDiscreteStartPos - 1])
        {
            break;
        }
        nDiscreteStartPos--;
        if (nDiscreteStartPos == 0)
        {
            break;
        }
    }

    while (nDiscreteEndPos < FFT_RESULT_LEN) /* 搜索结束点位置 */
    {
        if (data[nDiscreteEndPos] < data[nDiscreteEndPos + 1])
        {
            break;
        }
        nDiscreteEndPos++;
        if (nDiscreteEndPos == FFT_RESULT_LEN- 1)
        {
            break;
        }
    }

    lDataLen = nDiscreteEndPos - nDiscreteStartPos + 1;
    if (lDataLen < 2)
    {
        fault[ntype] = false;
        return;
    }

    pdblRange = (float *)malloc(sizeof(float) * lDataLen);
    for (i = nDiscreteStartPos; i <= nDiscreteEndPos; i++)
    {
        dblAveRange += data[i];
        pdblRange[i - nDiscreteStartPos] = data[i];
    }
    dblAveRange = dblAveRange / lDataLen;

    //如果左右搜索的点数都 >= 5, 则判断其左右第五点斜率,
    // x 轴每点的刻度以起始点的 1/5 来计算
    if (((nMaxPos - nDiscreteStartPos) >= 5) && ((nDiscreteEndPos - nMaxPos) >= 5))
    {
        float dblMaxRange = data[nMaxPos];
        float dblStartPosRange = data[nMaxPos - 5];
        float dblEndPosRange = data[nMaxPos + 5];
        if (((dblMaxRange - dblStartPosRange) / dblStartPosRange > 1.0f) &&
            ((dblMaxRange - dblEndPosRange) / dblEndPosRange > 1.0f) &&
            (dblMaxRange>5*dblAveRange))
        {
            fault[ntype] = true;
            return;
        }
    }


    nMidPos = lDataLen / 2;
    dblDiscreteLimite += 100.0 / lDataLen;

    if(!(lDataLen % 2)){
        for(i=0;i< nMidPos-1;i++){

            if(((pdblRange[nMidPos - i - 1] / pdblRange[nMidPos - i - 2] > 1.25) && (pdblRange[nMidPos - i - 1] / ave > 1.8))
                && ((pdblRange[nMidPos + i - 1] / pdblRange[nMidPos + i] > 1.25) && (pdblRange[nMidPos + i - 1] / ave > 1.8)))
                       dblDiscreteLimite +=3;
        }
    }else{
        for (i = 0; i < nMidPos; i++)
        {
            /* 双 单 边 能 流 落 差 很 大
        ***********************************************************/
            if (((pdblRange[nMidPos - i] / pdblRange[nMidPos - i - 1] > 1.25) &&
                 (pdblRange[nMidPos - i] / ave > 1.8)) &&
                    ((pdblRange[nMidPos + i] / pdblRange[nMidPos + i + 1] > 1.25) &&
                     (pdblRange[nMidPos + i] / ave > 1.8)))
            {
                dblDiscreteLimite += 3;
            }
        }
    }

    /* 出现两个相近幅值的极值点时会导致本来离散性
    较好的图形判断为不好的解决方法 *************/
    if ((fabs(data[nMaxPos] - data[nMaxPos - 1]) / data[nMaxPos] < 0.06) || (fabs(data[nMaxPos] - data[nMaxPos + 1]) / data[nMaxPos] <0.06))
    {

        dblDiscreteLimite += 100.0 / lDataLen;
    }

    /* 能量补偿 ********************************************************/
    dblBuChan = data[nMaxPos] / ave;
    if (dblBuChan < 3) /* 能量值达不到平均能量值的三倍时不予补偿 */
    {
        dblBuChan = 0;
    }
    if (dblBuChan > 10)
    {
        dblBuChan = 10;
    }
    dblDiscreteLimite += dblBuChan;

    /* 宽度补偿 *******************************************************/
    dblBuChan = 0;
    if (lDataLen > 3)
    {
        dblBuChan = log10(pow(2.0, lDataLen)) * lDataLen / 3.6;
    }
    if (dblBuChan > dblDiscreteLimite / 2)
    {
        dblBuChan /= 1.8;
    }

    dblDiscreteLimite += dblBuChan;
    /* 阶数补偿 **********************************************************/
    if (nRank > 1 )
    {
        dblXiShu = dblRankBuChanA;
        dblXiShu = pow(5.0, nRank - 2) * (dblRankBuChanA * dblRankBuChanA + dblRankBuChanA);
        dblDiscreteLimite += dblXiShu;
    }

    /* 计算搜索区域的减去平均值后的平均值 *************************/
    for (i = 0; i < lDataLen; i++)
    {
        if (pdblRange[i] < dblAveRange)
        {
            pdblRange[i] = 0;//将范围内小于平均值的数据归0
        }
        else
        {
            pdblRange[i] -= dblAveRange;
        }
        if (dblMaxRange < pdblRange[i])//搜索出的新区域减去平均值后的幅值极值
        {
            dblMaxRange = pdblRange[i];
        }

        dblAveRangeNew += pdblRange[i];//计算搜索区域的减去平均值后的平均值
    }

    dblAveRangeNew /= lDataLen;

    /* 计算阶数补偿值 ****************************************************/
    if (nRank == 1)
    {
        dblRankBuChanA = dblDiscreteLimite / (100.0 * dblAveRangeNew / dblMaxRange);//一阶时计算值为二阶做补偿
    }
    else
    {
        dblRankBuChanA = dblRankBuChanA * 0.7 + dblDiscreteLimite / (100.0 * dblAveRangeNew / dblMaxRange) * 0.3;//二阶时计算值为三阶补偿
    }
    dblAveRangeNew = 100.0 * dblAveRangeNew / dblMaxRange; // 0.88 是后来加的 *dblXiShu


    /* 判定离散性 ***************************************************/

    if (dblAveRangeNew < dblDiscreteLimite)
    {
        fault[ntype] = true;
    }
    else
    {
        fault[ntype] = false;
    }
    free(pdblRange);
}

void Algorithm_v2::SideFrenJudge_en(QVector<float> Data)
{
    int nBianPin[12][5][5]; /* 边频 */
    int nStartPos = 0, nEndPos = 0;
    int nFre = 0, nMainFre = 0; /* dblMaxVal: 最大幅值， nFre: 最大幅值所对应的频率*/
    float dblMaxVal = 0.0f;
    memset(nBianPin,0,sizeof(nBianPin));
    int falntype[12]={2,3,4,7,8,9,12,13,14,17,18,19};
    for(int nType=0;nType<12;nType++){
        if(!FaultResult[falntype[nType]]) continue;
        for(int nRank=0;nRank<Rank_5;nRank++){
            if(m_ArrMaxPoint_en[nType][nRank].number == 0) continue;
            float findmap = 0.0;
            for(int i=-2;i<2;i++){
                /* nType :类, nRank: 阶, i: 写 -2, -1 :低 2 边频, i=0:主频谱, i = 1, 2 :高 2 边频 *******/
                switch (nType) {
                case 0:
                case 1:
                case 3:
                case 4:
                    nBianPin[nType][nRank][i + 2] = round(m_ArrMaxPoint_en[falntype[nType]][nRank].number + i * Parameters.speed_struct.Lnum);
                    findmap=round(Parameters.speed_struct.Lnum*0.04);//搜索范围
                    if (findmap<=1) findmap=1;
                    break;
                case 2:
                case 5:
                case 8:
                case 11:
                    nBianPin[nType][nRank][i + 2] = round(m_ArrMaxPoint_en[falntype[nType]][nRank].number + i * m_ArrMaxPoint_en[falntype[nType]-4][0].number);
                    findmap=round(m_ArrMaxPoint_en[falntype[nType]-4][0].number*0.04);//搜索范围
                    if (findmap<=1) findmap=1;
                    break;
                case 6:
                case 7:
                case 9:
                case 10:
                    nBianPin[nType][nRank][i + 2] = round(m_ArrMaxPoint_en[falntype[nType]][nRank].number + i * Parameters.speed_struct.Hnum);
                    findmap=round(Parameters.speed_struct.Hnum*0.04);//搜索范围
                    if (findmap<=1) findmap=1;
                    break;
                }
                if(i == 0) continue;/* 此时 nBianPin[nType][nRank-1][2] 中存储的为主频， 不能进行频谱搜索*/
                if(nBianPin[nType][nRank][i+2] < 2) nBianPin[nType][nRank][i+2] = 2;//防止边频搜索过低

                if (nBianPin[nType][nRank - 1][i + 2] > FFT_RESULT_LEN.Demodulation - 3) //防止边频搜索过高
                {
                    nBianPin[nType][nRank - 1][i + 2] = FFT_RESULT_LEN.Demodulation - 3;
                }

                nStartPos = nBianPin[nType][nRank][i + 2] - findmap;//边频搜索左右
                nEndPos = nBianPin[nType][nRank][i + 2] + findmap;
                if(nStartPos < 1) nStartPos = 1;
                if(nEndPos > (FFT_RESULT_LEN.Demodulation - 1)) nEndPos = FFT_RESULT_LEN.Demodulation - 1;
                dblMaxVal = 0.0f;
                nFre = 0;

                for (int j = nStartPos; j <= nEndPos; j++)
                {
                    if (Data[j] > dblMaxVal)
                    {
                        dblMaxVal = Data[j];
                        nFre = j;//搜索边频最大的极值点
                    }
                }
                nBianPin[nType][nRank][i + 2] = nFre;/* 存储理论上存在的边频极值点处的频率 */
            }
            /* 确定是否确实存在边频， 判断原则: "边频 < 主频"、 "边频基本对称"*/
            nMainFre = nBianPin[nType][nRank][2]; /* 主频*/
            for(int i=0;i<5;i++){
                int nRangVal1 = 0, nRangVal2 = 0;
                int nBianPinFre1 = 0, nBianPinFre2 = 0;
                if (i == 2) /* [2]为主频 */
                {
                    continue;
                } /* 如 果 边 频 的 幅 值 > 主 频 幅 值 则 认 为 不 是 边 频*/
                nBianPinFre1 = nBianPin[nType][nRank][i];
                nRangVal1 = (int)Data[nBianPinFre1];
                nRangVal2 = (int)Data[nMainFre];
                if (nRangVal1 >= nRangVal2)
                {
                    nBianPin[nType][nRank][i] = 0;//认为不是边频置1，防止跑对称的时候报错
                    nBianPin[nType][nRank][4 - i] = 0;
                    continue;
                }

                nBianPinFre2 = nBianPin[nType][nRank][4 - i];
                nRangVal1 = (int)Data[nBianPinFre1];
                nRangVal2 = (int)Data[nBianPinFre2];
                if (nRangVal1 >= nRangVal2)//对两个边频进行排序，保证nRangVal1<nRangVal2
                {
                    int nTemp = nRangVal1;
                    nRangVal1 = nRangVal2;
                    nRangVal2 = nTemp;
                }

                /* 如 果 对 称 边 频 的 幅 度 不 近 似 相 等 则 认 为 不 是 边 频*/
                if (nRangVal1 < 0.9f * nRangVal2)
                {
                    nBianPin[nType][nRank][i] = 0;
                    nBianPin[nType][nRank][4 - i] = 0;
                }
            }
        }
    }
    /* 计 算 边 频 频 率*/
    for(int i=0;i<FaultTypeNum;i++){
        if(!FaultResult[i]) continue;
        for(int j=0;j<Rank_5;j++){
            if(m_ArrMaxPoint_en[i][j].number == 1) continue;
            for(int nBianPinType=0;nBianPinType<12;nBianPinType++){//边频类型
                for(int nBianPinRank=0;nBianPinRank<5;nBianPinRank++){//主频阶数
                    for(int k=0;k<Rank_5;k++){//0, 1 : 低 2 边频, 2 : 主频, 3, 4 : 高 2 边频
                        if(k==2) continue;//2为主频
                        nFre = m_ArrMaxPoint_en[i][j].number;
                        if(nFre == nBianPin[nBianPinType][nBianPinRank][k]){
                            FaultResult[i] = false;
                        }
                    }
                }
            }
        }
    }
}

void Algorithm_v2::MavinAlert_en(float ave, quint32 Rev, float Diameter)
{
    memset(dblRangeArr,0,sizeof(dblRangeArr));
    memset(dblRangeArr_tz,0,sizeof(dblRangeArr_tz));
    int m_bFault_point = 0;
    for(int ntype=0;ntype<FaultTypeNum;ntype++){
        if(FaultResult[ntype]){
            for(int nRank=0;nRank<Rank_5;nRank++){
                if(m_ArrMaxPoint_en[ntype][nRank].value == 0){
                    dblRangeArr[ntype][nRank] = 0;
                    m_bFault_point += 1;
                    continue;
                }
                dblRangeArr[ntype][nRank] = 20*log10(m_ArrMaxPoint_en[ntype][nRank].value/ave);//相对于平均值的db计算
                dblRangeArr_tz[ntype][nRank]=20*log10(200000*m_ArrMaxPoint_en[ntype][nRank].value/(Rev*pow(Diameter,0.6)));//标准车计算方法，后续需要修改高低轴转速
                if(dblRangeArr[ntype][nRank] < 25){
                    m_bFault_point += 1;
                }
            }
            if(m_bFault_point > 1){
                FaultResult[ntype] = false;
            }
        }
    }
}

void Algorithm_v2::SearchMaxPoint_zhou(QVector<float> Data, float ave)
{
    float m_nArrPuNum[FaultTypeNum][Rank_5],m_nArrSearchWidth[FaultTypeNum][Rank_5];
    int nStartPos = 0,count = 0,nEndPos = 0,nMaxPos = 0;
    float fAverage = 0.0;
    float dblMaxVal = 0.0;
    for(int i=0;i<FaultTypeNum;i++){
        for(int j=0;j<Rank_5;j++){
            m_nArrPuNum[i][j] = round((j+1)*m_dblArrMapBaseFre[i]);//计算1-5阶的故障谱线号
            if(m_nArrPuNum[i][j] >= FFT_RESULT_LEN.Demodulation){
                m_nArrPuNum[i][j] = 0;
                continue;
            }else{
                m_nArrSearchWidth[i][j] = round(0.5 + m_nArrPuNum[i][j] * 0.02);//改为了谱线号的0.02
                if(m_nArrSearchWidth[i][j] < 1){
                    m_nArrSearchWidth[i][j] = 1;
                }else if(m_nArrSearchWidth[i][j] >= 32){//防止搜索过多 32*0.3125=10hz（可修改）也就是最多搜索10hz范围
                    m_nArrSearchWidth[i][j] = 32;
                }
            }
        }
    }
    for(int ntype=0;ntype<FaultTypeNum;ntype++){
        count = 0;
        for(int nRank=0;nRank<Rank_5;nRank++){
            if(m_nArrPuNum[ntype][nRank] != 0){//防止去搜索超出范围的谱线
                nStartPos = m_nArrPuNum[ntype][nRank] - m_nArrSearchWidth[ntype][nRank];
                if(nStartPos <= 1){/* 获取搜索极值点的起始点位置 */
                    nStartPos = 1;
                }
                nEndPos = m_nArrPuNum[ntype][nRank] + m_nArrSearchWidth[ntype][nRank];/* 获取搜索极值点的结束点位置 */
                if(nEndPos > (FFT_RESULT_LEN.Original -1)){//此处传进来的应该是原始数据
                    nEndPos = FFT_RESULT_LEN.Original -1;//当故障谱线号加宽度大于fmax-1时以famx-1为结束
                }
                /* 获取极值点的位置 */
                dblMaxVal = fAverage = nMaxPos = 0;
                for(int i=nStartPos;i<nEndPos;i++){
                    fAverage +=Data[i];
                    if(Data[i] > dblMaxVal){
                        dblMaxVal = Data[i];
                        nMaxPos = i;
                    }
                }
                /* 记录每一类每一阶的极值点处的谱线号 */
                m_ArrMaxPoint_zhou[ntype][nRank].value = nMaxPos;
                FaultResult_zhou[ntype] = true;
                DiscreteAnalyse(ntype,nRank,nMaxPos,FFT_RESULT_LEN.Original,Data,ave,FaultResult_zhou);
                if (((nMaxPos == nStartPos) && (Data[nStartPos] < Data[nStartPos - 1])) || ((nMaxPos == nEndPos) && (Data[nEndPos] < Data[nEndPos + 1]))){
                    FaultResult_zhou[ntype] = false;
                    continue;
                }
                if(FaultResult_zhou[ntype]){
                    count +=1;
                    m_ArrMaxPoint_zhou[ntype][nRank].value = dblMaxVal;
                }
            }
        }
        if(count > 2){
            FaultResult_zhou[ntype] = true;
        }else{
            FaultResult_zhou[ntype] = false;
        }
    }
}

void Algorithm_v2::MavinAlert_zhou(float ave, quint32 Rev, float Diameter)
{
    memset(dblRangeArr_zhou,0,sizeof(dblRangeArr_zhou));
    memset(dblRangeArr_zhou_tz,0,sizeof(dblRangeArr_zhou_tz));
    int m_bFault_point = 0;
    for(int ntype=0;ntype<FaultTypeNum;ntype++){
        if(FaultResult_zhou[ntype]){
            for(int nRank=0;nRank<Rank_5;nRank++){
                if(m_ArrMaxPoint_zhou[ntype][nRank].value == 0){
                    dblRangeArr_zhou[ntype][nRank] = 0;
                    m_bFault_point += 1;
                    continue;
                }
                dblRangeArr_zhou[ntype][nRank] = 20*log10(m_ArrMaxPoint_zhou[ntype][nRank].value/ave);//相对于平均值的db计算
                dblRangeArr_zhou_tz[ntype][nRank]=20*log10(200000*m_ArrMaxPoint_zhou[ntype][nRank].value/(Rev*pow(Diameter,0.6)));//标准车计算方法，后续需要修改高低轴转速
                if(dblRangeArr_zhou[ntype][nRank] < 25){
                    m_bFault_point += 1;
                }
            }
            if(m_bFault_point > 1){
                FaultResult_zhou[ntype] = false;
            }
        }
    }
}

void Algorithm_v2::SearchMaxPoint_ta(QVector<float> Data, float ave)
{
    float m_nArrPuNum[FaultTypeNum][Rank_5],m_nArrSearchWidth[FaultTypeNum][Rank_5];
    int nStartPos = 0,count = 0,nEndPos = 0,nMaxPos = 0;
    float fAverage = 0.0;
    float dblMaxVal = 0.0;
    int ntype = FaultType.FaultOther.TaMian;
    for(int nRank=0;nRank<Rank_5;nRank++){
        m_nArrPuNum[ntype][nRank] = round((nRank+1)*m_dblArrMapBaseFre_other[ntype]);//计算1-5阶的故障谱线号
        if(m_nArrPuNum[ntype][nRank] >= FFT_RESULT_LEN.Original){
            m_nArrPuNum[ntype][nRank] = 0;
            continue;
        }else{
            m_nArrSearchWidth[ntype][nRank] = round(0.5 + m_nArrPuNum[ntype][nRank] * 0.02);//改为了谱线号的0.04
            if(m_nArrSearchWidth[ntype][nRank] < 1){
                m_nArrSearchWidth[ntype][nRank] = 1;
            }else if(m_nArrSearchWidth[ntype][nRank] >= 40){//防止搜索过多 40*0.25=10hz （可修改）1000的谱线号
                m_nArrSearchWidth[ntype][nRank] = 40;
            }
        }
    }
    for(int nRank=0;nRank<Rank_5;nRank++){
        if(m_nArrPuNum[ntype][nRank] != 0){//防止去搜索超出范围的谱线
            nStartPos = m_nArrPuNum[ntype][nRank] - m_nArrSearchWidth[ntype][nRank];
            if(nStartPos <= 1){/* 获取搜索极值点的起始点位置 */
                nStartPos = 1;
            }
            nEndPos = m_nArrPuNum[ntype][nRank] + m_nArrSearchWidth[ntype][nRank];/* 获取搜索极值点的结束点位置 */
            if(nEndPos > (FFT_RESULT_LEN.Original -1)){//此处传进来的应该是原始数据
                nEndPos = FFT_RESULT_LEN.Original -1;//当故障谱线号加宽度大于fmax-1时以famx-1为结束
            }
            /* 获取极值点的位置 */
            dblMaxVal = fAverage = nMaxPos = 0;
            for(int i=nStartPos;i<nEndPos;i++){
                fAverage +=Data[i];
                if(Data[i] > dblMaxVal){
                    dblMaxVal = Data[i];
                    nMaxPos = i;
                }
            }
            /* 记录每一类每一阶的极值点处的谱线号 */
            m_ArrMaxPoint_other[ntype][nRank].value = nMaxPos;
            FaultResult_other[ntype] = true;
            DiscreteAnalyse(ntype,nRank,nMaxPos,FFT_RESULT_LEN.Original,Data,ave,FaultResult_other);
            if (((nMaxPos == nStartPos) && (Data[nStartPos] < Data[nStartPos - 1])) || ((nMaxPos == nEndPos) && (Data[nEndPos] < Data[nEndPos + 1]))){
                FaultResult_other[ntype] = false;
                continue;
            }
            if(FaultResult_other[ntype]){
                count +=1;
                m_ArrMaxPoint_other[ntype][nRank].value = dblMaxVal;
            }
        }
    }
    if(count > 2){//做完离散性进行true的次数判定，超过2次算true
        FaultResult_other[ntype] = true;
    }else{
        FaultResult_other[ntype] = false;
    }
}

void Algorithm_v2::MavinAlert_ta(float ave)
{
    int ntype = FaultType.FaultOther.TaMian;
    if(!FaultResult_other[ntype]) return;
    memset(dblRangeArr_Ta,0,sizeof(dblRangeArr_Ta));
    int m_bFault_point = 0;
    for(int nRank=0;nRank<Rank_5;nRank++){
        dblRangeArr_Ta[ntype][nRank] = 20*log10(m_ArrMaxPoint_other[ntype][nRank].value/ave);//相对于平均值的db计算
        if(dblRangeArr_Ta[ntype][nRank] <= 15){
            m_bFault_point += 1;
        }
    }
    if(m_bFault_point > 1)
        FaultResult_other[ntype] = false;
}

void Algorithm_v2::SearchMaxPoint(QVector<float> Data, float ave)
{
    float m_nArrPuNum[9][Rank_3],m_nArrSearchWidth[9][Rank_3];
    int nStartPos = 0,count = 0,nEndPos = 0,nMaxPos = 0;
    float fAverage = 0.0;
    float dblMaxVal = 0.0;
    int ntype = FaultType.FaultOther.ChiLun;
    for(int nRank=0;nRank<Rank_3;nRank++){
        m_nArrPuNum[ntype][nRank] = round((nRank+1)*m_dblArrMapBaseFre_other[ntype]);//计算1-3阶的故障谱线号
        if(m_nArrPuNum[ntype][nRank] >= FFT_RESULT_LEN.Original){
            m_nArrPuNum[ntype][nRank] = 0;
            continue;
        }else{
            m_nArrSearchWidth[ntype][nRank] = round(0.5 + m_nArrPuNum[ntype][nRank] * 0.04);//改为了谱线号的0.04
            if(m_nArrSearchWidth[ntype][nRank] < 1){
                m_nArrSearchWidth[ntype][nRank] = 1;
            }
        }
    }

    for(int nRank=0;nRank<Rank_3;nRank++){
        if(m_nArrPuNum[ntype][nRank] != 0){//防止去搜索超出范围的谱线
            nStartPos = m_nArrPuNum[ntype][nRank] - m_nArrSearchWidth[ntype][nRank];
            if(nStartPos <= 1){/* 获取搜索极值点的起始点位置 */
                nStartPos = 1;
            }
            nEndPos = m_nArrPuNum[ntype][nRank] + m_nArrSearchWidth[ntype][nRank];/* 获取搜索极值点的结束点位置 */
            if(nEndPos > (FFT_RESULT_LEN.Original -1)){//此处传进来的应该是原始数据
                nEndPos = FFT_RESULT_LEN.Original -1;//当故障谱线号加宽度大于fmax-1时以famx-1为结束
            }
            /* 获取极值点的位置 */
            dblMaxVal = fAverage = nMaxPos = 0;
            for(int i=nStartPos;i<nEndPos;i++){
                fAverage +=Data[i];
                if(Data[i] > dblMaxVal){
                    dblMaxVal = Data[i];
                    nMaxPos = i;
                }
            }
            /* 记录每一类每一阶的极值点处的谱线号 */
            m_ArrMaxPoint_other[ntype][nRank].number = nMaxPos;
//            qDebug()<<"ntype == " << ntype << "rank = " << nRank << "nMaxPos = " << nMaxPos;
            FaultResult_other[ntype] = true;
            DiscreteAnalyse(ntype,nRank,nMaxPos,FFT_RESULT_LEN.Original,Data,ave,FaultResult_other);
            if (((nMaxPos == nStartPos) && (Data[nStartPos] < Data[nStartPos - 1])) || ((nMaxPos == nEndPos) && (Data[nEndPos] < Data[nEndPos + 1]))){
                FaultResult_other[ntype] = false;
                continue;
            }
            if(FaultResult_other[ntype]){
                count +=1;
                m_ArrMaxPoint_other[ntype][nRank].value = dblMaxVal;
            }
        }
    }
    if(count > 2){//做完离散性进行true的次数判定，超过2次算true
        FaultResult_other[ntype] = true;
    }else{
        FaultResult_other[ntype] = false;
    }
}

void Algorithm_v2::SideFrenJudge(QVector<float> Data, float ave)
{
    int nBianPin[2][3][21];

    float nBianPinave[2][3];
    float nBianPinsum[2][3];
//    bool m[2][3][21];
//    memset(nBianPin,1,sizeof(nBianPin));
//    memset(dbrange,0,sizeof(dbrange));
//    memset(nBianPinave,0,sizeof(nBianPinave));
//    memset(nBianPinsum,0,sizeof(nBianPinsum));
//    memset(m,true,sizeof(m));
    std::fill(&nBianPin[0][0][0], &nBianPin[0][0][0] + 2 * 3 * 21, 1);
    std::fill(&dbrange[0][0], &dbrange[0][0] + 2 * 3 , 0);
    std::fill(&nBianPinave[0][0], &nBianPinave[0][0] + 2 * 3 , 0);
    std::fill(&nBianPinsum[0][0], &nBianPinsum[0][0] + 2 * 3 , 0);
    std::fill(&m[0][0][0], &m[0][0][0] + 2 * 3 * 21, true);
    float findmap = 0.0;
    int nStartPos = 0,nEndPos = 0,nRangVal1,nRangVal2,nrankcount = 0,nicount = 0;
    float nFre = 0.0,nMainFre = 0.0,nBianPinFre1;
    float dblMaxVal = 0.0;
    for(int ntype=0;ntype < 2;ntype++){
        for(int nRank=0;nRank<3;nRank++){
            if(m_ArrMaxPoint_other[FaultType.FaultOther.ChiLun][nRank].number != 1){
                for(int i=-10;i<11;i++){
                    //* nType :类, nRank: 阶, i: 低 3 边频, i=0:主频率, 高 3边频
                    switch (ntype) {
                    case 0:
                        nBianPin[ntype][nRank][i+10] = (m_ArrMaxPoint_other[FaultType.FaultOther.ChiLun][nRank].number + round(i * m_dblArrMapBaseFre_other[FaultType.FaultOther.DaChiLun]));
                        findmap = round(Parameters.speed_struct.Lnum * 0.1);
                        if(findmap < 1){
                            findmap = 1;
                        }
                        break;
                    case 1:
                        nBianPin[ntype][nRank][i+10] = (m_ArrMaxPoint_other[FaultType.FaultOther.ChiLun][nRank].number + round(i * m_dblArrMapBaseFre_other[FaultType.FaultOther.XiaoChiLun]));
                        findmap = round(Parameters.speed_struct.Hnum * 0.1);
                        if(findmap < 1){
                            findmap = 1;
                        }
                        break;
                    }
                    if(i == 0) continue;//此时 nBianPin[nType][nRank-1][2] 中存储的为主频， 不能进行频谱搜索
//                    qDebug()<<"nRank =" << nRank << "+++++++++++++++++++++++++++++++++++++++++++++++++";
                    if(nBianPin[ntype][nRank][i+10] < 2){//防止边频搜索过低
                        nBianPin[ntype][nRank][i+10] = 2;
                    }

                    if(nBianPin[ntype][nRank][i+10] > (FFT_RESULT_LEN.Original -3)){//防止边频搜索过高
                        nBianPin[ntype][nRank][i+10] = FFT_RESULT_LEN.Original -3;
                    }

                    nStartPos = nBianPin[ntype][nRank][i+10] - findmap;//边频搜索左右
                    if(nStartPos < 1) nStartPos = 1;
                    nEndPos = nBianPin[ntype][nRank][i+10] + findmap;
                    if(nEndPos > (FFT_RESULT_LEN.Original - 1)){
                        nEndPos = FFT_RESULT_LEN.Original - 1;
                    }
                    dblMaxVal = nFre = 0.0;

                    for(int j =nStartPos;j<nEndPos;j++){
                        if(Data[j] > dblMaxVal){
                            dblMaxVal = Data[j];
                            nFre = j;
                        }
                    }
                    nBianPin[ntype][nRank][i+10] = nFre;//存储理论上存在的边频极值点处的谱线号
                }
//                qDebug()<<"nRank =" << nRank<<"-----------------------------------";
                //确定是否确实存在边频， 判断原则: "边频 < 主频
                nMainFre = nBianPin[ntype][nRank][10];/* 主频*/

                for(int i=0;i<21;i++){
                    if(nBianPin[ntype][nRank][i] < 5){//边频搜索过小时置1
                        nBianPin[ntype][nRank][i] = 1;
                    }

                    if(i == 10) continue;//主频

                    //如 果 边 频 的 幅 值 > 主 频 幅 值 则 认 为 不 是 边 频
                    nBianPinFre1 = nBianPin[ntype][nRank][i];
                    float nRangVal3 = Data[nBianPinFre1];//存放边频幅值
                    float nRangVal4 = Data[nMainFre];//存放主频幅值
                    if(nRangVal3 >= nRangVal4){
                        nBianPin[ntype][nRank][i] = 1;
                        continue;
                    }
                }
//                qDebug()<<"nRank =" << nRank<<"**********************************";
                //计算主频和边频范围内的平均值，用边频进行平均值对比
                for(int i=0;i<11;i++){
                    if(i==10){
                        nBianPinave[ntype][nRank] = 0;//如果搜到最后都没有边频就赋0，这样做db时为inf
                        break;
                    }

                    if((nBianPin[ntype][nRank][i] == 1)||(nBianPin[ntype][nRank][20-i] == 1)) continue;

                    nRangVal1 = nBianPin[ntype][nRank][i];
                    nRangVal2 = nBianPin[ntype][nRank][20-i];

                    nBianPinsum[ntype][nRank] = std::accumulate(Data.begin()+nRangVal1,Data.begin()+nRangVal2+1,0.0);

                    nBianPinave[ntype][nRank] = (nBianPinsum[ntype][nRank] - Data[nMainFre])/(nRangVal2-nRangVal1);
                    break;
                }
            }
        }
    }
    //边频离散性搜索
    for(int ntype=0;ntype<2;ntype++){
        nrankcount = 0;
        for(int nRank=0;nRank<3;nRank++){
            nicount = 0;
            for(int i=-10;i<11;i++){
                bool mb[1] = {true};
                if((nBianPin[ntype][nRank][i+10] == 1)|| (i==0)){
                    m[ntype][nRank][i+10] = false;
                    continue;
                }else{
                    DiscreteAnalyse(0,nRank,nBianPin[ntype][nRank][i+10],FFT_RESULT_LEN.Original,Data,ave,mb);
                    m[ntype][nRank][i+10] = mb[0];
                }

                if(!m[ntype][nRank][i+10]){
                    nBianPin[ntype][nRank][i+10] = 1;
                }
            }

            for(int i=7;i<14;i++){
                if(i != 10){
                    if(!m[ntype][nRank][i]){
                        nicount +=1;
                    }
                }
            }

            if(nicount > 2){
                nrankcount += 1;
            }
        }
        if(nrankcount > 1){
            FaultResult_other[ntype+2] = false;
        }
    }

    float nrangusm[2][3];
    memset(nrangusm,0,sizeof(nrangusm));
    for(int ntype=0;ntype<2;ntype++){
        nrankcount = 0;
        for(int nRank=0;nRank<3;nRank++){
//            qDebug()<<"ntype == " << ntype << "rank = " << nRank << "nBianPinave = " << nBianPinave[ntype][nRank];
            for(int i=7;i<14;i++){
                if(i!=10){
                    float nrangval = Data[nBianPin[ntype][nRank][i]];
                    nrangusm[ntype][nRank] += nrangval;
                }
            }
            if(nBianPinave[ntype][nRank] == 0) dbrange[ntype][nRank] = 0;
            else{
                dbrange[ntype][nRank] = 20*log10(nrangusm[ntype][nRank]/nBianPinave[ntype][nRank]);//计算边频的db值
            }
            if(dbrange[ntype][nRank] <= 26){
                nrankcount += 1;
            }
        }
        if(nrankcount >= 2){
            FaultResult_other[ntype+2] = false;
        }
    }
}
