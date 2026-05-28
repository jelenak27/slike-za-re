#include "dialog.h"
#include "ui_dialog.h"
#include <pigpiod_if2.h>

#define PIN_DHT22         4
#define PIN_PIR          17
#define PIN_LED_TEMP     27
#define PIN_LED_KRETANJE 22
#define I2C_BUS           1
#define PCF8591        0x48

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , prag(25.0f)
    , lastTemp(0.0f)
    , lastVlaz(0.0f)
{
    ui->setupUi(this);

    // povezi se na pigpio daemon
    pi = pigpio_start(nullptr, nullptr);

    // inicijalizuj DHT22
    dht22_init(PIN_DHT22, pi);

    // PIR ulaz, LED izlazi
    set_mode(pi, PIN_PIR,          PI_INPUT);
    set_mode(pi, PIN_LED_TEMP,     PI_OUTPUT);
    set_mode(pi, PIN_LED_KRETANJE, PI_OUTPUT);
    gpio_write(pi, PIN_LED_TEMP,     0);
    gpio_write(pi, PIN_LED_KRETANJE, 0);

    // timer svakih 2 sekunde
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this,  &Dialog::citajSenzore);
    timer->start(2000);

    connect(ui->pushButtonPostavi, &QPushButton::clicked,
            this, &Dialog::postaviPrag);

    // pocetne slike
    ui->labelKretanje->setPixmap(
        QPixmap(":/kretanje_ne.png").scaled(
            160, 35, Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    ui->labelStatus->setPixmap(
        QPixmap(":/status_ispod.png").scaled(
            160, 35, Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    ui->labelLedTemp->setPixmap(
        QPixmap(":/led_temp_off.png").scaled(
            120, 35, Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    ui->labelLedKretanje->setPixmap(
        QPixmap(":/led_kretanje_off.png").scaled(
            120, 35, Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
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
    // DHT22 - jedno citanje, dve vrednosti
    float t = 0.0f, h = 0.0f;
    int rc = dht22_read(pi, PIN_DHT22, &t, &h);
    if (rc == 0) {
        lastTemp = t;
        lastVlaz = h;
    }
    // ako citanje nije uspelo, koristi poslednje vrednosti

    // LDR i PIR
    int svetlo   = citajLDR();
    int kretanje = citajPIR();

    // prikaz
    ui->lcdTemp->display(lastTemp);
    ui->lcdVlaznost->display(lastVlaz);
    ui->progressBarOsvetljenje->setValue(svetlo);
    ui->grafWidget->dodajTacku(svetlo);

    // ispis
    QString linija = QString("T:%1C  H:%2%  L:%3%  PIR:%4")
        .arg(lastTemp, 0, 'f', 1)
        .arg(lastVlaz, 0, 'f', 1)
        .arg(svetlo)
        .arg(kretanje);
    ui->textEditIspis->append(linija);

    // kretanje
    if (kretanje) {
        gpio_write(pi, PIN_LED_KRETANJE, 1);
        ui->labelKretanje->setPixmap(
            QPixmap(":/kretanje_da.png").scaled(
                160, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap(":/led_kretanje_on.png").scaled(
                120, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    } else {
        gpio_write(pi, PIN_LED_KRETANJE, 0);
        ui->labelKretanje->setPixmap(
            QPixmap(":/kretanje_ne.png").scaled(
                160, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap(":/led_kretanje_off.png").scaled(
                120, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    }

    // temperatura vs prag
    if (lastTemp > prag) {
        gpio_write(pi, PIN_LED_TEMP, 1);
        ui->labelStatus->setPixmap(
            QPixmap(":/status_iznad.png").scaled(
                160, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        ui->labelLedTemp->setPixmap(
            QPixmap(":/led_temp_on.png").scaled(
                120, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    } else {
        gpio_write(pi, PIN_LED_TEMP, 0);
        ui->labelStatus->setPixmap(
            QPixmap(":/status_ispod.png").scaled(
                160, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        ui->labelLedTemp->setPixmap(
            QPixmap(":/led_temp_off.png").scaled(
                120, 35, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
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
