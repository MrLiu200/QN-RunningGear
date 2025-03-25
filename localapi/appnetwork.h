#ifndef APPNETWORK_H
#define APPNETWORK_H

#include <QObject>

class AppNetwork : public QObject
{
    Q_OBJECT
public:
    static AppNetwork *Instance();
    explicit AppNetwork(QObject *parent = nullptr);

private:
    static AppNetwork *self;
signals:

};

#endif // APPNETWORK_H
