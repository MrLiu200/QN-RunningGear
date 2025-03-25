/*
 * 类名:线程压缩文件类
 * 作者:Mr.Liu
 * 时间:2024-05-20
 * 说明:
 * 1、通过线程完成打包或压缩任务，不阻塞主界面
 * 2、完成压缩任务后自动删除源数据
 * 弊端：外部存储设备格式限制：ntfs和vfat。
 *      格式化外部只可以为 vfat或exfat
*/

#ifndef COMPRESSTHREAD_H
#define COMPRESSTHREAD_H

#include <QObject>
#include <QThread>
#include <QProcess>
#include "corehelper.h"

class CompressThread : public QThread
{
    Q_OBJECT
public:
    explicit CompressThread(QObject *parent = nullptr);

protected:
    void run() override;

private:
    QString filename;
    QString TarPath;
    QString path1;
    QString path2;
    QStringList pathlist;

public:
    void SetFileName(QString name);
    void SetVibpath(QString path);
    void SetTarpath(QString path);
    void Setclientpath(QString path);
    void SetPathlist(QStringList list);
signals:

};

#endif // COMPRESSTHREAD_H
