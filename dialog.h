#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTimer>
#include <pigpiod_if2.h>

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
    void citajSenzore();
    void postaviPrag();

private:
    Ui::Dialog *ui;
    QTimer *timer;
    int pi;          // pigpio handle
    float prag;      // temperatura prag

    float citajDHT22Temp();
    float citajDHT22Vlaznost();
    int citajLDR();
    int citajPIR();
};

#endif
