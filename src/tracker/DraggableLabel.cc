/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "DraggableLabel.h"

#include <QPainter>
#include "TMath.h"
#include "Logger.h"

namespace tracker
{
    DraggableLabel::DraggableLabel(QWidget* parent, Qt::WindowFlags f)
        : QLabel(parent, f)
        , mTimerID(0)
        , mZoom(1.0)
        , mLastPosition(0, 0)
    {
        this->setDisplayFrequency(24);
    }

    void DraggableLabel::setDisplayFrequency(double value)
    {
        mDisplayFrequency = value;
        if (mTimerID != 0)
            this->killTimer(mTimerID);
        mTimerID = this->startTimer(1000.0 / mDisplayFrequency);
    }

    void DraggableLabel::setZoom(double value)
    {
        if (value <= 0.0)
            return;

        // Almost 0 zoom would be bad because you can only zoom within the image...
        if (value >= 0.2 && value <= 5.0)
        {
            mZoom = value;
            mUpdateImage = true;
            emit zoomChanged(mZoom);
        }
    }

    void DraggableLabel::mousePressEvent(QMouseEvent* ev)
    {
        QLabel::mousePressEvent(ev);
        mLastPosition = ev->pos();
    }

    void DraggableLabel::mouseMoveEvent(QMouseEvent* ev)
    {
        QLabel::mouseMoveEvent(ev);
        emit mouseDragged(ev->pos() - mLastPosition);
        mLastPosition = ev->pos();
    }

    void DraggableLabel::wheelEvent(QWheelEvent* ev)
    {
        // Don't notify child or we scroll the vertical scroller as well
        //QLabel::wheelEvent(ev);

        // Zoom change should be proportional to the current zoom
        // Also, a step up and down again should not change the resulting zoom
        // --> Positive and negative steps cannot be calculated the same way
        // Note: Usually, a single wheel step is 120 on Windows
        /*
		for (int i = 0; i < +ev->delta() / 120; ++i)
            this->setZoom(mZoom * (0.1 + 1));
        for (int i = 0; i < -ev->delta() / 120; ++i)
            this->setZoom(mZoom / (0.1 + 1));
		*/
		emit labelrequestszmove(ev->delta()/120);
		int k= ev->delta();
		int i= +ev->delta();
		int j= -ev->delta();
    }

    void DraggableLabel::paintEvent(QPaintEvent* ev)
    {
        if (!mNewImage.isNull())
        {
            if (mNewDebugImage.isNull())
                this->setMinimumSize(mNewImage.size() * mZoom);
            else
            {
                this->setMinimumHeight((mNewImage.height() + mNewDebugImage.height()) * mZoom);
                this->setMinimumWidth(qMax(mNewImage.width(), mNewDebugImage.width()) * mZoom);
            }

            QPainter painter(this);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Zoom of 1.001 should not result in applying zoom and thus using CPU
            if (qRound(mZoom * 100.0) != 100)
                painter.scale(mZoom, mZoom);

            painter.drawImage(0, 0, mNewImage);
            if (!mNewDebugImage.isNull())
                painter.drawImage(0, mNewImage.height(), mNewDebugImage);
        }
    }

    void DraggableLabel::displayImage(QImage image)
    {
        mNewImage = image;
        mUpdateImage = true;
    }

    void DraggableLabel::displayDebugImage(QImage image)
    {
        mNewDebugImage = image;
        mUpdateImage = true;
    }

    void DraggableLabel::timerEvent(QTimerEvent* event)
    {
        if (mUpdateImage)
        {
            mUpdateImage = false;
            this->repaint();
        }
    }

    void DraggableLabel::clearImage()
    {
        this->setPixmap(QPixmap());
        mNewImage = QImage();
        mNewDebugImage = QImage();
    }
}