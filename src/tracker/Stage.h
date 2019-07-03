/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _Stage_H__
#define _Stage_H__

#include "TrackerPrereqs.h"

#include <QPointF>
#include <QObject>

namespace tracker
{
    /** Abstract class defining the interface to the movable stage.
        You can use \ref signalslotspage "Signals and Slots" for move() and
        stopAll() as well as call them directly (if you are in the same thread!).
    */
    class Stage : public QObject
    {
        Q_OBJECT;

    public:
        //! Returns a new stage (defined in source file of the stage implementation)
        static Stage* makeStage(QObject* parent = 0);

        Stage(QObject* parent = 0)
            : QObject(parent)
            { }

        virtual ~Stage()
            { }

        //! Indicates whether the stage is currently moving in x or y direction
        virtual bool isMovingXY() = 0;
        //! Indicates whether the stage is currently moving in z direction
        virtual bool isMovingZ() = 0;


    public slots:
        /** Moves the stage (in micrometers).
         * @param distance
         *   Distance to move the stage in X/Y.
         * @param block
         *   If set to 0, the function issues the move command to the stage and
         *   immediately returns (i.e. the stage might still be moving on
         *   return). If set to 1, the function blocks until the stage has
         *   finished moving. Default value is 0.
         */
        virtual bool move(QPointF distance, int block = 0) = 0;

        /** Moves the stage in z direction (in micrometers)
         *
         * @param distance
         *   Distance to move the stage in Z.
         * @param block
         *   If set to 0, the function issues the move command to the stage and
         *   immediately returns (i.e. the stage might still be moving on
         *   return). If set to 1, the function blocks until the stage has
         *   finished moving. Default value is 0.
         */
        virtual bool moveZ(double distance, int block = 0) = 0;

        /** Move stage to absolute Z position. */
        virtual bool moveToZ(double zPos, int block = 0) = 0;

        //! Reads the stage Zposition(in micrometers)
        virtual double getZpos() = 0;
        //! Reads the stage Xposition(in micrometers)
        virtual double getXpos() = 0;
        //! Reads the stage Yposition(in micrometers)
        virtual double getYpos() = 0;

        //! Immediately stops all stage movements and empties any command queues
        virtual void stopAll() = 0;

		//! Clears limits of Z movement
		virtual void clearZlimits()=0;
		//! Sets limts of Z movement
		virtual void setZlimits(double position)=0;

    private:
        Q_DISABLE_COPY(Stage);
    };
}

#endif /* _Stage_H__ */
