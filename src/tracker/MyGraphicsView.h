#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>

class MyGraphicsView : public QGraphicsView
{
  public:
    MyGraphicsView(QWidget * parent = 0);
    ~MyGraphicsView();

    QGraphicsScene* mGraphicsScene;
  private:
    void resizeEvent(QResizeEvent *event);
};

#endif // MYGRAPHICSVIEW_H
