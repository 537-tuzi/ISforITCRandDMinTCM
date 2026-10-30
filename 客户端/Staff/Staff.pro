QT       += core gui widgets
TARGET    = Staff
TEMPLATE  = app
CONFIG   += c++11

# 医生端+管理员端（同一工程，手册 3.3"客户端工程2"）；util/ 与 Patient 工程同构复用
SOURCES += \
    main.cpp \
    util/winmanager.cpp \
    util/Toast.cpp \
    util/UiScale.cpp \
    util/UIFactory.cpp \
    view/LoginView.cpp \
    view/MainWindow.cpp \
    view/ApptListView.cpp \
    view/DiagnoseView.cpp \
    view/HistoryView.cpp \
    view/MySchedView.cpp \
    view/DoctorView.cpp \
    view/DeptView.cpp \
    view/SchedMgrView.cpp \
    view/StatsView.cpp \
    view/LogView.cpp \
    view/Dialogs.cpp

HEADERS += \
    util/winmanager.h \
    util/Toast.h \
    util/UiScale.h \
    util/UIFactory.h \
    util/AppDefs.h \
    util/StaffTypes.h \
    view/LoginView.h \
    view/MainWindow.h \
    view/ApptListView.h \
    view/DiagnoseView.h \
    view/HistoryView.h \
    view/MySchedView.h \
    view/DoctorView.h \
    view/DeptView.h \
    view/SchedMgrView.h \
    view/StatsView.h \
    view/LogView.h \
    view/Dialogs.h

RESOURCES += resources.qrc
