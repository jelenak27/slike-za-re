#include "grafwidget.h"
#include <QPainter>

GrafWidget::GrafWidget(QWidget *parent)
    : QWidget(parent)
{
}

void GrafWidget::dodajTacku(float temp)
{
    podaci.append(temp);
    if (podaci.size() > 50)
        podaci.removeFirst();
    update(); // poziva paintEvent
}

void GrafWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (podaci.size() < 2) return;

    // odredi min/max za skaliranje
    float minT = *std::min_element(podaci.begin(), podaci.end()) - 2;
    float maxT = *std::max_element(podaci.begin(), podaci.end()) + 2;

    int w = width();
    int h = height();
    int n = podaci.size();

    // crtaj osu
    painter.setPen(QPen(Qt::gray, 1));
    painter.drawLine(30, 5, 30, h-20);
    painter.drawLine(30, h-20, w-5, h-20);

    // crtaj liniju temperature
    painter.setPen(QPen(Qt::red, 2));
    for (int i = 1; i < n; i++) {
        float x1 = 30 + (float)(i-1)/(n-1) * (w-35);
        float y1 = h-20 - (podaci[i-1]-minT)/(maxT-minT) * (h-30);
        float x2 = 30 + (float)i/(n-1) * (w-35);
        float y2 = h-20 - (podaci[i]-minT)/(maxT-minT) * (h-30);
        painter.drawLine(x1, y1, x2, y2);
    }

    // labele
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    painter.drawText(2, 15, QString::number(maxT,'f',1));
    painter.drawText(2, h-15, QString::number(minT,'f',1));
}
