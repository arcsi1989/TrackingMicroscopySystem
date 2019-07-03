#pragma once

#include <vector>

#include <QObject>
#include <QThread>
#include "Timing.h"

#include "SerialInterface.h"

namespace tracker
{
  /** This is the interface class between the tracker and the Lamp
   */
  class Lamp : public QObject, public SerialInterface
  {
    Q_OBJECT;

    public:
            Lamp(QObject* parent = 0);
            ~Lamp();

            bool setFreqBandAndIntensity(int band, double intensity);
            bool startMP1(int band, double intensity, unsigned int t_on, unsigned int t_off, unsigned int iterations);

            void measureTurnaroundTime();
            void measureAckTime();

    public slots:
            void measureTime();

    private:
          const int runnum;
          char cPingChar;
          std::vector<double> vRunTimes;
          HPClock mClock;

          void analyzeOutput(std::vector<double> &vRunTimes);
  };

  class LampSetFreqBandAndIntensityWorkerThread : public QThread
  {
      Q_OBJECT
      void run() {
          bool res = m_lamp->setFreqBandAndIntensity(m_band, m_intensity);
          emit finishedSetFreqBandAndIntensity(res);
      }

  public:
      void setData(Lamp* lamp, int band, double intensity) {
          m_lamp = lamp;
          m_band = band;
          m_intensity = intensity;
      }

  signals:
      void finishedSetFreqBandAndIntensity(bool);

  private:
      Lamp* m_lamp;
      int m_band;
      double m_intensity;
  };

  class LampMP1WorkerThread : public QThread
  {
      Q_OBJECT
      void run() {
          bool res = m_lamp->startMP1(m_band, m_intensity, m_t_on, m_t_off, m_iterations);
          emit finishedMP1(res);
      }

  public:
      void setData(Lamp* lamp, int band, double intensity, unsigned int t_on, unsigned int t_off, unsigned int iterations) {
          m_lamp = lamp;
          m_band = band;
          m_intensity = intensity;
          m_t_on = t_on;
          m_t_off = t_off;
          m_iterations = iterations;
      }

  signals:
      void finishedMP1(bool);

  private:
      Lamp* m_lamp;
      int m_band;
      double m_intensity;
      unsigned int m_t_on;
      unsigned int m_t_off;
      unsigned int m_iterations;
  };
}
