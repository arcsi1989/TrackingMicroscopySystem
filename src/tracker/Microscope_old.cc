#include "Microscope.h"


using namespace std;
namespace tracker{

Microscope::Microscope(){
minitialized = 0;
}

Microscope::~Microscope(){}

Unit* Microscope::initialize(void){

//void main(int argc, char *argv[]){
    // load unit
    try {
        if(theHardwareModel()){
            // load either default unit or unit name was passed by command line arguments
            Unit *pRootUnit = theHardwareModel()->getUnit("");//argc>1?argv[1]:"");
            if(pRootUnit){ // check if object is returned
                // work with unit

                //Unit* pILTurret = findUnit(pRootUnit, ahm::MICROSCOPE_IL_TURRET);
                //Unit* pNosepiece = findUnit(pRootUnit, ahm::MICROSCOPE_NOSEPIECE);
                /*Unit* pMicroscope = findUnit(pRootUnit, ahm::MICROSCOPE);
                MicroscopeContrastingMethods* pMethods = find_itf<MicroscopeContrastingMethods>(pMicroscope, IID_MICROSCOPE_CONTRASTING_METHODS);

                ahm::BasicControlValue *pControl;
                Unit* pShutter = findUnit(pRootUnit, ahm::MICROSCOPE_IL_SHUTTER);
                ahm::Interface *pInterface = pShutter->interfaces()->findInterface(ahm::IID_BASIC_CONTROL_VALUE);
                pControl = (BasicControlValue*)pInterface->object();
                //int nCurrentValue = pControl->getControlValue(); // get Current control value
                //pControl->setControlValue(!nCurrentValue);
                //move_turrets(pRootUnit, argc>2?argv[2]:0);
                MyILTurret ilt(pILTurret);
                ilt.moveto(1);*/

                // dispose unit - it is no longer needed here
                pRootUnit->dispose();
            }
            else cout << "there is something strange: no unit was delivered" << endl;
        }
    }
    catch(ahm::Exception & ex){
        // a hardware model exception occured:
        std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << endl;
    }
    cout << "DONE." << endl;
return 0;
}


//}

void Microscope::moveturret(int position){

//OI_StepZ(microns,0);
}
int Microscope::getturretposition(void){

return 0;
}

int Microscope::isready(){
return 1;}

}