QT += widgets multimedia

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 头文件搜索路径
INCLUDEPATH += $$PWD/sherpa/c-api
# 链接预编译lib库
LIBS += -L$$PWD/sherpa/lib -lsherpa-onnx-c-api

# Windows自动复制dll到exe输出目录，运行不会报缺失dll
win32 {
    QMAKE\_POST\_LINK += copy $$PWD/sherpa/bin/\*.dll $$OUT\_PWD
}

SOURCES += \
    audiorecorder.cpp \
    main.cpp \
    mainwindow.cpp \
    sensevoiceengine.cpp

HEADERS += \
    audiorecorder.h \
    mainwindow.h \
    sensevoiceengine.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 将项目源码根目录路径传递给C++代码
DEFINES += PROJECT_SOURCE_DIR=\\\"$$PWD\\\"