/*
 Copyright (c) 2012, Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _Dac_H__
#define _Dac_H__

#include <QObject>

namespace tracker
{
    class Dac : public QObject
    {
        //Q_OBJECT;

    public:
        static Dac* makeDac(QObject* parent);

        Dac(QObject* parent)
            : QObject(parent)
            { }

        virtual ~Dac()
            { }

        virtual void setvoltage(float) = 0;
        virtual float readvoltage() const = 0;
        virtual void startdactimer() = 0;
        virtual void stopdactimer() = 0;
        //virtual void timerEvent(QTimerEvent* event) = 0;
    signals:
        //void pressurestepped(float);
    };
}

#endif /* _Dac_H__ */