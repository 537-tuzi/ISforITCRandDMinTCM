QT       += core gui widgets
TARGET    = Patient
TEMPLATE  = app
CONFIG   += c++11

# 语音播报（TTS）：需要 Qt TextToSpeech 模块时取消下面两行注释并确认模块已安装
#QT       += texttospeech
#DEFINES  += PATIENT_TTS

# util/：工具类与共享组件（窗口管理/提示条/缩放/工厂/统一定义）
SOURCES += \
    main.cpp \
    util/winmanager.cpp \
    util/Toast.cpp \
    util/UiScale.cpp \
    util/UIFactory.cpp \
    view/InfoPanel.cpp \
    view/LoginView.cpp \
    view/MainWindow.cpp \
    view/HomeView.cpp \
    view/ProfileView.cpp \
    view/DetectView.cpp \
    view/RecordView.cpp \
    view/Dialogs.cpp

HEADERS += \
    util/winmanager.h \
    util/Toast.h \
    util/UiScale.h \
    util/UIFactory.h \
    util/AppDefs.h \
    util/PatientTypes.h \
    util/Speak.h \
    view/InfoPanel.h \
    view/LoginView.h \
    view/MainWindow.h \
    view/HomeView.h \
    view/ProfileView.h \
    view/DetectView.h \
    view/RecordView.h \
    view/Dialogs.h

RESOURCES += resources.qrc
