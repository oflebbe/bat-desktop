#include "renderarea.h"
#include <QPainter>
#include <QWindow>

const int padX = 20;
const int padY = 20;

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    gridWidth = 0;
    gridHeight = 0;
}

void RenderArea::paintEvent(QPaintEvent * /* event */)
{
    if (pixmaps.size() == 0)
    {
        return;
    }
    QScreen *screen = this->screen();

    QPainter painter(this);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);

    int cols = std::max( screen->size().width() / (gridWidth), 1);

    for (size_t j = 0; j < pixmaps.size(); j++)
    {
        int col = j % cols;
        int row = j / cols;

        painter.drawPixmap(col * (gridWidth + padX), row * (gridHeight + padY), pixmaps[j]);
    }
}

void RenderArea::addImage(const QImage &image)
{
    QPixmap qpix = QPixmap::fromImage(image);
    double scale = this->devicePixelRatioF();
    //scale= 1.25;
    qpix.setDevicePixelRatio(scale);
    pixmaps.push_back(qpix);
    gridWidth = std::max(gridWidth, (int)(qpix.width() / scale));
    gridHeight = std::max(gridHeight, (int)(qpix.height() / scale));

    QSize screenSize = this->screen()->size();

    int cols = std::max(screenSize.width() / (gridWidth),1);

    int rows = pixmaps.size() / cols + 1;

    setMinimumSize(QSize( gridWidth, rows * (gridHeight + padY)));

    update();
}