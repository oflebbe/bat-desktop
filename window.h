#ifndef WINDOW_H
#define WINDOW_H

#include <QMouseEvent>
#include <QWidget>
#include <QResizeEvent>
#include <QScrollArea>
#include "renderarea.h"
#include "flo_file.h"

class Window : public QWidget
{
    Q_OBJECT
private:
    QScrollArea *scrollArea;
    RenderArea *renderArea;
    float overlap_percent = 0.6;
    int fft_size = 512;
    size_t start = 0;
    size_t end = 0;
    size_t size = 0;
    uint16_t *raw_file = NULL;

public:
    Window(const char filename[]);
    void showContent();
    void addImage(const QImage &image);
    RenderArea *getRenderArea() { return renderArea; };
    ~Window() {
        if (raw_file != NULL) {
            flo_unmapfile( (uint8_t *) raw_file, size);
            raw_file = NULL;
        }
    }

public slots:
    void setRange(long start, long end);

protected:
    void resizeEvent(QResizeEvent *e);
};

#endif