/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

* Please contact trinity09181718@gmail.com if you need a commercial license.
* This software is available under GPL v3.

* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include <stdint.h>
#include <string.h>

#include "fabgl.h"
#include "fabutils.h"
#include "sampling_beepdata.h"

using namespace fabgl;

#define _BEEP   (0x20)


class BUZZER
{
public:
  BUZZER()
  {
    m_soundGenerator = NULL;
    m_beep           = NULL;
  }


  bool reset( SoundGenerator * soundGenerator )
  {
    if ( m_soundGenerator )
    {
      m_beep->enable( false );
      return true;
    }
    m_soundGenerator = soundGenerator;
    m_beep           = m_soundGenerator->playSamples( (int8_t *)&_beepData[0], sizeof( _beepData ), 100, -1 );
    if ( m_beep )
    {
      m_beep->enable( false );
      return true;
    }
    return false;
  }

  bool readIO( int address, int * result )
  {
    return false;
  }

  bool writeIO( int address, int value )
  {
    if ( address == 0x40 )
    {
      if ( value & _BEEP )
      {
        if ( m_beep )
        {
          if ( !m_beep->enabled() )
            m_beep->enable( true );
        }
      }
      else
      {
        if ( m_beep )
        {
          if ( m_beep->enabled() )
            m_beep->enable( false );
        }
      }
      return true;
    }
    return false;
  }

private:
  SoundGenerator    * m_soundGenerator;   // fabgl library : class SoundGenerator
  SamplesGenerator  * m_beep;             // fabgl library : class SamplesGenerator
};
