#ifndef GRAFWIDGET_H
#define GRAFWIDGET_H

#include <QWidget>
#include <QVector>

class GrafWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GrafWidget(QWidget *parent = nullptr);
    void dodajTacku(float temp);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<float> podaci;
};

#endif
