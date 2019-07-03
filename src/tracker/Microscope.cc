/*
 Copyright (c) 2013, Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/
#include "Microscope.h"

namespace tracker {

ahmMicroscope::ahmMicroscope(void)//: QObject(parent)
{
minitialized = this->initialize();//minitialized = 0;
}

ahmMicroscope::~ahmMicroscope(){

    //mflshutter->dispose();
    //mmicroscope->dispose();
    //milturret->dispose();
    //
    mrootunit->dispose();
    theHardwareModel()->dispose();

}

ahm::Unit *ahmMicroscope::findUnit(ahm::Unit *pUnit, ahm::TypeId typeId){
    // test unit's type for given typeId
    if(pUnit && pUnit->type()){
        if(pUnit->type()->isA(typeId)){
            return pUnit; // ok it is!
        }
        else {
            if(pUnit->units()){
                // recursively find unit in child units
                for(iop::int32 i = 0;i<pUnit->units()->numUnits();i++){
                    ahm::Unit *pDeepUnit = findUnit(pUnit->units()->getUnit(i), typeId);
                    if(pDeepUnit){
                        return pDeepUnit; // stop recursion
                    }
                }
            }
        }
    }
    return 0; // unit with type id was not found
}
//--
template <class T>
T* find_itf(ahm::Unit *pUnit, iop::int32 iid){
    if(pUnit && pUnit->interfaces()){
        ahm::Interface* pInterface = pUnit->interfaces()->findInterface(iid);
        return pInterface? (T*) pInterface->object() : 0;
    }
    return 0;
}

iop::string safe(iop::string sz){ return sz?sz:""; }

void split(std::vector<std::string> & vstrsplit, const std::string & str, const std::string& strdelims){
    vstrsplit.clear();
    std::string::size_type i, i0;
    i=i0=0;
    bool flagDone = false;
    while(i<str.length() && (i=str.find_first_of(strdelims, i0))!= std::string::npos){
        vstrsplit.push_back( str.substr(i0, i -i0));
        i++;
        while(i<str.length() && strdelims.find_first_of(str[i])==i){// consume delims
            i++;
        }
        i0 = i;
    }

    if(i0 < str.length()){
        vstrsplit.push_back( str.substr(i0));
    }
}

class MethodIds : public ahm::IdList {
public:
    MethodIds(ahm::MicroscopeContrastingMethods *pMethods, iop::string szMethods){
        if(pMethods){
            if(pMethods->supportedMethods()){
                for(iop::int32 i=0;i<pMethods->supportedMethods()->numIds();i++) m_vids.push_back(pMethods->supportedMethods()->getId(i));
            }

            if(pMethods->methodNames() && szMethods){
                m_vids.clear();
                std::vector<std::string> vstr;
                split(vstr, szMethods, ";,");
                for(std::vector<std::string>::size_type i=0;i<vstr.size();i++){
                    iop::int32 id = pMethods->methodNames()->findId(vstr[i].c_str());
                    if(id>=0) m_vids.push_back(id);
                }
            }
        }
    }
    virtual iop::int32 numIds(){
        return (iop::int32) m_vids.size();
    }
    virtual iop::int32 getId(iop::int32 index){
        return m_vids[index];
    }

    virtual iop::int32 findId(iop::int32 id){
        for(iop::int32 i=0;i<numIds();i++){
            if(getId(i)==id) return i;
        }
        return -1;
    }

private:
    std::vector<iop::int32> m_vids;
};
// end tools

class MyUnitBase{
public:
    bool isMethodSupported(iop::string szmethod, iop::int32 index){
        std::string strMethods;
        if(szmethod){// && getMethodProperty(strMethods, index)){
            std::vector<std::string> vstr;
            split(vstr, strMethods, ";");
            for(std::vector<std::string>::size_type i=0;i<vstr.size();i++){
                if(vstr[i] == std::string(szmethod)){
                    return true;
                }
            }
        }
        return false;
    }
    //virtual bool getMethodProperty(std::string &strbuf, iop::int32 index) = 0;
    //virtual bool getNameProperty(std::string &strbuf, iop::int32 index) = 0;

    virtual iop::int32 minPos(){
        return m_pbcv ? m_pbcv->minControlValue():0;
    }

    virtual iop::int32 maxPos(){
        return m_pbcv ? m_pbcv->maxControlValue():0;
    }

    virtual iop::int32 current(){
        return m_pbcv ? m_pbcv->getControlValue():0;
    }

    virtual void moveto(iop::int32 pos){
        if(m_pUnit && m_pUnit->type() && m_pUnit->type()->isA(ahm::MICROSCOPE_MOTORIZED_UNIT)){
            if(m_pbcv)m_pbcv->setControlValue(pos);
        }
        else {
            char szbuf[128];
            do {
                std::cout << "pos is now: " << current() << std::endl;
                szbuf[0] = '\0';
                if(current()!=pos){
                    std::cout << std::endl << "Please move to position" << pos << ">";
                    std::cin.getline(szbuf, sizeof(szbuf)-1);
                }
            }while(current()!=pos || szbuf[0] =='x');
        }
    }
    virtual iop::string name(){ return safe(m_pUnit ? m_pUnit->name():0); }
protected:
    MyUnitBase(ahm::Unit *pUnit) : m_pUnit(pUnit){
        m_pProperties = find_itf<ahm::Properties>(pUnit, ahm::IID_PROPERTIES);
        m_pbcv = find_itf<ahm::BasicControlValue>(pUnit, ahm::IID_BASIC_CONTROL_VALUE);
    }
protected:
    //virtual ahm::Properties* getIndexStruct(iop::int32 index) = 0;
    ahm::Unit *m_pUnit;
    ahm::Properties *m_pProperties;
    ahm::BasicControlValue* m_pbcv;
};


/*
class MyNosepiece : public MyUnitBase{
public:
    MyNosepiece(ahm::Unit*pUnit) : MyUnitBase(pUnit){}

    virtual bool getMethodProperty(std::string &strbuf, iop::int32 index){
        return prop_tools::getStringValue(this->getIndexStruct(index), PROP_METHODS, strbuf);
    }
    virtual bool getNameProperty(std::string &strbuf, iop::int32 index){
        std::string strname, straper;
        if(prop_tools::getStringValue(this->getIndexStruct(index), PROP_MAGNIFICATION, strname)
            && prop_tools::getStringValue(this->getIndexStruct(index), PROP_APERTURE, straper)
        ){
                strbuf = strname + string("x/") + straper;
                return true;
        }
        return false;
    }

    bool isDryObjective(iop::int32 i){
        std::string strtype;
        if(prop_tools::getStringValue(this->getIndexStruct(i), PROP_OBJECTIVE_TYPE, strtype)){

            if(!strtype.empty()){
                if(strtype[0] == 'D'){
                    return true;
                }
                else {
                    iop::int32 nFlag=0;
                    if(prop_tools::getIntValue(this->getIndexStruct(i), PROP_OBJECTIVE_TYPE_COMBI_FLAG, nFlag)){
                        return nFlag!=0;
                    }
                }
            }
        }
        return false;
    }
protected:
    virtual ahm::Properties* getIndexStruct(iop::int32 index){
        if(m_pProperties){
            ahm::Property *pProp = m_pProperties->findProperty(PROP_NOSEPIECE);
            ahm::PropertyValue *pElem = prop_tools::getIndexValue(pProp, index);
            if(prop_tools::properties(pElem)){
                pProp = prop_tools::properties(pElem)->findProperty(PROP_OBJECTIVE);
                return prop_tools::properties(pProp);
            }
        }
        return 0;
    }
};
*/

