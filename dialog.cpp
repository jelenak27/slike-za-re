#include "dialog.h"
#include "ui_dialog.h"
#include <pigpiod_if2.h>

#define PIN_DHT22         4
#define PIN_PIR          20
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

    // DHT thread
    dhtThread = new DhtThread();
    dhtThread->pi_handle = pi;
    dhtThread->gpio = PIN_DHT22;
    connect(dhtThread, &DhtThread::rezultat,
            this, &Dialog::primiDht);

    // timer svakih 2 sekunde
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this,  &Dialog::citajSenzore);
    timer->start(2000);

    connect(ui->pushButtonPostavi, &QPushButton::clicked,
            this, &Dialog::postaviPrag);

    // pocetne vrednosti labela
    ui->labelKretanje->setText("Nema kretanja");
    ui->labelStatus->setText("U granicama");
    ui->labelLedTemp->setText("Temp OK");
    ui->labelLedKretanje->setText("Mirovanje");
}

Dialog::~Dialog()
{
    dhtThread->quit();
    dhtThread->wait();
    gpio_write(pi, PIN_LED_TEMP,     0);
    gpio_write(pi, PIN_LED_KRETANJE, 0);
    pigpio_stop(pi);
    delete ui;
}

void Dialog::primiDht(float t, float h)
{
    lastTemp = t;
    lastVlaz = h;
}

void Dialog::citajSenzore()
{
    // pokreni DHT citanje u posebnom threadu ako vec ne radi
    if (!dhtThread->isRunning())
        dhtThread->start();

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
        ui->labelKretanje->setText("Detektovano");
        ui->labelLedKretanje->setText("Kretanje!");
    } else {
        gpio_write(pi, PIN_LED_KRETANJE, 0);
        ui->labelKretanje->setText("Nema kretanja");
        ui->labelLedKretanje->setText("Mirovanje");
    }

    // temperatura vs prag
    if (lastTemp > prag) {
        gpio_write(pi, PIN_LED_TEMP, 1);
        ui->labelStatus->setText("Iznad praga");
        ui->labelLedTemp->setText("Alarm temp");
    } else {
        gpio_write(pi, PIN_LED_TEMP, 0);
        ui->labelStatus->setText("U granicama");
        ui->labelLedTemp->setText("Temp OK");
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
