#ifndef ALGORITHM_V2_H
#define ALGORITHM_V2_H

#include <QObject>
#include <numeric>
#include <cmath>
#include <QtDebug>
#include "dbdata.h"
#define FaultType_OtherNum 4
#define FaultTypeNum 20
#define RANGE_JUDGE_LIMIT 0
#define SECONDARY_DB 34
#define ONELEVEL_DB  23
#define Rank_5 5
#define Rank_3 3
struct BEARING_ARG{
    float Angle;//轴承接触角
    float MidDia;//中经
    float RollerDia;//滚径
    float RollerAmount;//滚子数
};//BEARING_ARG

struct SHAFTDIAMETER_ARG{
    float HighDiameter;//高速轴轴径
    float LowDiameter;//低速轴轴径
};

struct SPEED_STRUCT{
    float SpeedRatio;//转速比
    quint32 Rev;//车速
    float Lspeed;//低速轴转频
    float Hspeed;//高速轴转频
    float Lnum;//低速轴转频谱线号
    float Hnum;//高速轴转频谱线号
};

struct GEAR_NUMBER{
    int BullGear;//大齿轮
    int Pinion;//小齿轮
};

struct ALGPRITHM_PARAMETERS{
    BEARING_ARG bearing1;
    BEARING_ARG bearing2;
    BEARING_ARG bearing3;
    BEARING_ARG bearing4;
    SHAFTDIAMETER_ARG ShaftDiameter;
    SPEED_STRUCT speed_struct;
    GEAR_NUMBER gear_number;
    char PointType;
};

struct FAULT{
    quint8 BaoWai;
    quint8 BaoNei;
    quint8 WaiHuan;
    quint8 NeiHuan;
    quint8 GunDan;
};
struct FAULT_OTHER{
    quint8 TaMian;
    quint8 ChiLun;
    quint8 DaChiLun;
    quint8 XiaoChiLun;
};

struct FAULTTYPE{
    FAULT FaultBearing1 = {0,1,2,3,4};
    FAULT FaultBearing2 = {5,6,7,8,9};
    FAULT FaultBearing3 = {10,11,12,13,14};
    FAULT FaultBearing4 = {15,16,17,18,19};
    FAULT_OTHER FaultOther = {0,1,2,3};
};

struct TWOTYPEARGDOUBLE{
    float Original;
    float Demodulation;
};
struct TWOTYPEARGINT{
    int Original;
    int Demodulation;
};
typedef enum {
    ZhouXiang = 0,
    ChiLunXiang = 1,
    Dianji = 2
}POINT_TYPE;

struct PEAK{
    float value;  /* 幅值 */
    int number; /* 谱线号 */
};
extern bool FaultResult[FaultTypeNum];
extern bool FaultResult_zhou[FaultTypeNum];
extern bool FaultResult_other[FaultType_OtherNum];
class Algorithm_v2 : public QObject
{
    Q_OBJECT
public:
    explicit Algorithm_v2(QObject *parent = nullptr);

    //轴承1、2、3、4的接触角、中径、滚径、滚子数，高速轮
    static void Argument_Init(float dblAngle1, float dblMidDia1, float dblRollerDia1, int nRollerAmount1,
                         float dblAngle2, float dblMidDia2, float dblRollerDia2, int nRollerAmount2,
                         float dblAngle3, float dblMidDia3, float dblRollerDia3, int nRollerAmount3,
                         float dblAngle4, float dblMidDia4, float dblRollerDia4, int nRollerAmount4,
                         float HighShaftDiameter,float LowShaftDiameter,float DspeedRatio,int gear_big,
                         int gear_small,char pointType);
/*
 * name :Algoruthm_Start
 * Explain:开始运行算法
 * parameter:
 *      @OriginalData: 原始数据
 *      @DemodulationData: 包络数据
 *      @OriginalLen: 原始数据长度
 *      @DemodulationLen: 包络数据长度
 *      @SamplingFrequency: 采样频率
 *      @OriginalMax : 原始数据整流平均值(原为最大值,现废弃但名称未改)
 *      @OriginalAVE : 原始数据平均值
 *      @DemodulationAVE: 包络数据平均值
 * return: QStringList list 最少包含一项，即 list.at(0)
 *      list.at(0): 8种故障的诊断结果，中间用|分隔
 *                  保外|保内|外环|内环|滚单|大齿轮|小齿轮|踏面
 *                  0xFF表示无报警，0表示预警，1表示一级报警，2表示二级报警
 *      list.at(1): 故障的详细信息，格式为：轴承编号(1,2,3,4)|故障位置|报警等级|db值(5阶用,分割)
 *                  例如：1|保外|一级报警|11,22,33,44,55
 *                       0|踏面|预警|11,22,33,44,55
 *                  诊断对象为踏面或齿轮时轴承编号为0
*/
    static QStringList Algoruthm_Start(QVector<float> OriginalData, QVector<float> DemodulationData, int OriginalLen,
                                       int DemodulationLen, int SamplingFrequency, float OriginalMax, float OriginalAVE,
                                       float DemodulationAVE);

private:
    static void InitMavinSys(char pointType,float Rev);
    static void ComputeFaultFre(void);
    static void SearchMaxPoint_en(QVector<float> Data);
    static void DiscreteAnalyse(int ntype, int nRank, int nMaxPos, int FFT_RESULT_LEN, QVector<float> data, float ave, bool fault[]);
    static void SideFrenJudge_en(QVector<float> Data);//边频误用判定
    static void MavinAlert_en(float ave, quint32 Rev, float Diameter);//对轴承故障计算db

    static void SearchMaxPoint_zhou(QVector<float> Data,float ave);
    static void MavinAlert_zhou(float ave, quint32 Rev, float Diameter);

    static void SearchMaxPoint_ta(QVector<float> Data,float ave);
    static void MavinAlert_ta(float ave);

    static void SearchMaxPoint(QVector<float> Data, float ave);
    static void SideFrenJudge(QVector<float> Data,float ave);

};

#endif // ALGORITHM_V2_H