class MyILTurret : public MyUnitBase{
public:
    MyILTurret(ahm::Unit*pUnit) : MyUnitBase(pUnit){}

    /*virtual bool getMethodProperty(std::string &strbuf, iop::int32 index){
        return prop_tools::getStringValue(this->getIndexStruct(index), PROP_ILTURRET_CUBE_METHODS, strbuf);
    }*/
    /*virtual bool getNameProperty(std::string &strbuf, iop::int32 index){
        std::string strname;
        if(prop_tools::getStringValue(this->getIndexStruct(index), PROP_ILTURRET_CUBE_NAME, strname)
        ){
                strbuf = strname ;
                return true;
        }

        return false;
    }*/

protected:
    /*virtual ahm::Properties* getIndexStruct(iop::int32 index){
        if(m_pProperties){
            ahm::Property *pProp = m_pProperties->findProperty(PROP_ILTURRET);
            ahm::PropertyValue *pElem = prop_tools::getIndexValue(pProp, index);
            if(prop_tools::properties(pElem)){
                pProp = prop_tools::properties(pElem)->findProperty(PROP_ILTURRET_CUBE);
                return prop_tools::properties(pProp);
            }
        }
        return 0;
    }*/
};

void testProperties(MyUnitBase& u){
    std::cout << std::endl << u.name() << std::endl;
    for(iop::int32 i=u.minPos();i<=u.maxPos();i++){
        std::string str;
        std::cout << i << ":";
        //if(u.getNameProperty(str,i)) cout << ' ' << str.c_str();
        //if(u.getMethodProperty(str,i)) cout << ' ' << str.c_str();
        std::cout << std::endl;
    }
}
/*
void test_by_methods(MicroscopeContrastingMethods* pMethods, MyNosepiece &nsp, MyILTurret &ilt, ahm::IdList *pTestMethods){
    if(!pTestMethods) pTestMethods = pMethods ? pMethods->supportedMethods() :0 ;

    if(pMethods && pTestMethods && pMethods->methodNames()){
        cout << endl << " - driven by methods " << endl;
        for(iop::int32 i=0;i<pTestMethods->numIds(); i++){
            iop::int32 methodId = pTestMethods->getId(i);
            iop::string szMethod = pMethods->methodNames()->findName(methodId);
            cout << "testing method " << methodId << " " << safe(szMethod) << endl;

            cout << "switching method...";
            pMethods->setContrastingMethod(methodId);
            cout << "done" << endl;

            cout << "now moving to all possible objectives" << endl;
            for(iop::int32 j=nsp.minPos(); j<=nsp.maxPos();j++){
                std::string str;
                nsp.getNameProperty(str, j);
                cout << j << ": " << str.c_str() << " ";
                if(nsp.isMethodSupported(szMethod, j)){
                    if(!nsp.isDryObjective(j)){
                        cout << "testing only DRY!" << endl;
                        continue;
                    }
                    cout << "OK"<< endl;

                    cout << "moving to " << j << "...";
                    nsp.moveto(j);
                    cout << "OK" << endl;
                    cout << "now testing all ILTurret Cubes" << endl;
                    for(iop::int32 k=ilt.minPos();k<ilt.maxPos();k++){
                        str.clear();
                        ilt.getNameProperty(str,k);
                        cout << "    " << k << ": " << str.c_str() << " ";
                        if(ilt.isMethodSupported(szMethod, k)){
                            cout << "OK" << endl;
                            cout << "    moving to " << k << "...";
                            ilt.moveto(k);
                            cout << "OK" << endl;
                        }
                        else cout << '-' << endl;
                    }
                }
                else cout << "-" << endl;
            }
        }
    }
}

void test_by_objectives(MicroscopeContrastingMethods* pMethods, MyNosepiece &nsp, MyILTurret &ilt, ahm::IdList *pTestMethods){
    if(!pTestMethods) pTestMethods = pMethods ? pMethods->supportedMethods() :0 ;

    if(pMethods && pTestMethods && pMethods->methodNames()){
        cout << endl << " - driven by objectives " << endl;
        for(iop::int32 j=nsp.minPos(); j<=nsp.maxPos();j++){
            std::string str;
            nsp.getNameProperty(str, j);
            cout << j << ": " << str.c_str() << " ";
            if(!nsp.isDryObjective(j)){
                cout << "testing only DRY!" << endl;
                continue;
            }
            cout << "OK"<< endl;
            cout << "moving to " << j << "...";
            nsp.moveto(j);
            cout << "OK" << endl;

            cout << " now testing all test methods" << endl;
            for(iop::int32 i=0;i<pTestMethods->numIds(); i++){
                iop::int32 methodId = pTestMethods->getId(i);
                iop::string szMethod = pMethods->methodNames()->findName(methodId);
                cout << "testing method " << methodId << " " << safe(szMethod) << endl;
                if(nsp.isMethodSupported(szMethod, j)){
                    cout << "switching method...";
                    pMethods->setContrastingMethod(methodId);
                    cout << "done" << endl;
                    cout << "now testing all ILTurret Cubes" << endl;
                    for(iop::int32 k=ilt.minPos();k<ilt.maxPos();k++){
                        str.clear();
                        ilt.getNameProperty(str,k);
                        cout << "    " << k << ": " << str.c_str() << " ";
                        if(ilt.isMethodSupported(szMethod, k)){
                            cout << "OK" << endl;
                            cout << "    moving to " << k << "...";
                            ilt.moveto(k);
                            cout << "OK" << endl;
                        }
                        else cout << '-' << endl;
                    }
                }
                else cout << "-" << endl;
            }
        }
    }
}
*/
/*
void move_turrets(Unit * pRootUnit, iop::string szMethods){
    Unit* pILTurret = findUnit(pRootUnit, ahm::MICROSCOPE_IL_TURRET);
    Unit* pNosepiece = findUnit(pRootUnit, ahm::MICROSCOPE_NOSEPIECE);
    Unit* pMicroscope = findUnit(pRootUnit, ahm::MICROSCOPE);
    MicroscopeContrastingMethods* pMethods = find_itf<MicroscopeContrastingMethods>(pMicroscope, IID_MICROSCOPE_CONTRASTING_METHODS);
    if(pILTurret) cout << "IL_TURRET" << endl;
    if(pNosepiece) cout << "NOSEPIECE" << endl;
    if(pMicroscope) cout << "MICROSCOPE" << endl;
    if(pMethods) cout << "METHODS" << endl;

    MyNosepiece nsp(pNosepiece);
    MyILTurret ilt(pILTurret);

    cout << "Nosepiece pos: " << nsp.current() << endl;
    cout << "ILTurret pos: " << ilt.current() << endl;

    testProperties(nsp);
    testProperties(ilt);

    cout << "Running Method Test!" << endl;

    if(pMethods && pMethods->methodNames()){
        MethodIds methodIds(pMethods, szMethods);
        const int maxRuns = 2;

        for(iop::int32 nRun=0;nRun<maxRuns; nRun++){
            cout << endl << " *** RUN " << nRun << " ***" << endl;
            test_by_objectives(pMethods,nsp,ilt, &methodIds);
            test_by_methods(pMethods, nsp, ilt, &methodIds);
        }

    }
}*/
//--

