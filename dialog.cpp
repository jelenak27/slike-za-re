#include "dialog.h"
#include "ui_dialog.h"
#include <pigpiod_if2.h>

#define PIN_DHT22  4
#define PIN_PIR   17
#define I2C_BUS    1
#define PCF8591   0x48

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , prag(25.0)
{
    ui->setupUi(this);

    // povezi se na pigpio daemon
    pi = pigpio_start(nullptr, nullptr);

    // postavi PIR kao ulaz
    set_mode(pi, PIN_PIR, PI_INPUT);

    // timer za ocitavanje svakih 2 sekunde
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Dialog::citajSenzore);
    timer->start(2000);

    connect(ui->pushButtonPostavi, &QPushButton::clicked,
            this, &Dialog::postaviPrag);

    // ucitaj pocetne slike
    ui->labelKretanje->setPixmap(
        QPixmap(":/kretanje_ne.png").scaled(
            ui->labelKretanje->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelLedTemp->setPixmap(
        QPixmap(":/led_temp_off.png").scaled(
            ui->labelLedTemp->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelLedKretanje->setPixmap(
        QPixmap(":/led_kretanje_off.png").scaled(
            ui->labelLedKretanje->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->labelStatus->setPixmap(
        QPixmap(":/status_ispod.png").scaled(
            ui->labelStatus->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

Dialog::~Dialog()
{
    pigpio_stop(pi);
    delete ui;
}

void Dialog::citajSenzore()
{
    float temp = citajDHT22Temp();
    float vlaz = citajDHT22Vlaznost();
    int svetlo = citajLDR();
    int kretanje = citajPIR();

    // prikaz temperature i vlaznosti
    ui->lcdTemp->display(temp);
    ui->lcdVlaznost->display(vlaz);

    // progress bar osvetljenje (0-255)
    ui->progressBarOsvetljenje->setValue(svetlo);

    // ispis merenja
    QString linija = QString("T:%1°C  H:%2%  L:%3  PIR:%4")
        .arg(temp, 0, 'f', 1)
        .arg(vlaz, 0, 'f', 1)
        .arg(svetlo)
        .arg(kretanje);
    ui->textBrowserIspis->append(linija);

    // kretanje slike
    if (kretanje) {
        ui->labelKretanje->setPixmap(
            QPixmap(":/kretanje_da.png").scaled(
                ui->labelKretanje->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap(":/led_kretanje_on.png").scaled(
                ui->labelLedKretanje->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        ui->labelKretanje->setPixmap(
            QPixmap(":/kretanje_ne.png").scaled(
                ui->labelKretanje->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedKretanje->setPixmap(
            QPixmap(":/led_kretanje_off.png").scaled(
                ui->labelLedKretanje->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // temperatura prag slike
    if (temp > prag) {
        ui->labelStatus->setPixmap(
            QPixmap(":/status_iznad.png").scaled(
                ui->labelStatus->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedTemp->setPixmap(
            QPixmap(":/led_temp_on.png").scaled(
                ui->labelLedTemp->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        ui->labelStatus->setPixmap(
            QPixmap(":/status_ispod.png").scaled(
                ui->labelStatus->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLedTemp->setPixmap(
            QPixmap(":/led_temp_off.png").scaled(
                ui->labelLedTemp->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    ui->grafWidget->dodajTacku(temp);

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
    i2c_write_byte(pi, handle, 0x40); // AIN0
    i2c_read_byte(pi, handle);        // dummy read
    int val = i2c_read_byte(pi, handle);
    i2c_close(pi, handle);
    return val;
}

float Dialog::citajDHT22Temp()
{
    // placeholder - DHT22 zahteva posebnu biblioteku
    // privremeno vraca test vrednost
    return 22.5;
}

float Dialog::citajDHT22Vlaznost()
{
    return 55.0;
}
