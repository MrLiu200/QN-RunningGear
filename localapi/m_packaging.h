#ifndef M_PACKAGING_H
#define M_PACKAGING_H

#include <QThread>
#include <QDate>
class M_Packaging : public QThread
{
    Q_OBJECT
public:
    M_Packaging(QString SourceDir,QString TargetFile, QString filterDate,QObject *parent = nullptr);

    void Stop();
protected:
    void run() override;
private:
    bool Stopped;

    QString Sdir;
    QString Tfilename;
    QString PackagingDate;

    void FindFile();
    QStringList FileList;

signals:
    void PackagingError(QString ErrorStr);
    void PackagingFinish(QString Tfilename);

};

#endif // M_PACKAGING_H