bool ahmMicroscope::initialize(void){

//void main(int argc, char *argv[]){
    // load unit
    try {
        if(theHardwareModel()){
            // load either default unit or unit name was passed by command line arguments

            mrootunit = theHardwareModel()->getUnit("");
            //Unit *pRootUnit = theHardwareModel()->getUnit("");//argc>1?argv[1]:"");
            if(mrootunit){ // check if object is returned
                // work with unit
                std::cout << "Microscope detected." << std::endl;

                milturret = findUnit(mrootunit, ahm::MICROSCOPE_IL_TURRET);
                
				mmicroscope = findUnit(mrootunit, ahm::MICROSCOPE);
                
				mflshutter = findUnit(mrootunit, ahm::MICROSCOPE_IL_SHUTTER);
        mflshutterinterf = mflshutter->interfaces()->findInterface(ahm::IID_BASIC_CONTROL_VALUE);
        mflshuttercontrol= (ahm::BasicControlValue*)mflshutterinterf->object();
				mflshutterstate = mflshuttercontrol->getControlValue();

        mtlshutter = findUnit(mrootunit, ahm::MICROSCOPE_TL_SHUTTER);
        mtlshutterinterf = mtlshutter->interfaces()->findInterface(ahm::IID_BASIC_CONTROL_VALUE);
        mtlshuttercontrol = (ahm::BasicControlValue*)mtlshutterinterf->object();
        mtlshutterstate = mtlshuttercontrol->getControlValue();
				
				mattenuator =findUnit(mrootunit, ahm::MICROSCOPE_IL_ATTENUATOR);
                mattenuatorinterf = mattenuator->interfaces()->findInterface(ahm::IID_BASIC_CONTROL_VALUE);
                mattenuatorcontrol= (ahm::BasicControlValue*)mattenuatorinterf->object();
				mattenuatorstate = mattenuatorcontrol->getControlValue();
				
				
				mlamp = findUnit(mrootunit, ahm::MICROSCOPE_LAMP);
				//mtldiaphragm = findUnit(mrootunit, ahm::MICROSCOPE_TL_FIELD_DIAPHRAGM);
				mlampinterf = mlamp->interfaces()->findInterface(ahm::IID_BASIC_CONTROL_VALUE);
                mlampcontrol=(ahm::BasicControlValue*)mlampinterf->object();
				
				//Unit* pNosepiece = findUnit(pRootUnit, ahm::MICROSCOPE_NOSEPIECE);
                //MicroscopeContrastingMethods* pMethods = find_itf<MicroscopeContrastingMethods>(pMicroscope, IID_MICROSCOPE_CONTRASTING_METHODS);

                //ahm::BasicControlValue *pControl;

                //ahm::Interface *pInterface = mflshutter->interfaces()->findInterface(ahm::IID_BASIC_CONTROL_VALUE);
                
                
				//pControl = (BasicControlValue*)pInterface->object();
                

                //MyILTurret ilt(milturret);
				//ilt.moveto(1);
                
				//mflshutterstate = pControl->getControlValue(); // get Current control value

                //pControl->setControlValue(!nCurrentValue);

                //move_turrets(pRootUnit, argc>2?argv[2]:0);
                //MyILTurret ilt(pILTurret);
                

                return 1;//pRootUnit;//theHardwareModel()->getUnit("");
                // dispose unit - it is no longer needed here

                //pRootUnit->dispose();
            }
            else std::cout << "there is something strange: no Microscope was delivered" << std::endl;
        }
    }
    catch(ahm::Exception & ex){
        // a hardware model exception occured:
        std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
        std::cout << "Turn on or connect microscope !" << std::endl;
		return 0;
    }

return 0;
}

