/********************************************************************************
** Form generated from reading UI file 'lcdform.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LCDFORM_H
#define UI_LCDFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LCDForm
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QPushButton *pushButton;

    void setupUi(QWidget *LCDForm)
    {
        if (LCDForm->objectName().isEmpty())
            LCDForm->setObjectName(QString::fromUtf8("LCDForm"));
        LCDForm->resize(800, 500);
        gridLayout = new QGridLayout(LCDForm);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(LCDForm);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        pushButton = new QPushButton(LCDForm);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        gridLayout->addWidget(pushButton, 1, 0, 1, 1);


        retranslateUi(LCDForm);

        QMetaObject::connectSlotsByName(LCDForm);
    } // setupUi

    void retranslateUi(QWidget *LCDForm)
    {
        LCDForm->setWindowTitle(QApplication::translate("LCDForm", "Form", nullptr));
        label->setText(QApplication::translate("LCDForm", "TextLabel-zmj", nullptr));
        pushButton->setText(QApplication::translate("LCDForm", "close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LCDForm: public Ui_LCDForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LCDFORM_H
