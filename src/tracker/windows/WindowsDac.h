#ifndef _WindowsDac_H__
#define _WindowsDac_H__

#include "Dac.h"
#include <NIDAQmx.h>

namespace tracker
{
    class WindowsDac : public Dac
    {
    public:
        WindowsDac(QObject* parent);
        ~WindowsDac();
		/** sets a voltage at AO0 of the DAC		*/
        void setvoltage(float);
		/** reads the voltage at AI0 of the DAC		*/
        float readvoltage() const;
        void startdactimer();
		void stopdactimer();
        //void timerEvent(QTimerEvent* event);

	private:
        TaskHandle aotaskHandle;
        TaskHandle aitaskHandle;
        int steps;
        float stepsize;
        int timeinterval;
        int tickcounter;
        int dactimerID;
    };
}

#endif /* _WindowsDac_H__ */