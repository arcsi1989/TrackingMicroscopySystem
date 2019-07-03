#include "ahm.h"  // include AHM header
#include "ahwbasic.h" // include basic control interfaces (BasicControlValue, MetricsConverters, MetricsConverter)
#include "ahwmic.h"// include microscope specific definitions
#include "ahwmicpropid.h"
#include <QObject>
#include <TrackerPrereqs.h>
#include <iostream>


//using namespace std;
using namespace ahm;   // direct use of AHM classes
//using namespace tracker;
// forward
/*void initialize();
void workWithZDrive(Unit* pRootUnit);
Unit *findUnit(Unit *pUnit, ahm::TypeId typeId);*/
namespace tracker
{
    class Microscope {//: public QObject{
       //Q_OBJECT;


     public:

        int minitialized;
        int *pzpos;


        Microscope();
        ~Microscope();
        Unit* initialize(void);

        int isready(void);


    public slots:

        void moveturret(int position);
        int getturretposition(void);
        void openshutter(void);
        void closeshutter(void);

    };




}