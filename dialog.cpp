#include "dialog.h"
#include "ui_dialog.h"
#include <pigpiod_if2.h>
#include <cstdio>

#define PIN_DHT22        4
#define PIN_PIR         20
#define PIN_LED_TEMP    27
#define PIN_LED_KRETANJE 22
#define I2C_BUS          1
#define PCF8591       0x48
#define LDR_PRAG        60

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , lastTemp(0.0f)
    , lastVlaz(0.0f)
    , prag(25.0f)
{
    ui->setupUi(this);

    pi = pigpio_start(nullptr, nullptr);
    dht22_init(PIN_DHT22, pi);

    set_mode(pi, PIN_PIR,          PI_INPUT);
    set_mode(pi, PIN_LED_TEMP,     PI_OUTPUT);
    set_mode(pi, PIN_LED_KRETANJE, PI_OUTPUT);
    gpio_write(pi, PIN_LED_TEMP,     0);
    gpio_write(pi, PIN_LED_KRETANJE, 0);

    // pocetne slike
    ui->labelStatus->setPixmap(
        QPixmap("/opt/re_proj/slike/status_ispod.png").scaled(
            160, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelKretanje->setPixmap(
        QPixmap("/opt/re_proj/slike/kretanje_ne.png").scaled(
            160, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelLedKretanje->setPixmap(
        QPixmap("/opt/re_proj/slike/led_kretanje_off.png").scaled(
            120, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelLedTemp->setPixmap(
        QPixmap("/opt/re_proj/slike/led_temp_off.png").scaled(
            120, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->labelDanNoc->setText("N/A");

    // timer za DHT22 i LDR svakih 5 sekundi
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &Dialog::citajSenzore);
    timer->start(5000);

    // timer za PIR svakih 500ms
    timerPIR = new QTimer(this);
    connect(timerPIR, &QTimer::timeout,
            this, &Dialog::citajPIR);
    timerPIR->start(500);

    connect(ui->pushButtonPostavi, &QPushButton::clicked,
            this, &Dialog::postaviPrag);
}

Dialog::~Dialog()
{
    gpio_write(pi, PIN_LED_TEMP,     0);
    gpio_write(pi, PIN_LED_KRETANJE, 0);
    pigpio_stop(pi);
    delete ui;
}

void Dialog::citajSenzore()
{
    // DHT22
    float t = 0.0f, h = 0.0f;
    int rc = dht22_read(pi, PIN_DHT22, &t, &h);
    if (rc == 0) {
        lastTemp = t;
        lastVlaz = h;
        ui->lcdTemp->display(lastTemp);
        ui->lcdVlaznost->display(lastVlaz);

        if (lastTemp > prag) {
            gpio_write(pi, PIN_LED_TEMP, 1);
            ui->labelStatus->setPixmap(
                QPixmap("/opt/re_proj/slike/status_iznad.png").scaled(
                    160, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            ui->labelLedTemp->setPixmap(
                QPixmap("/opt/re_proj/slike/led_temp_on.png").scaled(
                    120, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            gpio_write(pi, PIN_LED_TEMP, 0);
            ui->labelStatus->setPixmap(
                QPixmap("/opt/re_proj/slike/status_ispod.png").scaled(
                    160, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            ui->labelLedTemp->setPixmap(
                QPixmap("/opt/re_proj/slike/led_temp_off.png").scaled(
                    120, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    // graf temperature
    ui->grafWidget->dodajTacku(lastTemp);

    // LDR - dan ili noc (invertovano: manja vrednost = vise svetla)
    int svetlo = citajLDR();
    fprintf(stderr, "LDR raw: %d\n", svetlo);
    if (svetlo <= LDR_PRAG)
        ui->labelDanNoc->setText("Dan");
    else
        ui->labelDanNoc->setText("Noc");

    QString linija = QString("T:%1C H:%2% L:%3% rc:%4")
        .arg(lastTemp, 0, 'f', 1)
        .arg(lastVlaz, 0, 'f', 1)
        .arg(svetlo)
        .arg(rc);
    ui->textBrowserIspis->append(linija);
}

void Dialog::citajPIR()
{
    int kretanje = gpio_read(pi, PIN_PIR);
    if (kretanje) {
        gpio_write(pi, PIN_LED_KRETANJE, 1);
        ui->labelKretanje->setPixmap(
            QPixmap("/opt/re_proj/slike/kretanje_da.png").scaled(
                160, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap("/opt/re_proj/slike/led_kretanje_on.png").scaled(
                120, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        gpio_write(pi, PIN_LED_KRETANJE, 0);
        ui->labelKretanje->setPixmap(
            QPixmap("/opt/re_proj/slike/kretanje_ne.png").scaled(
                160, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap("/opt/re_proj/slike/led_kretanje_off.png").scaled(
                120, 35, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

int Dialog::citajLDR()
{
    int handle = i2c_open(pi, I2C_BUS, PCF8591, 0);
    if (handle < 0) return 0;
    i2c_write_byte(pi, handle, 0x40 | 0x00); // aktiviraj ADC, kanal 0
    i2c_read_byte(pi, handle);               // odbaci prvi
    int val = i2c_read_byte(pi, handle);     // pravi ocitaj
    i2c_close(pi, handle);
    return (val * 100) / 255;
}

void Dialog::postaviPrag()
{
    prag = ui->spinBoxPrag->value();
}