void ahmMicroscope::setShutter(bool state)
{    
    if(getContrastingMethod()==205){
        mflshuttercontrol->setControlValue(state);
        mflshutterstate = state;
    }
     if(getContrastingMethod()==100){
        mtlshuttercontrol->setControlValue(state);
        mtlshutterstate = state;
     }
}

void ahmMicroscope::openShutter()
{
  setShutter(true);
}

void ahmMicroscope::closeShutter()
{
  setShutter(false);
}

void ahmMicroscope::toggleShutter()
{
    setShutter(!mflshutterstate);
}

void ahmMicroscope::moveturret(int dir){
    int actpos, newpos,minpos, maxpos;
	MyILTurret ilt(milturret);
	minpos=ilt.minPos();
	maxpos=ilt.maxPos();
	actpos=ilt.current();
	
	if (dir==1)
		newpos = actpos+1;
	if (dir==-1)
		newpos = actpos-1;
	
	if (newpos > maxpos)
		newpos=minpos;
	if (newpos < minpos)
		newpos=maxpos;

	
	try{
		ilt.moveto(newpos);}
	catch(ahm::Exception & ex)
	{
    std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
    std::cout << "There is no cube inserted in slot " << newpos << "!" << std::endl;
	//ilt.moveto(maxpos);
	}
}
void ahmMicroscope::setturretposition(int pos){
    if(getContrastingMethod()==205){// only change turret in IL
		MyILTurret ilt(milturret);
		try{
			ilt.moveto(pos);}
		catch(ahm::Exception & ex)
		{
        std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
        std::cout << "There is no cube inserted in slot " << pos << "!" << std::endl;
		//ilt.moveto(maxpos);
		}
	}

}
int ahmMicroscope::getturretposition(void){
	if(minitialized){
		
		try{
			MyILTurret ilt(milturret);
			return ilt.current() ;
		}
		catch(ahm::Exception & ex){
        std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
        std::cout << "Could not get turret position!" << std::endl;
		return 0;
		}	
	}
	else
		return 0;
	
}

