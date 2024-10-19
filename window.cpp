#include "window.h"

#include <QGridLayout>

Window::Window()
{
  QGridLayout *layout = new QGridLayout(this);
  renderArea = new RenderArea;
  // setMouseTracking(true);
  

  scrollArea = new QScrollArea;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(renderArea);
  scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn);
  scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn);
  scrollArea->setWidgetResizable(true);
  layout->addWidget(scrollArea, 0, 0);
  layout->setColumnStretch(0, 1);
  layout->setRowStretch(0, 1);


  setWindowTitle(tr("BAT Desktop"));
}

void Window::addImage(const QImage &image)
{
  renderArea->addImage(image);
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
}

void Window::resizeEvent(QResizeEvent *e)
{
  scrollArea->resize(e->size());
}
