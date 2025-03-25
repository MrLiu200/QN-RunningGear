QT += network serialbus  core sql widgets concurrent

TARGET              = RailTrafficMonitoring_KM4
TEMPLATE            = app
MOC_DIR             = temp/moc
RCC_DIR             = temp/rcc
#UI_DIR              = temp/ui
OBJECTS_DIR         = temp/obj
DESTDIR             = bin
SOURCES             += main.cpp
include             ($$PWD/localapi/localapi.pri)
include             ($$PWD/busapi/busapi.pri)
include             ($$PWD/JudgeFault/JudgeFault.pri)
include             ($$PWD/logapi/logapi.pri)
include             ($$PWD/otherapi/otherapi.pri)
include             ($$PWD/networkapi/networkapi.pri)
include             ($$PWD/fftw/fftw.pri)


INCLUDEPATH         += $$PWD/localapi
INCLUDEPATH         += $$PWD/busapi
INCLUDEPATH         += $$PWD/JudgeFault
INCLUDEPATH         += $$PWD/logapi
INCLUDEPATH         += $$PWD/otherapi
INCLUDEPATH         += $$PWD/networkapi
INCLUDEPATH         += $$PWD/fftw
INCLUDEPATH         += $$PWD/eigen_3.4.0

#定义为分机程序
#DEFINES += DEVICE_SLAVE

RESOURCES += \
    other.qrc
win32{
    INCLUDEPATH         += $$PWD/quazip/win/include
    LIBS += -L$$PWD/quazip/win/lib/ -lquazipd
}

unix{


#    LIBS += -L$$PWD/quazip/linux/lib/ -lquazip
#    LIB += -L$$PWD/quazip/linux/lib/ -lz
#    INCLUDEPATH         += /opt/arago-2021.09-aarch64-linux-tisdk/sysroots/aarch64-linux/usr/include/QtZlib
#    LIBS += -L/opt/arago-2021.09-aarch64-linux-tisdk/sysroots/aarch64-linux/usr/lib -lz
    INCLUDEPATH         += $$PWD/quazip/linux/include
    LIBS += -L/lib -lquazip
}

HEADERS +=



