#include <cassert>

#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("usage: filename\n");
    exit(1);
  }

  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
  QApplication app(argc, argv);

  Window qwindow(argv[1]);
  QObject::connect(qwindow.getRenderArea(), &RenderArea::selectRange, &qwindow, &Window::setRange);
  qwindow.show();
  app.exec();
}
