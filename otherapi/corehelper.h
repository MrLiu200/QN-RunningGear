#ifndef COREHELPER_H
#define COREHELPER_H

#include <QObject>
#include <QDir>
#include <QtCore>
#include <QFileDialog>
#include <Eigen/Dense>
#include "fftw3.h"
#include "appsetting.h"

#define TIMEMS          qPrintable(QTime::currentTime().toString("HH:mm:ss zzz"))
#define TIME            qPrintable(QTime::currentTime().toString("HH:mm:ss"))
#define QDATE           qPrintable(QDate::currentDate().toString("yyyy-MM-dd"))
#define QTIME           qPrintable(QTime::currentTime().toString("HH-mm-ss"))
#define DATETIME        qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
#define DATETIMES       qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz"))
#define VIBTIME         qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz"))

#define TOUINT16(x,y) ((((quint16)x<<8) & 0xff00) | (y&0x00ff))

#define LO(x) ((quint8) ((x)&0x00ff))
#define HI(x) ((quint8) (((x)>>8)&0x00ff))

#define CAN_Default_FrameID (char)0x00
#define data_zero (char)0x00
#define Can0_Addr (char)0x00

#ifdef Q_OS_WIN
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

class CoreHelper : public QObject
{
    Q_OBJECT
public:
    explicit CoreHelper(QObject *parent = nullptr);

    //程序本身文件名称
    static QString APPName();

    //程序当前所在路径
    static QString APPPath();

    //QByteArray 转换为 Hex
    static QString byteArrayToHexStr(const QByteArray &data);

    //新建目录
    static void newDir(const QString &dirName);

    //设置系统时间
    static void SetSystemTime(int year, int mounth, int day,int hour, int minutes, int secord);

    //获取容量占比---返回当前盘符占比
    static int Getcapacity(QString DriveName);

    //删除文件夹下所有文件
    static void deleteDirectory(const QString &path);

    //重启设备
    static void Reboot(void);

    //设置延时
    static void sleep(int sec);

    //获取选择的文件
    static QString getFileName(const QString &filter, QString defaultDir = QCoreApplication::applicationDirPath());

    //获取保存的文件
    static QString getSaveName(const QString &filter, QString defaultDir = QCoreApplication::applicationDirPath());

    //文件夹是否存在
    static bool folderIsExist(const QString &strFolder);

    //文件是否存在
    static bool fileIsExist(const QString &strFile);

    //复制文件
    static bool copyFile(const QString &sourceFile, const QString &targetFile);

    //根据数据算出: 最大值|最小值|均值|方差|标准差|均方根|整流平均值|峭度|峰峰值|波形因子|峰值因子|脉冲因子|裕度因子|峭度因子
    static QVector<float> CalculateDimension(QVector <float> data);

    //挂载系统最大的盘符，该软件指外部存储硬盘
    static bool ExternalStorageInit();

    //CRC32/MPEG-2---输入输出不取反
    static quint32 GetrCRC32_MPEG_2(QString fileName);
    static quint32 GetCRC32_MPEG_2(const QByteArray array);
    //STM32H750 CRC32
    static quint32 GetCRC32_STM32H750(QString filename);

    //校验和
    static quint8 CheckSum(const uint8_t *data, int Start, int end);
    static quint8 CheckSum(const QByteArray array,int Start, int end);

    //根据传输的两个字节计算出温度,温度传感器型号为: DS18B20
    static double DS18B20ByteToTemperature(uint8_t High,uint8_t Low);

    //根据两个字节计算出温度，传感器型号为M117
    static double M117ByteToTemperature(uint8_t High,uint8_t Low);

    //傅里叶变换
    static void FFT(QVector<float> &envelope);

    //根据传入的两个值，返回报警情况，1：预警 2：一级报警 0xff：无报警
    static uint8_t ObtainTemAlarmLevel(double ambient,double channel);
    //传入诊断结果以及通道号，返回报警列表，每个元素包含 类别|等级，例如 保内|2
//    static QStringList ObtainVIBAlarmLever(int prech,QStringList alarmresult);

    //设置IO口亮灭
    static void SetLedState(QString name,uint8_t state);

    //设置IP
    static void SetNetWorkConfig(QString ip, QString gateway, QString dns, QString ethName);

signals:

};

#endif // COREHELPER_H
