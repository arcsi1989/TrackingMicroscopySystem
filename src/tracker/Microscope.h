/*
 Copyright (c) 2013, Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef __MICROSCOPE_H__
#define __MICROSCOPE_H__

#include "ahm.h"  // include AHM header
#include "ahwbasic.h" // include basic control interfaces (BasicControlValue, MetricsConverters, MetricsConverter)
#include "ahwmic.h"// include microscope specific definitions
#include "ahwmicpropid.h"
#include <QObject>
#include <TrackerPrereqs.h>
#include <iostream>
#include <vector>
#include <string>
#include <ahwprop2.h>

namespace tracker
{
    /** This is the interface class between the tracker and the Leica DM5500
	It uses the abstract hardware model (Ahm) of Leica. (See "The Abstract Hardware Model Programmer's Guide)"
	
	\n*/

	class ahmMicroscope// : public QObject
    {
       //Q_OBJECT;

      public:

        /**
         * @enum Illumination
         * @brief Defines the two contrast methods, used by the microscope.
         */
        enum Illumination{ ilWhiteLight = 100,		/**< Normal white light */
                           ilFluorescence = 205};	/**< Fluorescent */

        //bool minitialized;									///< flag to check initialization
        //int *pzpos;

        /*
        ahm::Unit* mrootunit;									///< Pointer to AHM root unit
        ahm::Unit* mmicroscope;									///< Pointer to AHM microscope unit
        ahm::Unit* milturret;									///< Pointer to AHM IL turret unit
        ahm::Unit* mflshutter;									///< Pointer to AHM shutter unit
        ahm::Unit* mtlshutter;									///< Pointer to AHM TL shutter unit
        ahm::Unit* mattenuator;									///< Pointer to AHM light attenuator unit
        ahm::Unit* mlamp;										///< Pointer to AHM lamp unit

        ahm::Interface*mflshutterinterf;					///< Pointer to AHM IL shutter interface
        ahm::BasicControlValue* mflshuttercontrol;			///< Pointer to AHM IL shutter control value

        ahm::Interface* mtlshutterinterf;					///< Pointer to AHM TL shutter interface
        ahm::BasicControlValue* mtlshuttercontrol;			///< Pointer to AHM TL shutter control value
		
        ahm::Interface*mattenuatorinterf;					///< Pointer to AHM attenuator interface
        ahm::BasicControlValue* mattenuatorcontrol;			///< Pointer to AHM attenuator control value
        
        ahm::Interface* mlampinterf;						///< Pointer to AHM lamp interface
        ahm::BasicControlValue* mlampcontrol;				///< Pointer to AHM lamp control value
		
        int mflshutterstate;
        int mtlshutterstate;
        int mattenuatorstate;
        int mlampstate;
        */
        ahmMicroscope();
        ~ahmMicroscope();

        /** Get the microscope contrasting methods.
         *
         * @return
         */
        int getContrastingMethod(void) const;

        /**
         * @brief Return state of the shutter (open/close)
         * @return Shutter open=1/close=0
         */
        int getFlShutterState(void) const;

    public slots:
      /**
       Changes the cube in the microscope turret in the direction specified (1 is up, -1 is down)
       */
      void moveturret(int dir);
      /**
       Changes the cube in the microscope turret to the position specified
       */
      void setturretposition(int pos);
      /**
       Returns the current cube position
       */
      int getturretposition(void);
		
      void openShutter(void);
       
      void closeShutter(void);

      void toggleShutter(void);

      void setShutter(bool state);

      /**
        Sets tht the light intensity to the value intensity with the attenuator
      */
      void setintensity(int intensity);
      void changeintensity(int direction);
      int getintensity(void);
      /**
        Changes the contrast method from IL to TL or vice versa
      */
      void changemethod(void);

      /**
       * @brief Set transmitted light as the illumination method.
       */
      void transmittedIllumination(void);

      /**
       * @brief Set incident light as the illumination method.
       */
      void incidentIllumination(void);

private:
      bool initialize(void);

      ahm::Unit* findUnit(ahm::Unit* pUnit, ahm::TypeId typeId);

      bool                      minitialized;       ///< flag to check initialization

      ahm::Unit*                mrootunit;          ///< Pointer to AHM root unit
      ahm::Unit*                mmicroscope;        ///< Pointer to AHM microscope unit
      ahm::Unit*                milturret;          ///< Pointer to AHM IL turret unit
      ahm::Unit*                mflshutter;         ///< Pointer to AHM shutter unit
      ahm::Unit*								mtlshutter;									///< Pointer to AHM TL shutter unit
      ahm::Unit*                mattenuator;        ///< Pointer to AHM light attenuator unit
      ahm::Unit*                mlamp;              ///< Pointer to AHM lamp unit

      ahm::Interface*           mflshutterinterf;   ///< Pointer to AHM shutter interface
      ahm::BasicControlValue*   mflshuttercontrol;  ///< Pointer to AHM shutter control value

      ahm::Interface* mtlshutterinterf;							///< Pointer to AHM TL shutter interface
      ahm::BasicControlValue* mtlshuttercontrol;		///< Pointer to AHM TL shutter control value

      ahm::Interface*           mattenuatorinterf;  ///< Pointer to AHM attenuator interface
      ahm::BasicControlValue*   mattenuatorcontrol; ///< Pointer to AHM attenuator control value

      ahm::Interface*           mlampinterf;        ///< Pointer to AHM lamp interface
      ahm::BasicControlValue*   mlampcontrol;       ///< Pointer to AHM lamp control value

      int mflshutterstate;
      int mtlshutterstate;
      int mattenuatorstate;
      int mlampstate;
    };
}

#endif // __MICROSCOPE_H__
