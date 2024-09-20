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

   /* QSize minimumSizeHint() const override;
    QSize sizeHint() const override; */
    void addPixmap(const QPixmap& pix);

public slots:
    
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<QPixmap> pixmaps;
};

#endif