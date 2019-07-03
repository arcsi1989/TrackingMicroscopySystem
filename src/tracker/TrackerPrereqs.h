/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Contains all the necessary forward declarations for all classes and structs.
*/

#ifndef _TrackerPrereqs_H__
#define _TrackerPrereqs_H__

#include "TrackerConfig.h"


//-----------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------

namespace tracker
{
    class Core;
    class Exception;
    class FrameCounter;
    class HPClock;
    class Logger;
    struct LogEntry;
    class OutputHandler;
    class OutputListener;
    class PathConfig;
    class Runnable;
    template <class T>
    class Singleton;
    class Thread;

    class Camera;
    class PressureSensor;
    class SerialInterface;
    class Stage;
    class Dac;

    class Controller;
    class Correlator;
    class BaseImage;
    class CorrelationImage;

    class CurveFitter;
    class FocusTracker;
    struct FocusValue;

    class DraggableLabel;
    class MainWindow;
    class TimeSpinBox;

#ifdef TRACKER_DUMMY
    class DummyCamera;
    class DummyMicroscope;
    class DummyStage;
#endif
}

namespace Loki
{
    class ScopeGuardImplBase;
    typedef const ScopeGuardImplBase& ScopeGuard;
}

// wxCTB
class wxSerialPort;

// Gauge
class QtSvgDialGauge;

// std::auto_ptr
namespace std     { template <class> class auto_ptr; }
namespace tracker { using std::auto_ptr; }

#endif /* _TrackerPrereqs_H__ */
