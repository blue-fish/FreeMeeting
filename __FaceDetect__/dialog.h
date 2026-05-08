#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include"capturesample.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_pb_start_clicked();

    void on_pb_close_clicked();

    void on_pb_quit_clicked();

    void slot_getOneImage(QImage& img);

private:
    Ui::Dialog *ui;

    CaptureSample* m_captureSample;
};
#endif // DIALOG_H
