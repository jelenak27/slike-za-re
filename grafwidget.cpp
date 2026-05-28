#include "grafwidget.h"
#include <QPainter>

GrafWidget::GrafWidget(QWidget *parent)
    : QWidget(parent)
{
}

void GrafWidget::dodajTacku(int svetlo)
{
    podaci.append(svetlo);
    // cuva max 50 tacaka
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

    int w = width();
    int h = height();
    int n = podaci.size();

    // osi
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(30, 5, 30, h - 20);       // Y osa
    painter.drawLine(30, h - 20, w - 5, h - 20); // X osa

    // oznake Y ose (0, 50, 100)
    painter.setFont(QFont("Arial", 8));
    painter.drawText(2, h - 20, "0");
    painter.drawText(2, h/2, "50");
    painter.drawText(2, 12, "100");

    // linija grafa
    painter.setPen(QPen(QColor(255, 140, 0), 2)); // narandzasta
    float korakX = (float)(w - 35) / (n - 1);

    for (int i = 1; i < n; i++) {
        int x1 = 30 + (i - 1) * korakX;
        int y1 = (h - 20) - (podaci[i-1] * (h - 30) / 100);
        int x2 = 30 + i * korakX;
        int y2 = (h - 20) - (podaci[i] * (h - 30) / 100);
        painter.drawLine(x1, y1, x2, y2);
    }
}
