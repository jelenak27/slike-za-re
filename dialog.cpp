#include "dialog.h"
#include "ui_dialog.h"
#include <pigpiod_if2.h>

#define PIN_DHT22        4
#define PIN_PIR         17
#define PIN_LED_TEMP    27
#define PIN_LED_KRETANJE 22
#define I2C_BUS          1
#define PCF8591       0x48

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , prag(25.0)
{
    ui->setupUi(this);

    pi = pigpio_start(nullptr, nullptr);

    set_mode(pi, PIN_PIR, PI_INPUT);
    set_mode(pi, PIN_LED_TEMP, PI_OUTPUT);
    set_mode(pi, PIN_LED_KRETANJE, PI_OUTPUT);
    gpio_write(pi, PIN_LED_TEMP, 0);
    gpio_write(pi, PIN_LED_KRETANJE, 0);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Dialog::citajSenzore);
    timer->start(2000);

    connect(ui->pushButtonPostavi, &QPushButton::clicked,
            this, &Dialog::postaviPrag);

    ui->labelKretanje->setPixmap(
        QPixmap(":/kretanje_ne.png").scaled(160, 35,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelStatus->setPixmap(
        QPixmap(":/status_ispod.png").scaled(160, 35,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelLedTemp->setPixmap(
        QPixmap(":/led_temp_off.png").scaled(120, 35,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelLedKretanje->setPixmap(
        QPixmap(":/led_kretanje_off.png").scaled(120, 35,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

Dialog::~Dialog()
{
    gpio_write(pi, PIN_LED_TEMP, 0);
    gpio_write(pi, PIN_LED_KRETANJE, 0);
    pigpio_stop(pi);
    delete ui;
}

void Dialog::citajSenzore()
{
    float temp = citajDHT22Temp();
    float vlaz = citajDHT22Vlaznost();
    int svetlo = citajLDR();
    int kretanje = citajPIR();

    ui->lcdTemp->display(temp);
    ui->lcdVlaznost->display(vlaz);
    ui->progressBarOsvetljenje->setValue(svetlo);
    ui->grafWidget->dodajTacku(svetlo);

    QString linija = QString("T:%1°C  H:%2%  L:%3%  PIR:%4")
        .arg(temp, 0, 'f', 1)
        .arg(vlaz, 0, 'f', 1)
        .arg(svetlo)
        .arg(kretanje);
    ui->textEditIspis->append(linija);

    if (kretanje) {
        gpio_write(pi, PIN_LED_KRETANJE, 1);
        ui->labelKretanje->setPixmap(
            QPixmap(":/kretanje_da.png").scaled(160, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap(":/led_kretanje_on.png").scaled(120, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        gpio_write(pi, PIN_LED_KRETANJE, 0);
        ui->labelKretanje->setPixmap(
            QPixmap(":/kretanje_ne.png").scaled(160, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap(":/led_kretanje_off.png").scaled(120, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    if (temp > prag) {
        gpio_write(pi, PIN_LED_TEMP, 1);
        ui->labelStatus->setPixmap(
            QPixmap(":/status_iznad.png").scaled(160, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedTemp->setPixmap(
            QPixmap(":/led_temp_on.png").scaled(120, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        gpio_write(pi, PIN_LED_TEMP, 0);
        ui->labelStatus->setPixmap(
            QPixmap(":/status_ispod.png").scaled(160, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedTemp->setPixmap(
            QPixmap(":/led_temp_off.png").scaled(120, 35,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void Dialog::postaviPrag()
{
    prag = ui->spinBoxPrag->value();
}

int Dialog::citajPIR()
{
    return gpio_read(pi, PIN_PIR);
}

int Dialog::citajLDR()
{
    int handle = i2c_open(pi, I2C_BUS, PCF8591, 0);
    if (handle < 0) return 0;
    i2c_write_byte(pi, handle, 0x01);
    i2c_read_byte(pi, handle);
    int val = i2c_read_byte(pi, handle);
    i2c_close(pi, handle);
    return (val * 100) / 255;
}

float Dialog::citajDHT22Temp()
{
    return 22.5;
}

float Dialog::citajDHT22Vlaznost()
{
    return 55.0;
}
