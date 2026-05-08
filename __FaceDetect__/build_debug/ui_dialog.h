/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QLabel *lb_video;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *pb_start;
    QPushButton *pb_close;
    QPushButton *pb_quit;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(800, 664);
        lb_video = new QLabel(Dialog);
        lb_video->setObjectName(QString::fromUtf8("lb_video"));
        lb_video->setGeometry(QRect(0, 10, 800, 600));
        lb_video->setAlignment(Qt::AlignCenter);
        layoutWidget = new QWidget(Dialog);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(0, 620, 801, 41));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        pb_start = new QPushButton(layoutWidget);
        pb_start->setObjectName(QString::fromUtf8("pb_start"));

        horizontalLayout->addWidget(pb_start);

        pb_close = new QPushButton(layoutWidget);
        pb_close->setObjectName(QString::fromUtf8("pb_close"));

        horizontalLayout->addWidget(pb_close);

        pb_quit = new QPushButton(layoutWidget);
        pb_quit->setObjectName(QString::fromUtf8("pb_quit"));

        horizontalLayout->addWidget(pb_quit);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        retranslateUi(Dialog);

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QApplication::translate("Dialog", "Dialog", nullptr));
        lb_video->setText(QString());
        pb_start->setText(QApplication::translate("Dialog", "\345\274\200\345\220\257", nullptr));
        pb_close->setText(QApplication::translate("Dialog", "\345\205\263\351\227\255", nullptr));
        pb_quit->setText(QApplication::translate("Dialog", "\351\200\200\345\207\272", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOG_H
