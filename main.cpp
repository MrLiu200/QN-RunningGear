#include <QApplication>
#include <QMainWindow>
#include "appinit.h"
int main(int argc,char *argv[])
{
    QApplication a(argc,argv);
    APPInit::Instance()->start();
    QMainWindow windows;
    windows.show();
    return a.exec();
}
