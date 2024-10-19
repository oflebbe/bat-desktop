#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QWidget>
#include <QResizeEvent>
#include <vector>

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    explicit RenderArea(QWidget *parent = nullptr);

    void addImage(const QImage& image);
public slots:
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void paintLayout();

private:
    std::vector<QPixmap> pixmaps;
    int gridWidth;
    int gridHeight;
};

#endif