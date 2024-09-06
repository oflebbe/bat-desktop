#ifndef MYLABEL
#define MYLABEL

#include <QMouseEvent>
#include <QLabel>
#include <QResizeEvent>

class MyLabel : public QLabel {
    Q_OBJECT
    private: 
        QPixmap pixmap;
    public:
        MyLabel();
    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void resizeEvent(QResizeEvent * e);
    public slots:
        void setPixmap(const QPixmap &);
};

#endif