void ahmMicroscope::setintensity(int intensity)
{
	if (minitialized){
        if(getContrastingMethod()==205){
			try{
				mattenuatorcontrol->setControlValue(intensity);
				mattenuatorstate=intensity;
			}
			catch(ahm::Exception & ex)
			{
                std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
                std::cout << "Could not set IL intensity to level " << intensity << "!" << std::endl;
			}
		}
        if(getContrastingMethod()==100){
			try{
				mlampcontrol->setControlValue(intensity);
				mlampstate=intensity;
			}
			catch(ahm::Exception & ex)
			{
                std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
                std::cout << "Could not set TL intensity to level " << intensity << "!" << std::endl;
			}
		}

	}

}
void ahmMicroscope::changeintensity(int direction)
{
	if(minitialized){
        iop::int32 method = getContrastingMethod();
		if(method==205){ // IL
			int state, newstate, minval, maxval;
			state=mattenuatorcontrol->getControlValue();
			minval=mattenuatorcontrol->minControlValue();
			maxval=mattenuatorcontrol->maxControlValue();
			newstate=state+direction;
			if (newstate > 5)
			newstate=5;
			if (newstate <1)
			newstate=1;
			try{
				mattenuatorcontrol->setControlValue(newstate);}
			catch(ahm::Exception & ex)
			{
                std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
                std::cout << "There was an error with the light intensity, could not set intensity to " << newstate << "!" << std::endl;
				//ilt.moveto(maxpos);
			}
		}
		if(method==100){ // TL
			int state, newstate, minval, maxval;
			state=mlampcontrol->getControlValue();
			minval=mlampcontrol->minControlValue();
			maxval=mlampcontrol->maxControlValue();
			newstate=state+16*direction;// transmitted light has 256 steps of light intensity
			if (newstate > maxval)
				newstate=maxval;
			if (newstate < minval)
				newstate=minval;
			try{
				mlampcontrol->setControlValue(newstate);}
			catch(ahm::Exception & ex)
			{
                std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
                std::cout << "There was an error with the light intensity, could not set intensity to " << newstate << "!" << std::endl;
				//ilt.moveto(maxpos);
			}
		
		}
	}
}
int ahmMicroscope::getintensity(void){
    if (minitialized){
        iop::int32 method = getContrastingMethod();
        if(method==205){
			try{
				return mattenuatorcontrol->getControlValue();
			}
			catch(ahm::Exception & ex)
			{
                std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
                std::cout << "There was an error with the IL light intensity, could not read intensity!" << std::endl;
				return 0;
			}
		}
		
        if(method==100)
		{
			try{
				return mlampcontrol->getControlValue();
			}
			catch(ahm::Exception & ex)
			{
                std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
                std::cout << "There was an error with the TL light intensity, could not read intensity!" << std::endl;
				return 0;
			}
		}
	}
	return 0;
}

