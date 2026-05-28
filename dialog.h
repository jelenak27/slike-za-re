#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <pigpiod_if2.h>
#include "dht22.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class DhtThread : public QThread
{
    Q_OBJECT
public:
    int pi_handle;
    int gpio;
    void run() override {
        float t = 0.0f, h = 0.0f;
        int rc = dht22_read(pi_handle, gpio, &t, &h);
        if (rc == 0) emit rezultat(t, h);
    }
signals:
    void rezultat(float t, float h);
};

class Dialog : public QDialog
{
    Q_OBJECT
public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
private slots:
    void citajSenzore();
    void postaviPrag();
    void primiDht(float t, float h);
private:
    Ui::Dialog *ui;
    QTimer     *timer;
    DhtThread  *dhtThread;
    int         pi;
    float       prag;
    float       lastTemp;
    float       lastVlaz;
    int citajPIR();
    int citajLDR();
};

#endif
