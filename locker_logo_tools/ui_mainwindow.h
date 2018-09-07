/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created: Fri Mar 30 13:40:12 2018
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QStatusBar>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QLabel *label;
    QListWidget *listWidget;
    QToolButton *UPtoolButton;
    QToolButton *DowntoolButton;
    QLabel *label_7;
    QLabel *label_8;
    QFrame *line;
    QWidget *layoutWidget;
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLineEdit *x_pos_lineEdit;
    QFrame *line_2;
    QSpacerItem *horizontalSpacer_2;
    QLineEdit *y_pos_lineEdit_2;
    QLabel *label_3;
    QLineEdit *w_len_lineEdit_3;
    QLabel *label_4;
    QLineEdit *h_len_lineEdit_4;
    QLabel *label_5;
    QLineEdit *time_lineEdit_5;
    QLabel *label_6;
    QPushButton *pushButton_3;
    QPushButton *pushButton;
    QPushButton *pushButton_4;
    QPushButton *build_pushButton;
    QPushButton *pushButton_2;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(493, 524);
        MainWindow->setMinimumSize(QSize(493, 524));
        MainWindow->setMaximumSize(QSize(493, 524));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 30, 240, 320));
        label->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        listWidget = new QListWidget(centralWidget);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setGeometry(QRect(260, 30, 161, 320));
        UPtoolButton = new QToolButton(centralWidget);
        UPtoolButton->setObjectName(QString::fromUtf8("UPtoolButton"));
        UPtoolButton->setGeometry(QRect(430, 140, 37, 31));
        DowntoolButton = new QToolButton(centralWidget);
        DowntoolButton->setObjectName(QString::fromUtf8("DowntoolButton"));
        DowntoolButton->setGeometry(QRect(430, 190, 37, 31));
        label_7 = new QLabel(centralWidget);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(70, 10, 131, 16));
        label_8 = new QLabel(centralWidget);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(310, 10, 54, 12));
        line = new QFrame(centralWidget);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(10, 360, 471, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layoutWidget = new QWidget(centralWidget);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(6, 381, 471, 111));
        gridLayout = new QGridLayout(layoutWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(layoutWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        x_pos_lineEdit = new QLineEdit(layoutWidget);
        x_pos_lineEdit->setObjectName(QString::fromUtf8("x_pos_lineEdit"));

        gridLayout->addWidget(x_pos_lineEdit, 1, 1, 1, 1);

        line_2 = new QFrame(layoutWidget);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line_2, 2, 0, 1, 10);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 1, 9, 1, 1);

        y_pos_lineEdit_2 = new QLineEdit(layoutWidget);
        y_pos_lineEdit_2->setObjectName(QString::fromUtf8("y_pos_lineEdit_2"));

        gridLayout->addWidget(y_pos_lineEdit_2, 1, 4, 1, 1);

        label_3 = new QLabel(layoutWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 1, 2, 1, 2);

        w_len_lineEdit_3 = new QLineEdit(layoutWidget);
        w_len_lineEdit_3->setObjectName(QString::fromUtf8("w_len_lineEdit_3"));

        gridLayout->addWidget(w_len_lineEdit_3, 0, 7, 1, 3);

        label_4 = new QLabel(layoutWidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 0, 5, 1, 2);

        h_len_lineEdit_4 = new QLineEdit(layoutWidget);
        h_len_lineEdit_4->setObjectName(QString::fromUtf8("h_len_lineEdit_4"));

        gridLayout->addWidget(h_len_lineEdit_4, 0, 4, 1, 1);

        label_5 = new QLabel(layoutWidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 0, 2, 1, 1);

        time_lineEdit_5 = new QLineEdit(layoutWidget);
        time_lineEdit_5->setObjectName(QString::fromUtf8("time_lineEdit_5"));

        gridLayout->addWidget(time_lineEdit_5, 0, 1, 1, 1);

        label_6 = new QLabel(layoutWidget);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        gridLayout->addWidget(label_6, 0, 0, 1, 1);

        pushButton_3 = new QPushButton(layoutWidget);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));

        gridLayout->addWidget(pushButton_3, 3, 0, 1, 1);

        pushButton = new QPushButton(layoutWidget);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        gridLayout->addWidget(pushButton, 3, 1, 1, 2);

        pushButton_4 = new QPushButton(layoutWidget);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));

        gridLayout->addWidget(pushButton_4, 3, 3, 1, 2);

        build_pushButton = new QPushButton(layoutWidget);
        build_pushButton->setObjectName(QString::fromUtf8("build_pushButton"));

        gridLayout->addWidget(build_pushButton, 3, 5, 1, 3);

        pushButton_2 = new QPushButton(layoutWidget);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));

        gridLayout->addWidget(pushButton_2, 3, 8, 1, 2);

        MainWindow->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "\346\261\211\347\216\213\346\231\272\350\277\234\344\272\272\350\204\270\351\224\201LOGO\347\224\237\346\210\220\345\267\245\345\205\267", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "LOGO", 0, QApplication::UnicodeUTF8));
        UPtoolButton->setText(QApplication::translate("MainWindow", "\345\220\221\344\270\212", 0, QApplication::UnicodeUTF8));
        DowntoolButton->setText(QApplication::translate("MainWindow", "\345\220\221\344\270\213", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("MainWindow", "\344\272\272\350\204\270\351\224\201\345\261\217\345\271\225\357\274\210320x240\357\274\211", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("MainWindow", "\345\212\250\347\224\273\345\210\227\350\241\250", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "\345\212\250\347\224\273\346\250\252\345\235\220\346\240\207\357\274\232", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("MainWindow", "\345\212\250\347\224\273\347\272\265\345\235\220\346\240\207\357\274\232", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("MainWindow", "\345\212\250\347\224\273\345\256\275\345\272\246\357\274\232", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("MainWindow", "\345\212\250\347\224\273\351\225\277\345\272\246\357\274\232", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("MainWindow", "\345\212\250\347\224\273\351\227\264\351\232\224\357\274\232", 0, QApplication::UnicodeUTF8));
        pushButton_3->setText(QApplication::translate("MainWindow", "\346\267\273\345\212\240\350\203\214\346\231\257", 0, QApplication::UnicodeUTF8));
        pushButton->setText(QApplication::translate("MainWindow", "\346\267\273\345\212\240\345\212\250\347\224\273", 0, QApplication::UnicodeUTF8));
        pushButton_4->setText(QApplication::translate("MainWindow", "\345\210\240\351\231\244\345\267\262\351\200\211\345\212\250\347\224\273", 0, QApplication::UnicodeUTF8));
        build_pushButton->setText(QApplication::translate("MainWindow", "\345\274\200\345\247\213\346\274\224\347\244\272", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("MainWindow", "\347\224\237\346\210\220\346\226\207\344\273\266", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
