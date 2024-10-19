#include <cassert>
#include <fstream>
#include <fmt/core.h>

#include <QApplication>
#include <QImage>
#include <QInputDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <filesystem>
#include "window.h"
#include "create_image.h"

#include "flo_file.h"

int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("usage: filename\n");
    exit(1);
  }

  QApplication app(argc, argv);

  Window qwindow;
  QDirIterator it(argv[1]);

  while (it.hasNext())
  {
    QFileInfo fi = it.nextFileInfo();
    if (fi.suffix() == "raw")
    {
      long size = 0;
      uint16_t *raw_file = (uint16_t *) flo_readfile( fi.filesystemFilePath().c_str(), &size);
      if (!raw_file) {
        continue;
      }
      if (size % 2 == 1) {
        free(raw_file);
        continue;   
      }
      size /= 2;
      float overlap_percent = 0.6;
      flo_pixmap_t *pixmap = create_image_meow(size, raw_file, 512, overlap_percent);
      free( raw_file);
      if (!pixmap) {
        continue;
      }
      QImage qimage((uchar *)pixmap->buf, pixmap->width, pixmap->height, QImage::Format_RGB16, free, pixmap);
      qwindow.addImage(qimage);
    }
  }

  qwindow.show();
  app.exec();
}
