#include <cassert>
#include <fstream>
#include <fmt/core.h>

#include <QApplication>
#include <QImage>
#include <QInputDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <vector>
#include <filesystem>
#include "window.h"
#include "classify.h"

int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    fmt::print("usage: filename\n");
    exit(1);
  }

  QApplication app(argc, argv);

  Window qwindow;
  QDirIterator it(argv[1]);
  Classifier c( 512);
  while (it.hasNext()) {
      QFileInfo fi = it.nextFileInfo();
      if (fi.suffix() == "raw") {
        c.open(fi.filePath().toStdString());
        if (c.isRelevant())
        {
          qwindow.addImage(c.image());
        }
      }

  }

  qwindow.show();
  app.exec();
}
