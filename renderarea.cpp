#include "renderarea.h"
#include <QPainter>
#include <fmt/core.h>

const int padX = 20;
const int padY = 20;

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}


void RenderArea::paintEvent(QPaintEvent * /* event */)
{
    if (pixmaps.size() == 0) {
        return;
    }
    
    QPainter painter(this);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
    
    int count = 0;
    int width = pixmaps.at(0).width() / pixmaps.at(0).devicePixelRatioF();
    int height = pixmaps.at(0).height()/ pixmaps.at(0).devicePixelRatioF();
    
    
    // number of pixmaps per line
    int numX = this->width() / (width+padX);

    for (auto p : pixmaps)
    {
        int col = count % numX;
        int row = count / numX;
        painter.drawPixmap(col*(width+padX), row * (height+padY), p);
        count++;
    }
}

void RenderArea::addPixmap(const QPixmap &pix)
{
    pixmaps.push_back(pix);
    update();
}
