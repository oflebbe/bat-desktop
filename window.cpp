#include "window.h"

#include <fmt/core.h>
#include <QGridLayout>

Window::Window()
{
  QGridLayout *layout = new QGridLayout(this);
  renderArea = new RenderArea;
  // setMouseTracking(true);
  

  scrollArea = new QScrollArea;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(renderArea);
  // scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn);
  layout->addWidget(scrollArea, 0, 0);
  layout->setColumnStretch(0, 1);
  layout->setRowStretch(0, 1);

  setWindowTitle(tr("BAT Desktop"));
}

void Window::addImage(const QImage &image)
{
  QPixmap qpix = QPixmap::fromImage(image);
  qpix.setDevicePixelRatio(this->devicePixelRatioF());
  renderArea->addPixmap(qpix);
  renderArea->setMinimumSize(QSize(2000, 20000));
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
  // fmt::print("{} {}\n", pos.rx(), pos.ry());
}

void Window::resizeEvent(QResizeEvent *e)
{
  scrollArea->resize(e->size());
}
