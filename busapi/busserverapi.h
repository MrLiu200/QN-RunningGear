#ifndef BUSSERVERAPI_H
#define BUSSERVERAPI_H

#include <QObject>
#include "bus0api.h"
#include "bus1api.h"

typedef struct UPDATESTRUCT{
    QString UpdateFile;
    uint32_t Updatecrc;
    int filesize;
    QList<uint8_t> bus0_pre;
    QList<uint8_t> bus1_pre;
    uint8_t bus0_flag;//0:ok 1:wait 2:fail
    uint8_t bus1_flag;
}UPDATESTRUCT;

enum RUNMODE{
    LowPower = 0,
    Temperature,
    Comprehensive,
    InitMode
};

class BusServerAPI : public QObject
{
    Q_OBJECT
public:
    static BusServerAPI *Instance();
    explicit BusServerAPI(QObject *parent = nullptr);

    void Init();
private:
    static BusServerAPI *self;
    QTimer *timerTask;
    struct UPDATESTRUCT m_updatestruct;
    QTimer *timerUpdate;
    RUNMODE currentRunMode;
    //主任务
    void TaskMain();

    //获取当前时速
    double GetCurrentSpeed(void);

    //重置任务状态
    void ResetState_Task(void);

    //低功耗模式
    void LowPower_Task(void);

    // 温度巡检任务
    void TmpInspection_Task(void);

    // 综合巡检任务
    void CmpInspection_Task(void);

    //更新报警状态
    void UpdateAlarmState(uint8_t preid, uint8_t prech,uint8_t alarmgrade);

    //更新任务
    void TaskUpdate();

    //Bus0 温度巡检
    void Bus0_TmpTask();

    //Bus1 温度巡检
    void Bus1_TmpTask();

    //Bus0 综合巡检
    void Bus0_CmpTask();

    //Bus1 综合巡检
    void Bus1_CmpTask();

    void SetAlarmState(uint8_t level);

private slots:
    //自检数据
    void SelfInspection(PRE_SELF_INSPECTION *preselfinspection);

    //温度巡检
    void TemInspection(PRE_TEM_VALUE *tem_value);

    //综合巡检
    void ReturnVIBData(PRE_VIBRATION_DATA * vibration_data);

    //返回空闲状态
    void PreState(uint8_t preid,bool Isbusy);

    //升级结果
    void UpdateResults(uint8_t preid,bool IsFail);

    //重启结果
    void RebootState(uint8_t preid,bool IsAbnormal);

    //单通道状态返回
    void ReturnChannelStatus(uint8_t preid, uint8_t ch, bool IsAbnormal);

    //自检任务
    void SelfInspection_Task(void);

    //开启服务
    void StartServer();

    //暂停服务
    void StopServer();

    //前置处理器更新
    void PreSoftwareUpdate(const QString filename,const uint32_t crc,uint8_t deviceID);

    //上位机指定自检
    void Slot_SelfInspection(uint8_t pre_id);
signals:

};

#endif // BUSSERVERAPI_H
