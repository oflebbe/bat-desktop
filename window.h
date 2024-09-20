#ifndef WINDOW_H
#define WINDOW_H

#include <QMouseEvent>
#include <QWidget>
#include <QResizeEvent>
#include <QScrollArea>
#include "renderarea.h"

class Window : public QWidget
{
    Q_OBJECT
private:
    QScrollArea *scrollArea;
    RenderArea *renderArea;

public:
    Window();
    void addImage(const QImage &image);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *e);
};

#endif