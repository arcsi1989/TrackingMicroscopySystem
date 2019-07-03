/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _WindowsStage_H__
#define _WindowsStage_H__

#include "TrackerPrereqs.h"

#include "Stage.h"

namespace tracker
{
    /// Implementation of an interface to an OASIS controlled Stage.
    class WindowsStage : public Stage
    {
        Q_OBJECT;

    public:
        /// Opens the OASIS driver connection
        WindowsStage(QObject* parent = 0);
        /// Closes the OASIS driver connection
        ~WindowsStage();

        bool isMovingXY();
        bool isMovingZ();


    public slots:
        /** Move the stage in X/Y (distance in microns).

            See Stage::move() for details.
        */
        bool move(QPointF distance, int block);

        /** Move the stage in Z (distance in microns).

            See Stage::moveZ() for details.
        */
        bool moveZ(double distance, int block);

        /** Move to stage to an absolute position in Z.

            See Stage::moveToZ() for details.
        */
        bool moveToZ(double zPos, int block);

        //! Reads the z position of the stage
        double getZpos();
        //! Reads the x position of the stage
        double getXpos();
        //! Reads the y position of the stage
        double getYpos();

		void clearZlimits();
		void setZlimits(double position);

        void stopAll();

    private:
        WindowsStage(const WindowsStage&); //!< Don't use (undefined symbol)

        bool mbDriverOpen;  ///< Tells whether driver opening was successful

        ///< Direction of the previous Z stage movement (-1.0 for down, 1.0 for up)
        double mPrevZDirection;
    };
}

#endif /* _WindowsStage_H__ */
