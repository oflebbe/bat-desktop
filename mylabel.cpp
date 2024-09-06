#include "mylabel.h"

#include <fmt/core.h>

MyLabel::MyLabel() { setMouseTracking(true); }

void MyLabel::mouseMoveEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  fmt::print("{} {}\n", pos.rx(), pos.ry());
}

void MyLabel::resizeEvent(QResizeEvent *e) {
  QPixmap scaled = this->pixmap.scaled(this->size(), 
      Qt::KeepAspectRatio,
      Qt::SmoothTransformation);
  QLabel::setPixmap(scaled);
}

void MyLabel::setPixmap(const QPixmap &pixmap) {
  this->pixmap = pixmap;
  QLabel::setPixmap(pixmap);
}