int ahmMicroscope::getContrastingMethod() const
{
	if(minitialized){
		iop::int32 method;
	
        ahm::MicroscopeContrastingMethods* pMethods;// = find_itf<MicroscopeContrastingMethods>(mmicroscope, IID_MICROSCOPE_CONTRASTING_METHODS);
		try{
            pMethods = find_itf<ahm::MicroscopeContrastingMethods>(mmicroscope, ahm::IID_MICROSCOPE_CONTRASTING_METHODS);
			method=pMethods->getContrastingMethod();
		}
		catch(ahm::Exception & ex)
		{
        std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
        std::cout << "There was an error with detecting the contrast method!" << std::endl;
        return 0;
        }
		return method;
	}
	return 0;
}

/**
 * @brief Return state of the shutter (open/close)
 * @return Shutter open=1/close=0
 */
int ahmMicroscope::getFlShutterState() const
{
    return mflshutterstate;
}

void ahmMicroscope::changemethod(void)
{	
    ahm::MicroscopeContrastingMethods* pMethods;
	
	if(minitialized){
    //MicroscopeContrastingMethods* pMethods;// = find_itf<MicroscopeContrastingMethods>(mmicroscope, IID_MICROSCOPE_CONTRASTING_METHODS);
    try{
      pMethods = find_itf<ahm::MicroscopeContrastingMethods>(mmicroscope, ahm::IID_MICROSCOPE_CONTRASTING_METHODS);
      //method=pMethods->getContrastingMethod();
    }catch(ahm::Exception & ex)
    {
      std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
      std::cout << "There was an error with changing the contrast method!" << std::endl;
      return ;
    }
	
    iop::int32 method = getContrastingMethod();
    if(method == ilFluorescence/*205*/){
      setturretposition(4);// go to empty cube
      pMethods->setContrastingMethod(100);
    }
    if(method == ilWhiteLight/*100*/){
      pMethods->setContrastingMethod(205);
    }
	}	
}

