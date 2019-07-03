#include "WindowsDac.h"

namespace tracker
{
    /*static*/ Dac* Dac::makeDac(QObject* parent)
    {
        return new WindowsDac(parent);
    }

    WindowsDac::WindowsDac(QObject* parent)
        : Dac(parent)
    {
        DAQmxCreateTask("ao",&(this->aotaskHandle));
        DAQmxCreateAOVoltageChan(aotaskHandle,"Dev1/ao0","",0,5.0,DAQmx_Val_Volts,"");
        DAQmxCreateTask("ai",&(this->aitaskHandle));
        DAQmxCreateAIVoltageChan(aitaskHandle,"Dev1/ai0","",DAQmx_Val_Cfg_Default,0,10.0,DAQmx_Val_Volts,NULL);

        timeinterval = 1000;
        steps = 6;
        stepsize = 5; //stepsize in kpa
        tickcounter=0;
        dactimerID=0;
    }

    WindowsDac::~WindowsDac()
    {
        if (this->aotaskHandle != 0) {
            DAQmxStopTask(this->aotaskHandle);
            DAQmxClearTask(this->aotaskHandle);
        }
        if (this->aitaskHandle != 0) {
            DAQmxStopTask(this->aitaskHandle);
            DAQmxClearTask(this->aitaskHandle);
        }
    }

    void WindowsDac::stopdactimer()
    {
        this->killTimer(dactimerID);
    }

    void WindowsDac::startdactimer()
    {
        dactimerID = this->startTimer(timeinterval);
    }

/*void WindowsDac::timerEvent(QTimerEvent* event)
{
    if((tickcounter/steps)%2)
    {
    setvoltage((100-((tickcounter%steps)*stepsize))/20);//set the voltage to create pressure oscillation
    emit pressurestepped(stepsize);
    }
else
{
    setvoltage((100-(steps*stepsize-(tickcounter%steps)*stepsize))/20);
    emit pressurestepped(-stepsize);
}
tickcounter = tickcounter +1;
}*/

void WindowsDac::setvoltage(float voltage){
//#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else
    //-DAQmxStartTask(aotaskHandle);
    int         error=0;
    //TaskHandle    taskHandle=0;
    char        errBuff[2048]={'\0'};
    float64 data[1] = {voltage};

    /*********************************************/
    // DAQmx Configure Code
    /*********************************************/
    //DAQmxErrChk (
    ///DAQmxCreateTask("",&taskHandle);//);
    //DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle,"Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,""));
    //DAQmxErrChk (
    ///DAQmxCreateAOVoltageChan(taskHandle,"Dev1/ao0","",0,5.0,DAQmx_Val_Volts,"");//);

    /*********************************************/
    // DAQmx Start Code
    /*********************************************/
    //DAQmxErrChk (
    ///DAQmxStartTask(taskHandle);//);

    /*********************************************/
    // DAQmx Write Code
    /*********************************************/
    //DAQmxErrChk (


    DAQmxWriteAnalogF64(aotaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,data,NULL,NULL);//);

/*Error:
    if( DAQmxFailed(error) )
        DAQmxGetExtendedErrorInfo(errBuff,2048);
    if( taskHandle!=0 ) {
        /*********************************************/
        // DAQmx Stop Code
        /*********************************************/
        /*DAQmxStopTask(taskHandle);
        DAQmxClearTask(taskHandle);
    }
    //if( DAQmxFailed(error) )
        printf("DAQmx Error: %s\n",errBuff);*/

    //printf("End of program, press Enter key to quit\n");
    //getchar();
    //return 0;

}
float WindowsDac::readvoltage() const
{
    int32 read;
    float64 data[1];


    /*********************************************/
    // DAQmx Start Code
    /*********************************************/

    DAQmxStartTask(aitaskHandle);

    /*********************************************/
    // DAQmx Read Code
    /*********************************************/

    DAQmxCfgSampClkTiming(aitaskHandle,"",10000.0,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,1);

    DAQmxReadAnalogF64(aitaskHandle,1,10.0,DAQmx_Val_GroupByChannel,data,1,&read,NULL);

    return (float)data[0];

}

}
