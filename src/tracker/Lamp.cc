#include "Lamp.h"

#include <wxctb/serport.h>
#include <sstream>
#include <iostream>
#include <cmath>
#include <windows.h>
#include "Timing.h"

namespace tracker
{
    Lamp::Lamp(QObject* parent)
    : QObject(parent),
      //SerialInterface("com2", wxBAUD_115200),
      //SerialInterface("com2", 57600),
      SerialInterface("com2", wxBAUD_57600),
      //SerialInterface("com2", wxBAUD_38400),
      runnum(1000),
      cPingChar('G')
    {
        this->openConnection();
    }

    Lamp::~Lamp()
    {}

    bool Lamp::setFreqBandAndIntensity(int band, double intensity)
    {
        bool bFinished = false;
        char first_byte;
        char second_byte;
        char rcv_first_byte = 0;
        char rcv_second_byte = 0;

        int quantized_intensity = intensity / 100.0 * 0xfff;

        first_byte = (quantized_intensity << 4) & 0xf0;
        first_byte |= (band & 0x0f);

        second_byte = (quantized_intensity >> 4) & 0xff;

        mSerialPort->Writev(&first_byte, 1, wxTIMEOUT_INFINITY);
        mSerialPort->Writev(&second_byte, 1, wxTIMEOUT_INFINITY);

        while (!bFinished)
        {
          mSerialPort->Readv(&rcv_first_byte, 1, 4);

          if (rcv_first_byte == 0x00)
          {
            mSerialPort->Read(&rcv_first_byte, 1);
            mSerialPort->Read(&rcv_second_byte, 1);

            if (rcv_first_byte == first_byte && rcv_second_byte == second_byte) {
              std::cout << "Frequency band and intensity set." << std::endl;
              bFinished = true;
            }
            else
            {
              std::cout << "0x" << std::hex << rcv_first_byte << " , 0x" << std::hex << first_byte << std::endl;
              std::cout << "0x" << std::hex << rcv_second_byte << " , 0x" << std::hex << second_byte << std::endl;

              return false;
            }
          }
          else
          {
            std::cout << "0x" << std::hex << rcv_first_byte << std::endl;
          }
        }
    }

    bool Lamp::startMP1(int band, double intensity, unsigned int t_on, unsigned int t_off, unsigned int iterations)
    {
        bool bFinished = false;
        char ton = t_on;
        char toff = t_off;
        char first_byte;
        char second_byte;
        char it = iterations;
        char rcv_first_byte = 0;
        char rcv_bytes[6];

        int quantized_intensity = intensity / 100.0 * 0xfff;

        first_byte = (quantized_intensity << 4) & 0xf0;
        first_byte |= (band & 0x0f);

        second_byte = (quantized_intensity >> 4) & 0xff;

        char mp = 0x18;
        mSerialPort->Writev(&mp, 1, wxTIMEOUT_INFINITY);
        mSerialPort->Writev(&ton, 1, wxTIMEOUT_INFINITY);
        mSerialPort->Writev(&toff, 1, wxTIMEOUT_INFINITY);
        mSerialPort->Writev(&first_byte, 1, wxTIMEOUT_INFINITY);
        mSerialPort->Writev(&second_byte, 1, wxTIMEOUT_INFINITY);
        mSerialPort->Writev(&it, 1, wxTIMEOUT_INFINITY);

        while (!bFinished)
        {
          mSerialPort->Readv(&rcv_first_byte, 1, 10);

          if (rcv_first_byte == 0x00)
          {
            //std::cout << "Ack received." << std::endl;
            bFinished = true;
            mSerialPort->Readv(rcv_bytes, 6, 10000);

            if (rcv_bytes[0] == 0x18 && rcv_bytes[1] == t_on && rcv_bytes[2] == t_off && rcv_bytes[3] == first_byte && rcv_bytes[4] == second_byte && rcv_bytes[5] == iterations) {
              std::cout << "Micro program finished. - " << mClock.getTime()<<std::endl;
              bFinished = true;
            }
            else
            {
              std::cout << "0x" << std::hex << int(rcv_bytes[0]) << " , 0x18" << std::endl;
              std::cout << "0x" << std::hex << int(rcv_bytes[1]) << " , 0x" << std::hex << int(t_on) << std::endl;
              std::cout << "0x" << std::hex << int(rcv_bytes[2]) << " , 0x" << std::hex << int(t_off) << std::endl;
              std::cout << "0x" << std::hex << int(rcv_bytes[3]) << " , 0x" << std::hex << int(first_byte) << std::endl;
              std::cout << "0x" << std::hex << int(rcv_bytes[4]) << " , 0x" << std::hex << int(second_byte) << std::endl;
              std::cout << "0x" << std::hex << int(rcv_bytes[5]) << " , 0x" << std::hex << int(iterations) << std::endl;

              return false;
            }
          }
          else
          {
            std::cout << "0x" << std::hex << rcv_first_byte << std::endl;
          }
        }
      return true;
    }

