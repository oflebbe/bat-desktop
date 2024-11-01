#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QWidget>
#include <QResizeEvent>
#include <vector>

typedef enum  {
    STATUS_OFF,
    STATUS_LEFT_SET,
    STATUS_CUT
} STATE;

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    explicit RenderArea(QWidget *parent = nullptr);

    void addImage(const QImage &image);
    void clear();
public slots:

protected:
    void paintEvent(QPaintEvent *event) override;
    void paintLayout();
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

signals:
    void selectRange(int start, int stop);

private:
    std::vector<QPixmap> pixmaps;
    int gridWidth;
    int gridHeight;
    int mouseX;
    int maxY;
    int leftX;
    STATE state;
};

#endif