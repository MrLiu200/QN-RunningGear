INCLUDEPATH += $$PWD/include

win32{
LIBS += $$PWD/winlib/libfftw3-3.lib \
        $$PWD/winlib/libfftw3f-3.lib \
        $$PWD/winlib/libfftw3l-3.lib
}
unix{
LIBS += $$PWD/linuxlib/libfftw3.so.3.6.10
}
