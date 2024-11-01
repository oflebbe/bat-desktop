#include <QImage>
#include <QInputDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <QGridLayout>
#include <filesystem>
#include "window.h"
#include "create_image.h"

#include "flo_file.h"
#define STEREO
#include "window.h"

Window::Window(const char filename[])
{
  QGridLayout *layout = new QGridLayout(this);
  renderArea = new RenderArea;
  // setMouseTracking(true);

  scrollArea = new QScrollArea;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(renderArea);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setWidgetResizable(true);
  layout->addWidget(scrollArea, 0, 0);
  layout->setColumnStretch(0, 1);
  layout->setRowStretch(0, 1);

  setWindowTitle(tr("BAT Desktop"));
  size = 0;
  FILE *fp = fopen( filename, "r");
  if (!fp) {
    return;
  }
  raw_file = (uint16_t *)flo_mapfile(fp, &size);
  fclose(fp);
  if (!raw_file)
  {
    return;
  }

  size /= 2;
  if (size % 2 == 1)
  {
    raw_file = NULL;
    return;
  }
  showContent();
}

void Window::showContent() {
  

  // size = 1024*1024*10;

#ifdef STEREO
  uint16_t *audio_l = (uint16_t *)malloc(size);
  uint16_t *audio_r = (uint16_t *)malloc(size);
  int stereo_size = size/2;
  for (int j = 0; j < stereo_size / 2 - 1; j++)
  {
    audio_l[j] = raw_file[j * 2];
    audio_r[j] = raw_file[j * 2 + 1];
  }
  
  if (start == 0 && end == 0) {
    end = stereo_size;
  }
  flo_pixmap_t *pixmap_l = create_image_meow(stereo_size, audio_l, start, end, fft_size, overlap_percent);
  if (!pixmap_l)
  {
    return;
  }
  QImage qimage_l((uchar *)pixmap_l->buf, pixmap_l->width, pixmap_l->height, QImage::Format_RGB16, free, pixmap_l);
  renderArea->addImage(qimage_l);

  flo_pixmap_t *pixmap_r = create_image_meow(stereo_size, audio_r, start, end, fft_size, overlap_percent);
  if (!pixmap_r)
  {
    return;
  }
  QImage qimage_r((uchar *)pixmap_r->buf, pixmap_r->width, pixmap_r->height, QImage::Format_RGB16, free, pixmap_r);
  renderArea->addImage(qimage_r);
  /*
  flo_pixmap_t *pixmap_c = create_image_cross(stereo_size, audio_l, audio_r, 512, overlap_percent);
  free(audio_l);
  free(audio_r);
  if (!pixmap_c)
  {
    continue;
  }
  QImage qimage_c((uchar *)pixmap_c->buf, pixmap_c->width, pixmap_c->height, QImage::Format_RGB16, free, pixmap_c);
  qwindow.addImage(qimage_c);
*/
#else
  flo_pixmap_t *pixmap = create_image_meow(stereo_size, raw_file, 512, overlap_percent);
  if (!pixmap)
  {
    continue;
  }
  QImage qimage((uchar *)pixmap->buf, pixmap->width, pixmap->height, QImage::Format_RGB16, free, pixmap);
  qwindow.addImage(qimage);
#endif
  free(audio_l);
  free(audio_r);
}

void Window::resizeEvent(QResizeEvent *e)
{
  scrollArea->resize(e->size());
}

void Window::setRange(long start, long end)
{
  
  this->start = start / ;
  this->end = end;
  showContent();
}

