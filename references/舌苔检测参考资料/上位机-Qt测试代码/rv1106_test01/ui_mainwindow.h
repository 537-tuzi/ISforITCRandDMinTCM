/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *layoutWidget;
    QGridLayout *gridLayout_4;
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer;
    QLabel *label_2;
    QComboBox *combo_baud;
    QPushButton *btn_serial;
    QLabel *label_serial_status;
    QLabel *label_5;
    QLabel *label;
    QComboBox *combo_port;
    QPushButton *btn_serial_refresh;
    QGridLayout *gridLayout_2;
    QPushButton *btn_net;
    QLineEdit *edit_port;
    QLabel *label_net_status;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_6;
    QLabel *label_4;
    QLabel *label_3;
    QLineEdit *edit_ip;
    QGridLayout *gridLayout_3;
    QTextEdit *text_result;
    QPushButton *btn_action;
    QLabel *label_video;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        layoutWidget = new QWidget(centralwidget);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(50, 30, 671, 511));
        gridLayout_4 = new QGridLayout(layoutWidget);
        gridLayout_4->setObjectName("gridLayout_4");
        gridLayout_4->setContentsMargins(0, 0, 0, 0);
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 6, 1, 1);

        label_2 = new QLabel(layoutWidget);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 0, 2, 1, 1);

        combo_baud = new QComboBox(layoutWidget);
        combo_baud->addItem(QString());
        combo_baud->addItem(QString());
        combo_baud->setObjectName("combo_baud");

        gridLayout->addWidget(combo_baud, 0, 3, 1, 1);

        btn_serial = new QPushButton(layoutWidget);
        btn_serial->setObjectName("btn_serial");

        gridLayout->addWidget(btn_serial, 0, 5, 1, 1);

        label_serial_status = new QLabel(layoutWidget);
        label_serial_status->setObjectName("label_serial_status");

        gridLayout->addWidget(label_serial_status, 0, 8, 1, 1);

        label_5 = new QLabel(layoutWidget);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 0, 7, 1, 1);

        label = new QLabel(layoutWidget);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        combo_port = new QComboBox(layoutWidget);
        combo_port->setObjectName("combo_port");

        gridLayout->addWidget(combo_port, 0, 1, 1, 1);

        btn_serial_refresh = new QPushButton(layoutWidget);
        btn_serial_refresh->setObjectName("btn_serial_refresh");

        gridLayout->addWidget(btn_serial_refresh, 0, 4, 1, 1);

        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 5);
        gridLayout->setColumnStretch(2, 1);
        gridLayout->setColumnStretch(3, 1);
        gridLayout->setColumnStretch(4, 2);
        gridLayout->setColumnStretch(5, 2);
        gridLayout->setColumnStretch(6, 2);
        gridLayout->setColumnStretch(7, 3);
        gridLayout->setColumnStretch(8, 4);

        gridLayout_4->addLayout(gridLayout, 0, 0, 1, 1);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        btn_net = new QPushButton(layoutWidget);
        btn_net->setObjectName("btn_net");

        gridLayout_2->addWidget(btn_net, 0, 4, 1, 1);

        edit_port = new QLineEdit(layoutWidget);
        edit_port->setObjectName("edit_port");

        gridLayout_2->addWidget(edit_port, 0, 3, 1, 1);

        label_net_status = new QLabel(layoutWidget);
        label_net_status->setObjectName("label_net_status");

        gridLayout_2->addWidget(label_net_status, 0, 7, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_2, 0, 5, 1, 1);

        label_6 = new QLabel(layoutWidget);
        label_6->setObjectName("label_6");

        gridLayout_2->addWidget(label_6, 0, 6, 1, 1);

        label_4 = new QLabel(layoutWidget);
        label_4->setObjectName("label_4");

        gridLayout_2->addWidget(label_4, 0, 2, 1, 1);

        label_3 = new QLabel(layoutWidget);
        label_3->setObjectName("label_3");

        gridLayout_2->addWidget(label_3, 0, 0, 1, 1);

        edit_ip = new QLineEdit(layoutWidget);
        edit_ip->setObjectName("edit_ip");

        gridLayout_2->addWidget(edit_ip, 0, 1, 1, 1);

        gridLayout_2->setColumnStretch(0, 1);
        gridLayout_2->setColumnStretch(1, 3);
        gridLayout_2->setColumnStretch(2, 1);
        gridLayout_2->setColumnStretch(3, 2);
        gridLayout_2->setColumnStretch(4, 2);
        gridLayout_2->setColumnStretch(5, 2);
        gridLayout_2->setColumnStretch(6, 2);
        gridLayout_2->setColumnStretch(7, 4);

        gridLayout_4->addLayout(gridLayout_2, 1, 0, 1, 1);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        text_result = new QTextEdit(layoutWidget);
        text_result->setObjectName("text_result");

        gridLayout_3->addWidget(text_result, 0, 1, 1, 1);

        btn_action = new QPushButton(layoutWidget);
        btn_action->setObjectName("btn_action");
        btn_action->setMaximumSize(QSize(100, 16777215));

        gridLayout_3->addWidget(btn_action, 0, 0, 1, 1);

        gridLayout_3->setColumnStretch(0, 1);

        gridLayout_4->addLayout(gridLayout_3, 2, 0, 1, 1);

        label_video = new QLabel(layoutWidget);
        label_video->setObjectName("label_video");

        gridLayout_4->addWidget(label_video, 3, 0, 1, 1);

        gridLayout_4->setRowStretch(0, 1);
        gridLayout_4->setRowStretch(1, 1);
        gridLayout_4->setRowStretch(2, 3);
        gridLayout_4->setRowStretch(3, 24);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 20));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "\346\263\242\347\211\271\347\216\207", nullptr));
        combo_baud->setItemText(0, QCoreApplication::translate("MainWindow", "115200", nullptr));
        combo_baud->setItemText(1, QCoreApplication::translate("MainWindow", "9600", nullptr));

        btn_serial->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245\344\270\262\345\217\243", nullptr));
        label_serial_status->setText(QCoreApplication::translate("MainWindow", "\346\234\252\350\277\236\346\216\245", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "\344\270\262\345\217\243\350\277\236\346\216\245\347\212\266\346\200\201\357\274\232", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\344\270\262\345\217\243", nullptr));
        btn_serial_refresh->setText(QCoreApplication::translate("MainWindow", "\345\210\267\346\226\260\344\270\262\345\217\243", nullptr));
        btn_net->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245\347\275\221\347\273\234", nullptr));
        edit_port->setText(QCoreApplication::translate("MainWindow", "6868", nullptr));
        label_net_status->setText(QCoreApplication::translate("MainWindow", "\346\234\252\350\277\236\346\216\245", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "\347\275\221\347\273\234\350\277\236\346\216\245\347\212\266\346\200\201\357\274\232", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "\347\253\257\345\217\243\345\217\267", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "\346\221\204\345\203\217\345\244\264IP\345\234\260\345\235\200", nullptr));
        edit_ip->setText(QCoreApplication::translate("MainWindow", "10.1.1.144", nullptr));
        btn_action->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213\346\216\250\347\220\206", nullptr));
        label_video->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
