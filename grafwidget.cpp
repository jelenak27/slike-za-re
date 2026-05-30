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
    update();
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

    // min/max za skaliranje
    float minT = podaci[0], maxT = podaci[0];
    for (int i = 1; i < n; i++) {
        if (podaci[i] < minT) minT = podaci[i];
        if (podaci[i] > maxT) maxT = podaci[i];
    }
    if (maxT - minT < 2.0f) {
        maxT = minT + 2.0f;
    }

    // osi
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(30, 5, 30, h - 20);
    painter.drawLine(30, h - 20, w - 5, h - 20);

    // oznake Y ose
    painter.setFont(QFont("Arial", 8));
    painter.drawText(2, h - 20, QString::number((int)minT));
    painter.drawText(2, h / 2, QString::number((int)((minT + maxT) / 2)));
    painter.drawText(2, 12, QString::number((int)maxT));

    // linija grafa - plava za temperaturu
    painter.setPen(QPen(QColor(0, 100, 255), 2));
    float korakX = (float)(w - 35) / (n - 1);

    for (int i = 1; i < n; i++) {
        int x1 = 30 + (i - 1) * korakX;
        int y1 = (h - 20) - (int)((podaci[i-1] - minT) * (h - 30) / (maxT - minT));
        int x2 = 30 + i * korakX;
        int y2 = (h - 20) - (int)((podaci[i] - minT) * (h - 30) / (maxT - minT));
        painter.drawLine(x1, y1, x2, y2);
    }
}
