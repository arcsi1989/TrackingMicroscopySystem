#ifndef PROFILER_H
#define PROFILER_H

#include <iostream>

#include "Timing.h"

namespace tracker {
    class Profiler {
    public:
        void start()    { startTime = clock.getTime(); }
        void end()      { endTime = clock.getTime(); }
        quint64 startTime;
        quint64 endTime;
    private:
        tracker::HPClock clock;
    };
}

#define PROFILER_START()    do { \
    tracker::Profiler __prof; \
    __prof.start()
#define PROFILER_END()      \
    __prof.end(); \
    std::cout << "PROFILE: " << __FUNCTION__ << "() = " \
        << __prof.endTime - __prof.startTime << "us" << std::endl; } while (0)

#endif // PROFILER_H
