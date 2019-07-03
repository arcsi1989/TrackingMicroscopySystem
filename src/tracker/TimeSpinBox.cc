/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "TimeSpinBox.h"

#include <cassert>
#include <cmath>
#include <QMouseEvent>
#include "Logger.h"

namespace tracker
{
    TimeSpinBox::TimeSpinBox(QWidget* parent)
        : QSpinBox(parent)
    {
        mDoubleValidator = new QDoubleValidator(this);
        mIntValidator = new QIntValidator(this);
    }

    QValidator::State TimeSpinBox::validate(QString& input, int& pos) const
    {
        if (input.size() > 1)
        {
            int last = input.size() - 1;
            if (input[last] == 's')
            {
                if (input[last - 1] == 'u' || input[last - 1] == QString::fromUtf8("\316\274")[0])
                    return this->validateInt(input.mid(0, input.size() - 2));
                if (input[last - 1] == 'm')
                    return this->validateDouble(input.mid(0, input.size() - 2));
                else
                    return this->validateDouble(input.mid(0, input.size() - 1));
            }
            if (input[last] == 'u' || input[last] == QString::fromUtf8("\316\274")[0])
                return (this->validateInt(input.mid(0, input.size() - 1)) == QValidator::Invalid) ? QValidator::Invalid : QValidator::Intermediate;
            if (input[last] == 'm')
                return (this->validateDouble(input.mid(0, input.size() - 1)) == QValidator::Invalid) ? QValidator::Invalid : QValidator::Intermediate;
            else
                return (this->validateDouble(input) == QValidator::Invalid) ? QValidator::Invalid : QValidator::Intermediate;
        }
        else if (input.size() == 1)
        {
            if (input[0] == 'u' ||
                input[0] == QString::fromUtf8("\316\274")[0] ||
                input[0] == 'm' ||
                input[0] == 's')
            {
                return QValidator::Intermediate;
            }
            else
                return (this->validateDouble(input) == QValidator::Invalid) ? QValidator::Invalid : QValidator::Intermediate;
        }
        else
            return QValidator::Intermediate;
    }

    int TimeSpinBox::valueFromText(const QString& text) const
    {
        if (text.endsWith("us") || text.endsWith(QString::fromUtf8("\316\274s")))
        {
            assert(text.size() > 2);
            mStepWidth = 1;
            return text.mid(0, text.size() - 2).toInt();
        }
        if (text.endsWith("ms"))
        {
            assert(text.size() > 2);
            mStepWidth = 1000;
            return mStepWidth * QLocale::system().toDouble(text.mid(0, text.size() - 2));
        }
        if (text.endsWith("s"))
        {
            assert(text.size() > 1);
            mStepWidth = 1000000;
            return mStepWidth * QLocale::system().toDouble(text.mid(0, text.size() - 1));
        }
        assert(false);
        return 0;
    }

    QString TimeSpinBox::textFromValue(int val) const
    {
        double unitValue; // Value in appropriate unit (like milliseconds)
        QString suffix;   // Unit suffix
        if (val >= 999500)     // Display in seconds
        {
            mStepWidth = 1000000;
            unitValue = val / (double)mStepWidth;
            if (unitValue >= 999.5)
                unitValue = 999.0; // Limit display
            suffix = "s";
        }
        else if (val >= 999) // Display in milliseconds
        {
            mStepWidth = 1000;
            unitValue = val / (double)mStepWidth;
            suffix = "ms";
        }
        else                   // Display in microseconds
        {
            mStepWidth = 1;
            // Return with zero precision because the time is actually in integer in microseconds
            return QString::number(val) + QString::fromUtf8("\316\274s");
        }

        assert(unitValue >= 0 && unitValue < 999.5);
        int precision;
        if (unitValue >= 99.95)
            precision = 0;
        else if (unitValue >= 9.995)
            precision = 1;
        else
            precision = 2;

        return QLocale::system().toString(unitValue, 'f', precision) + suffix;
    }

    void TimeSpinBox::stepBy(int steps)
    {
        if (steps < 0)
        {
            // For simplicity reasons, make single steps (this one counts too, hence -steps - 1)
            for (int i = 0; i < -steps - 1; ++i)
                this->stepBy(-1);

            // Prevent steps like (1ms - 1ms) = 0ms instead of 999us
            // But at the same time allow something like (1.5ms - 1ms) = 500us;
            if (this->value() - mStepWidth < mStepWidth / 2)
                QSpinBox::stepBy(-this->value() + mStepWidth - mStepWidth / 1000);
            else
                QSpinBox::stepBy(-mStepWidth);
        }
        else
            QSpinBox::stepBy(steps * mStepWidth);
    }

    QValidator::State TimeSpinBox::validateInt(QString text) const
    {
        int dummy = 0;
        return mIntValidator->validate(text, dummy);
    }

    QValidator::State TimeSpinBox::validateDouble(QString text) const
    {
        int dummy = 0;
        return mDoubleValidator->validate(text, dummy);
    }
}
