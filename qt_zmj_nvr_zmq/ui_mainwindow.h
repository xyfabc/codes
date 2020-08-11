/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QLabel *label_logo;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;
    QLabel *label_time;
    QLabel *label_title;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1323, 1000);
        MainWindow->setMinimumSize(QSize(1000, 1000));
        MainWindow->setMaximumSize(QSize(1920, 1080));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        label_logo = new QLabel(centralWidget);
        label_logo->setObjectName(QString::fromUtf8("label_logo"));
        label_logo->setGeometry(QRect(0, 0, 30, 30));
        label_logo->setStyleSheet(QString::fromUtf8("image: url(:/111.png);\n"
"background-color: rgb(0, 0, 0);"));
        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(220, 40, 80, 20));
        pushButton_2 = new QPushButton(centralWidget);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(360, 40, 80, 20));
        pushButton_3 = new QPushButton(centralWidget);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(570, 60, 80, 20));
        label_time = new QLabel(centralWidget);
        label_time->setObjectName(QString::fromUtf8("label_time"));
        label_time->setGeometry(QRect(720, 20, 221, 30));
        label_time->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        label_title = new QLabel(centralWidget);
        label_title->setObjectName(QString::fromUtf8("label_title"));
        label_title->setGeometry(QRect(180, 230, 501, 30));
        label_title->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        pushButton_4 = new QPushButton(centralWidget);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));
        pushButton_4->setGeometry(QRect(100, 120, 89, 24));
        pushButton_5 = new QPushButton(centralWidget);
        pushButton_5->setObjectName(QString::fromUtf8("pushButton_5"));
        pushButton_5->setGeometry(QRect(270, 120, 89, 24));
        MainWindow->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        label_logo->setText(QApplication::translate("MainWindow", "<html><head/><body><p align=\"center\"><br/></p></body></html>", nullptr));
        pushButton->setText(QApplication::translate("MainWindow", "1\350\267\257", nullptr));
        pushButton_2->setText(QApplication::translate("MainWindow", "4\350\267\257", nullptr));
        pushButton_3->setText(QApplication::translate("MainWindow", "16\350\267\257", nullptr));
        label_time->setText(QApplication::translate("MainWindow", "<html><head/><body><p align=\"right\"><span style=\" font-size:12pt; color:#ffffff;\">2019-12-12 13:56:00</span></p></body></html>", nullptr));
        label_title->setText(QApplication::translate("MainWindow", "<html><head/><body><p><span style=\" font-size:22pt; font-weight:600; color:#ffffff;\">\351\203\221\345\267\236\347\205\244\347\237\277\346\234\272\346\242\260\351\233\206\345\233\242\350\202\241\344\273\275\346\234\211\351\231\220</span></p></body></html>", nullptr));
        pushButton_4->setText(QApplication::translate("MainWindow", "close", nullptr));
        pushButton_5->setText(QApplication::translate("MainWindow", "open", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
