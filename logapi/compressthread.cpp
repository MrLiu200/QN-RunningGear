#include "compressthread.h"

CompressThread::CompressThread(QObject *parent) : QThread(parent)
{

}

void CompressThread::run()
{
#if 1
    QProcess process;

//    qDebug()<<"Start packaging"<< filename;
    process.start("tar" , QStringList() << "-jcvf" << filename << "-C" << TarPath << ".");
    process.waitForFinished(-1);

    QString str;
    for(int i=0;i<pathlist.size();i++){
        str = pathlist.at(i) + "/*.bin";
//        qDebug()<<" Preparing to delete :" << str;
        process.start("rm",QStringList() << str);
        process.waitForFinished(-1);
    }
#else
    QProcess *process = new QProcess(this);

    qDebug()<<"Start packaging"<< filename;
    process->start("tar" , QStringList() << "-jcvf" << filename << "-C" << TarPath << ".");
    process->waitForFinished(-1);

    QString str;
    for(int i=0;i<pathlist.size();i++){
//        qDebug()<<" Preparing to delete :" << pathlist.at(i);
        str = pathlist.at(i) + "/*.bin";
         qDebug()<<" Preparing to delete :" << str;
        process->start("rm",QStringList() << str);
        process->waitForFinished(-1);
    }
    process->deleteLater();
#endif

//    qDebug()<<"CompressThread quit";


}

void CompressThread::SetFileName(QString name)
{
    filename = name;
}

void CompressThread::SetVibpath(QString path)
{
    path1 = path;
}

void CompressThread::SetTarpath(QString path)
{
    TarPath = path;
}

void CompressThread::Setclientpath(QString path)
{
    path2 = path;
}

void CompressThread::SetPathlist(QStringList list)
{
    pathlist = list;
}
