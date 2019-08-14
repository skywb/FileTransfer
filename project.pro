#-------------------------------------------------
#
# Project created by QtCreator 2019-08-06T14:33:40
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = project
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        recv/fileRecv.cpp \
        send/File.cpp \
        send/FileSendControl.cpp \
        send/fileSend.cpp \
        util/Connecter.cpp \
        util/multicastUtil.cpp \
        util/testutil.cpp \
        util/zip.cpp

HEADERS += \
        mainwindow.h \
        recv/fileRecv.h \
        send/File.h \
        send/FileSendControl.h \
        send/fileSend.h \
        util/Connecter.h \
        util/multicastUtil.h \
        util/testutil.h \
        util/zip.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/release/ -lrecv
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/debug/ -lrecv
else:unix: LIBS += -L$$PWD/../FileTranslate/lib/ -lrecv

INCLUDEPATH += $$PWD/../FileTranslate
DEPENDPATH += $$PWD/../FileTranslate

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/librecv.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/librecv.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/recv.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/recv.lib
else:unix: PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/librecv.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/release/ -lsend
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/debug/ -lsend
else:unix: LIBS += -L$$PWD/../FileTranslate/lib/ -lsend

INCLUDEPATH += $$PWD/../FileTranslate
DEPENDPATH += $$PWD/../FileTranslate

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/libsend.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/libsend.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/send.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/send.lib
else:unix: PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/libsend.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/release/ -lsend
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/debug/ -lsend
else:unix: LIBS += -L$$PWD/../FileTranslate/lib/ -lsend

INCLUDEPATH += $$PWD/../FileTranslate
DEPENDPATH += $$PWD/../FileTranslate

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/libsend.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/libsend.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/send.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/send.lib
else:unix: PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/libsend.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/release/ -lutil
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../FileTranslate/lib/debug/ -lutil
else:unix: LIBS += -L$$PWD/../FileTranslate/lib/ -lutil

INCLUDEPATH += $$PWD/../FileTranslate
DEPENDPATH += $$PWD/../FileTranslate

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/libutil.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/libutil.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/release/util.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/debug/util.lib
else:unix: PRE_TARGETDEPS += $$PWD/../FileTranslate/lib/libutil.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./release/ -lrecv
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./debug/ -lrecv
else:unix: LIBS += -L$$PWD/./ -lrecv

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./release/librecv.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./debug/librecv.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./release/recv.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./debug/recv.lib
else:unix: PRE_TARGETDEPS += $$PWD/./librecv.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./release/ -lsend
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./debug/ -lsend
else:unix: LIBS += -L$$PWD/./ -lsend

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./release/libsend.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./debug/libsend.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./release/send.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./debug/send.lib
else:unix: PRE_TARGETDEPS += $$PWD/./libsend.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./release/ -lutil
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./debug/ -lutil
else:unix: LIBS += -L$$PWD/./ -lutil

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./release/libutil.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./debug/libutil.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./release/util.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./debug/util.lib
else:unix: PRE_TARGETDEPS += $$PWD/./libutil.a

unix|win32: LIBS += -lzip
