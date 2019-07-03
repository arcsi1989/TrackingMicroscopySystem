/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _Thread_H__
#define _Thread_H__

#include "TrackerPrereqs.h"

#include <QThread>

namespace tracker
{
    /** Abstract base for objects of classes that partially live in a
        different thread. \n
        That means thread safety is also the caller's problem: always use
        signal/slot connections for public member calls when the object is
        'running'.
    */
    class Runnable : public QObject
    {
        Q_OBJECT;
        friend class Thread;

    public:
        virtual ~Runnable()
            { }

    signals:
        //! To be called within the thread to end it safely
        void quit();

    private:
        //! Actual thread starts here @param thread The underlaying QThread
        virtual void run(Thread* thread) = 0;
    };

    //! QThread wrapper that incorporates Runnable
    class Thread : public QThread
    {
        Q_OBJECT;

    public:
        /** Adds itself to the parent and makes the quit() connection.
        @param object
            The Runnable object that is associated with this thread
        @param parent
            Parent Qt object
        */
        Thread(Runnable* object, QObject* parent)
            : QThread(parent)
            , mObject(object)
            { this->connect(object, SIGNAL(quit()), SLOT(quit())); }

        //! Overrides QThread::run() and calls Runnable::run()
        void run()
            { mObject->run(this); }

        //! Starts the event loop (doesn't end until QThread::quit() is called)
        void exec()
            { QThread::exec(); }

    private:
        Q_DISABLE_COPY(Thread);

        Runnable* mObject; //!< The Runnable object associated with this thread (exactly one)
    };
}

#endif /* _Thread_H__ */
