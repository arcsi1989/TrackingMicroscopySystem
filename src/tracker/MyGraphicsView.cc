#include "MyGraphicsView.h"
#include <QResizeEvent>

MyGraphicsView::MyGraphicsView(QWidget * parent)
  : QGraphicsView(parent)
{
  mGraphicsScene = new QGraphicsScene;
}

MyGraphicsView::~MyGraphicsView()
{
  delete mGraphicsScene;
}

void MyGraphicsView::resizeEvent(QResizeEvent *event)
{
  fitInView(mGraphicsScene->sceneRect(),Qt::KeepAspectRatio);
  QGraphicsView::resizeEvent(event);
}
