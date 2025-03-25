#ifndef APPINIT_H
#define APPINIT_H

#include <QObject>
#include "appsetting.h"
#include "busserverapi.h"
#include "taskapi.h"
class APPInit : public QObject
{
    Q_OBJECT
public:
    static APPInit *Instance();
    explicit APPInit(QObject *parent = nullptr);
    void start(void);

private:
    static APPInit *self;
    void RefreshAlgorithmLimit();
    void NetCheck();
signals:

};

#endif // APPINIT_H
