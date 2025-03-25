#ifndef TCPSENDFILETHREAD_H
#define TCPSENDFILETHREAD_H

#include <QObject>
#include <QThread>
#include <QtNetwork>
#include <QTimer>


class TcpSendFileThread : public QThread
{
    Q_OBJECT
public:
    TcpSendFileThread(QObject *parent = 0);

protected:
    void run() override;

private:
    QTcpSocket *Socket;
    QString ServerIp;
    int ServerPort;
#ifdef DEVICE_SLAVE
    QTcpSocket *SocketStandby;
    QString ServerIpStandby;
    int ServerPortStandby;
#endif
    QTimer *timeSendVibData;
    bool NeedSend;
    bool stopped;
    QString Filename;
    void InitParameter(void);
    void Thread_Stop(void);

private slots:
    void FindSendFile(void);
    void disConnect(void);
    void SendFile(void);
    void SendError(QString name);
signals:
    void error(QString fileName);

};

#endif // TCPSENDFILETHREAD_H
