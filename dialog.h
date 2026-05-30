#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTimer>
#include <pigpiod_if2.h>
#include "dht22.h"

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
    void citajPIR();
    void postaviPrag();

private:
    Ui::Dialog *ui;
    QTimer     *timer;
    QTimer     *timerPIR;
    int         pi;
    float       lastTemp;
    float       lastVlaz;
    float       prag;

    int citajLDR();
};

#endif
