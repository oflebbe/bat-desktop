#include "renderarea.h"
#include <QPainter>
#include <QWindow>
#include <unistd.h>

const int padX = 20;
const int padY = 20;

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    gridWidth = 0;
    gridHeight = 0;
    setEnabled(true);
    mouseX = 0;
    state = STATUS_OFF;
    setFocusPolicy(Qt::StrongFocus);
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

    int cols = std::max(screen->size().width() / (gridWidth), 1);

    for (size_t j = 0; j < pixmaps.size(); j++)
    {
        int col = j % cols;
        int row = j / cols;

        painter.drawPixmap(col * (gridWidth + padX), row * (gridHeight + padY), pixmaps[j]);
    }
    printf("%d %d\n", mouseX, maxY);
    if (state == STATUS_LEFT_SET) {
           painter.drawLine(leftX, 0, leftX, maxY);
    }
    if (state == STATUS_OFF|| state == STATUS_LEFT_SET)
        painter.drawLine(mouseX, 0, mouseX, maxY);

    if (state == STATUS_CUT) {
        painter.fillRect(leftX, 0, mouseX-leftX, maxY, Qt::red);
        state=STATUS_OFF;
        emit selectRange( leftX, mouseX);
    }
    setMouseTracking(true);
}

void RenderArea::addImage(const QImage &image)
{
    QPixmap qpix = QPixmap::fromImage(image);
    double scale = this->devicePixelRatioF();
    // scale= 1.25;
    qpix.setDevicePixelRatio(scale);
    pixmaps.push_back(qpix);
    gridWidth = std::max(gridWidth, (int)(qpix.width() / scale));
    gridHeight = std::max(gridHeight, (int)(qpix.height() / scale));

    QSize screenSize = this->screen()->size();

    int cols = std::max(screenSize.width() / (gridWidth), 1);

    int rows = pixmaps.size() / cols + 1;
    maxY =  rows * (gridHeight + padY);

    setMinimumSize(QSize(gridWidth, maxY));

    update();
}

void RenderArea::clear() {
    pixmaps.clear();
    update();
}


void RenderArea::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    mouseX = pos.x();
    event->accept();
    setMouseTracking(true);
    update();
}

void RenderArea::keyPressEvent(QKeyEvent *event) {
    if (state == STATUS_OFF) {
        state = STATUS_LEFT_SET;
        leftX = mouseX;
    } else if (state == STATUS_LEFT_SET) {
        state = STATUS_CUT;
    }
}