    void Lamp::measureTime()
    {
        measureAckTime();
    }


    void Lamp::measureTurnaroundTime()
    {

      std::cout << "Start measuring time." << std::endl;

      LARGE_INTEGER frequency;        // ticks per second
      LARGE_INTEGER t1, t2;           // ticks
      double elapsedTime;

      // get ticks per second
      QueryPerformanceFrequency(&frequency);

      char readResult = '\0';
      for (int run = 0; run < runnum; run++)
      {
        bool bFinished = false;
        // start timer
        QueryPerformanceCounter(&t1);

        mSerialPort->Writev(&cPingChar, 1, wxTIMEOUT_INFINITY);
        while (!bFinished)
        {
          mSerialPort->Read(&readResult, 1);
          if (readResult != '\0')
          {
            if (readResult == cPingChar)
            {
              bFinished = true;
            }
            else
            {
              std::cout << readResult << std::endl;
            }
          }
        }

        // stop timer
        QueryPerformanceCounter(&t2);

        if (mSerialPort->Read(&readResult, 1))
        {
          do
          {
            std::cout << readResult;
          } while (mSerialPort->Read(&readResult, 1));
          std::cout << std::endl;
        }

        // compute the elapsed time in millisec
        elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
        vRunTimes.push_back(elapsedTime);

        std::stringstream ss;
        ss << "Run Number: " << run << std::endl;
        std::cout << ss.str();
      }

      this->closeConnection();
      analyzeOutput(vRunTimes);
    }

    void Lamp::measureAckTime()
    {

      std::cout << "Start measuring time." << std::endl;

      LARGE_INTEGER frequency;        // ticks per second
      LARGE_INTEGER t1, t2;           // ticks
      double elapsedTime;

      // get ticks per second
      QueryPerformanceFrequency(&frequency);

      char cPingChar = 0x00;
      char readResult = 0;
      for (int run = 0; run < runnum; run++)
      {
        bool bFinished = false;
        // start timer
        QueryPerformanceCounter(&t1);

        while (!bFinished)
        {
          bFinished = setFreqBandAndIntensity(1, 100);
        }

        // stop timer
        QueryPerformanceCounter(&t2);

        if (mSerialPort->Read(&readResult, 1))
        {
          do
          {
            std::cout << readResult;
          } while (mSerialPort->Read(&readResult, 1));
          std::cout << std::endl;
        }

        // compute the elapsed time in millisec
        elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
        vRunTimes.push_back(elapsedTime);

        /*
        std::stringstream ss;
        ss << "Run Number: " << run << std::endl;
        std::cout << ss.str();
        */
      }

      this->closeConnection();
      analyzeOutput(vRunTimes);
    }

    void Lamp::analyzeOutput(std::vector<double> &vRunTimes)
    {
      double dTimeSum = 0.0;
      double dTimeSqSum = 0.0;
      long iNum = 0;

      int nsMax = 0;

      //for (int time : vRunTimes)
      for (unsigned int i = 0; i < vRunTimes.size(); i++) {
        double time = vRunTimes[i];
        dTimeSum += time;
        dTimeSqSum += (time * time);
        iNum++;

        if (time > nsMax)
        {
          nsMax = time;
        }
      }

      double dAverage = dTimeSum / iNum;

      double dStandardDeviation = sqrt((dTimeSqSum / iNum) - dAverage*dAverage);

      std::cout << "Trial of " << std::dec << iNum << " messages finised." << std::endl;
      std::cout << "  Average turnaround time: " << dAverage*1000 << "us" << std::endl;
      std::cout << "  Turnaround Standard Deviation: " << dStandardDeviation*1000 << "us" << std::endl;
      std::cout << "  Longest Latency: " << std::dec << nsMax*1000 << "us" << std::endl;
    }
}