/**
 * @brief Set transmitted light as the illumination method.
 */
void ahmMicroscope::transmittedIllumination(void){
  ahm::MicroscopeContrastingMethods* pMethods;
  if(minitialized){
    try{
      pMethods = find_itf<ahm::MicroscopeContrastingMethods>(mmicroscope, ahm::IID_MICROSCOPE_CONTRASTING_METHODS);
    }catch(ahm::Exception & ex)
    {
      std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
      std::cout << "There was an error with setting transmitted light as illumination method!" << std::endl;
      return ;
    }

    iop::int32 method = getContrastingMethod();
    if(method != ilWhiteLight/*100*/){
      pMethods->setContrastingMethod(ilWhiteLight/*100*/);
    }
  }
}

/**
 * @brief Set incident light as the illumination method.
 */
void ahmMicroscope::incidentIllumination(void){
  ahm::MicroscopeContrastingMethods* pMethods;
  if(minitialized){
    try{
      pMethods = find_itf<ahm::MicroscopeContrastingMethods>(mmicroscope, ahm::IID_MICROSCOPE_CONTRASTING_METHODS);
    }catch(ahm::Exception & ex)
    {
      std::cout << "a hardware model exception occurred: error code: " << ex.errorClass()<< ", error code: " << ex.errorCode() << ", text: " << ex.errorText() << std::endl;
      std::cout << "There was an error with setting incident light as illumination method!" << std::endl;
      return ;
    }

    iop::int32 method = getContrastingMethod();
    if(method != ilFluorescence/*205*/){
      pMethods->setContrastingMethod(ilFluorescence/*205*/);
    }
  }

}

}
