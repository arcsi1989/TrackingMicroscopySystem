/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _DraggableLabel_H__
#define _DraggableLabel_H__

#include "TrackerPrereqs.h"

#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>

namespace tracker
{
    /** The DraggableLabel is a QLabel that supports mouse dragging, resizing
        by scrolling and displaying a second debug image. In short: a collection
        of the stuff required, but not something general purpose. \n
    @par Usage
        Embed it in your layout and connect the mouseDragged() event to act on it.
        The label itself doesn't perform any dragging action. Only the resizing
        via scroll wheel events is handled directly. \n
        Use displayImage() to display images that are only drawn with a
        specified update frequency (see setDisplayFrequency()).
    @note
        Zooming (resizing) is always proportional to the current size. The
        resulting offsets caused by the discretisation are compensated for.
    */
    class DraggableLabel : public QLabel
    {
        Q_OBJECT;

    public:
        //! Starts the update timer and sets the default frequency to 24Hz.
        DraggableLabel(QWidget* parent = 0, Qt::WindowFlags f = 0);

        /** Set the display update frequency. Bare in mind that updating the
            image takes a little time. That means you can easily call
            displayImage() extremely often without compromising performance
        @param value
            Display update frequency in Hertz
        */
        void setDisplayFrequency(double value);
        //! Returns the display update frequency. See setDisplayFrequency()
        double getDisplayFrequency() const
            { return mDisplayFrequency; }

        //! Sets size of the label relative to the image size. The allowed value range is [0.2, 5.0].
        void setZoom(double value);
        //! Returns the size relative to the image size
        double getZoom() const
            { return mZoom; }

    public slots:
        /** Stores a new image to be displayed in a buffer. It only gets drawn
            upon the update timer event. There is almost no performance penalty
            in calling this function too often with new images (or even the same).
        */
        void displayImage(QImage image);
        //! Displays a second image on the right. See displayImage()
        void displayDebugImage(QImage image);
        //! Overwrites the content of the label with background colour.
        void clearImage();

    signals:
        //! Occurs whenever the user dragged (click+move) the label content.
        void mouseDragged(QPoint delta);
        /** Fired when you change the zoom with setZoom(). This is basically
            just the return value of setZoom() (can be called asynchronously).
        */
        void zoomChanged(double);
		void labelrequestszmove(double);

    protected:
        virtual void mousePressEvent(QMouseEvent* ev);
        virtual void mouseMoveEvent(QMouseEvent* ev);
        virtual void wheelEvent(QWheelEvent* ev);
        virtual void paintEvent(QPaintEvent* ev);

    private:
        Q_DISABLE_COPY(DraggableLabel);

        //! Called by the QTimer started in the constructor
        void timerEvent(QTimerEvent* event);

        int         mTimerID;               //!< Reference to the timer object
        double      mDisplayFrequency;      //!< Label content refresh rate
        double      mZoom;                  //!< Relative label size
        QImage      mNewImage;              //!< Buffer for incoming images
        QImage      mNewDebugImage;         //!< Buffer for incoming debug images
        //! Indicates whether the image needs to be redrawn upon the timer event
        bool        mUpdateImage;

        QPoint      mLastPosition;          //!< Stores the last know mouse position
    };
}

#endif /* _DraggableLabel_H__ */
