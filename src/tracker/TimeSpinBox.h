/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _TimeSpinBox_H__
#define _TimeSpinBox_H__

#include "TrackerPrereqs.h"

#include <QSpinBox>
#include <QDoubleValidator>
#include <QIntValidator>

namespace tracker
{
    /** Modified version of the QSpinBox that can nicely display time durations
        between 1 microsecond and 999 seconds.
        Specifically, times are displayed with the SI postfix. Examples:
         - "45us" --> 45 microseconds
         - "1.23s" --> 1.23 seconds

        The precision is always 3 digits in total except for microseconds where
        there is no dot at all (always precise). \n
        The internal format is always an integer number representing microseconds.
        It's only the display that has to do with milliseconds and seconds.
    @par Value ranges
        @code
        [0,          999] --> [1,    999]us
        [1000,    999500] --> [1.00, 999]ms
        [999500, INT_MAX] --> [1.00, 999]s
        @endcode
    */
    class TimeSpinBox : public QSpinBox
    {
        Q_OBJECT;

    public:
        TimeSpinBox(QWidget* parent = 0);

    protected:
        /** Returns whether the \c input string represents a valid time duration
            or an intermediate state of a valid string (different return value then)
        @param pos
            Position of the cursor (neglected)
        */
        QValidator::State validate(QString &input, int &pos) const;

        //! Returns the time duration in microseconds from a string
        int valueFromText(const QString &text) const;

        //! Formats a time duration in microseconds as string
        QString textFromValue(int val) const;

        /** Called when leaving the spin box with an 'Intermediate' text.
            Ignoring this function means using the last valid text.
        */
        void fixup(QString &str) const { }

        /** Performs \c steps in negative or positive direction
            (Hitting the up or down arrows will do exactly this in single steps)
        */
        void stepBy(int steps);

    private:
        Q_DISABLE_COPY(TimeSpinBox);

        //! Shortcut function to validate an integer number
        QValidator::State validateInt(QString text) const;
        //! Shortcut function to validate a double floating point number
        QValidator::State validateDouble(QString text) const;

        QDoubleValidator* mDoubleValidator;  //! Validator used for the doubles (> microseconds)
        QIntValidator*    mIntValidator;     //! Validator used for the integers (microseconds)
        mutable int       mStepWidth;        //! Current width of one single step in microseconds
    };
}

#endif /* _TimeSpinBox_H__ */
