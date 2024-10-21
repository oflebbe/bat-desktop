#include <cassert>

#include <QApplication>
#include <QImage>
#include <QInputDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <filesystem>
#include "window.h"
#include "create_image.h"

#include "flo_file.h"
#define STEREO

int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("usage: filename\n");
    exit(1);
  }

  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
  QApplication app(argc, argv);

  Window qwindow;
  QDirIterator it(argv[1]);

  while (it.hasNext())
  {
    QFileInfo fi = it.nextFileInfo();
    if (fi.suffix() == "raw")
    {
      long size = 0;
      uint16_t *raw_file = (uint16_t *)flo_readfile(fi.filesystemFilePath().string().c_str(), &size);
      if (!raw_file)
      {
        continue;
      }
      float overlap_percent = 0.6;
      size /= 2;
      if (size % 2 == 1)
      {
        free(raw_file);
        continue;
      }


#ifdef STEREO
      uint16_t *audio_l = (uint16_t *) malloc( size);
      uint16_t *audio_r = (uint16_t *) malloc( size);
      size /= 2;      
      for (int j = 0; j < size / 2 - 1; j++)
      {
        audio_l[j] = raw_file[j * 2];
        audio_r[j] = raw_file[j * 2+1];
      }
      free( raw_file);
      flo_pixmap_t *pixmap_l = create_image_meow(size, audio_l, 512, overlap_percent);
      free(audio_l);
      if (!pixmap_l)
      {
        continue;
      }
      QImage qimage_l((uchar *)pixmap_l->buf, pixmap_l->width, pixmap_l->height, QImage::Format_RGB16, free, pixmap_l);
      qwindow.addImage(qimage_l);

      flo_pixmap_t *pixmap_r = create_image_meow(size, audio_r, 512, overlap_percent);
      free(audio_r);
      if (!pixmap_r)
      {
        continue;
      }
      QImage qimage_r((uchar *)pixmap_r->buf, pixmap_r->width, pixmap_r->height, QImage::Format_RGB16, free, pixmap_r);
      qwindow.addImage(qimage_r);
#else
      flo_pixmap_t *pixmap = create_image_meow(size, raw_file, 512, overlap_percent);
      free(raw_file);
      if (!pixmap)
      {
        continue;
      }
      QImage qimage((uchar *)pixmap->buf, pixmap->width, pixmap->height, QImage::Format_RGB16, free, pixmap);
      qwindow.addImage(qimage);
#endif
    }
  }

  qwindow.show();
  app.exec();
}
