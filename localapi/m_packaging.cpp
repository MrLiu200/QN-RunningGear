#include "m_packaging.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include "quazip.h"
#include "quazipfile.h"
#include "JlCompress.h"
M_Packaging::M_Packaging(QString SourceDir, QString TargetFile, QString filterDate,QObject *parent) : QThread(parent),
    Sdir(SourceDir),
    Tfilename(TargetFile),
    PackagingDate(filterDate)
{
    Stopped = false;
    FileList.clear();
}

void M_Packaging::Stop()
{
    Stopped = true;
}

void M_Packaging::run()
{
    FindFile();
    // 检查是否找到任何符合条件的文件
    if (FileList.isEmpty()) {
        qDebug() << "没有找到符合条件的文件！";
//        Stopped = true;
//        qDebug() << "打包完成，M_Packaging 线程停止。";
//        deleteLater();
        Q_EMIT PackagingFinish(Tfilename);
        return;
    }

    // 判断目标文件是否存在，如果存在先删除
    if(QFile::exists(Tfilename)){
        QFile::remove(Tfilename);
    }

    // 创建 QuaZip 对象，表示目标压缩包
    QuaZip zip(Tfilename);

    // 尝试打开压缩包，如果打开失败返回 false
    if (!zip.open(QuaZip::mdCreate)) {
        qDebug() << "无法打开压缩包：" << Tfilename;
        Q_EMIT PackagingError(QString("无法打开压缩包:%1").arg(Tfilename));
        return;
    }
//    qDebug()<<"打包开始";
    // 遍历文件列表，将每个文件添加到压缩包
    for (const QString &file : FileList) {
        if (Stopped) break; // 如果线程需要停止，退出循环

        QFile inputFile(file);
        if (!inputFile.open(QIODevice::ReadOnly)) {
//            qDebug() << "无法打开文件：" << file;
            continue; // 继续处理下一个文件
        }

        QString fileName = QFileInfo(file).fileName();
        // 创建一个新的压缩包文件
        QuaZipFile zipFile(&zip);

        // 打开压缩包文件进行写入
        if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileName, file)))
        {
//            qDebug() << "Failed to add file to ZIP:" << file;
            continue; // 继续处理下一个文件
        }

        // 读取文件数据并写入到压缩包
        QByteArray fileData = inputFile.readAll();
        zipFile.write(fileData);

        // 关闭压缩包文件
        zipFile.close();
        inputFile.close();
        inputFile.remove();
    }

    // 关闭压缩包
    zip.close();

    // 完成后停止线程
    Stopped = true;
    qDebug() << "打包完成，M_Packaging 线程停止。";
//    deleteLater();
    Q_EMIT PackagingFinish(Tfilename);
}

void M_Packaging::FindFile()
{
    if(Sdir.isEmpty() || Tfilename.isEmpty()) return;
    //PackagingDate 格式 yyyy-MM-dd 文件存储格式:*yyyyMMdd*.bin
    QString filterDate = QDate::fromString(PackagingDate,"yyyy-MM-dd").toString("yyyyMMdd");
    QDir dir(Sdir);
    QStringList filters;
    QString filter = QString("*%1*.bin").arg(filterDate);
    filters << filter;
    qDebug()<<"开始寻找符合的文件";
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    QFileInfoList files = dir.entryInfoList();
    qDebug()<<"符合的文件数量为：" << files.size();
    foreach (const QFileInfo &fileInfo, files){
        QString fileName = fileInfo.absoluteFilePath();
        FileList.append(fileName);
    }
}
