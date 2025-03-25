HEADERS += \
    $$PWD/tcpreceivefilethread.h \
    $$PWD/tcpsendfilethread.h \ \
    $$PWD/udpmulticastapi.h \
    $$PWD/udpslotapi.h \
    $$PWD/uppercomputer.h

SOURCES += \
    $$PWD/tcpreceivefilethread.cpp \
    $$PWD/tcpsendfilethread.cpp \
    $$PWD/udpmulticastapi.cpp \
    $$PWD/udpslotapi.cpp \
    $$PWD/uppercomputer.cpp

#contains(DEFINES, DEVICE_SLAVE){
#HEADERS += $$PWD/udpmulticastclient.h
#SOURCES += $$PWD/udpmulticastclient.cpp
#}else{
#HEADERS += $$PWD/udpmulticastserver.h
#SOURCES += $$PWD/udpmulticastserver.cpp
#}
