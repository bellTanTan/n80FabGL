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


class PRINTER
{
public:
  bool reset( bool enablePrinter = false )
  {
    m_enablePrinter = enablePrinter;
    m_IoOut10 = 0x00;
    m_pstrb   = 0;
    return true;
  }

  bool readIO( int address, int * result )
  {
    return false;
  }

  bool writeIO( int address, int value )
  {
    if ( address == 0x10 )
    {
      if ( m_enablePrinter )
        m_IoOut10 = value;
      return true;
    }

    if ( address == 0x40 )
    {
      if ( m_enablePrinter )
      {
        // strobe signal check
        uint8_t ah = value;
        uint8_t bl = ah & 0x01;
        uint8_t al = m_pstrb & 0x01;
        al ^= bl;
        // strobe edge detect
        if ( al != 0 )
        {
          m_pstrb = bl;
          // strobe edge H -> L
          if ( bl == 0 )
            Serial.printf( "%c", m_IoOut10 );
        }
        return true;
      }
    }
    return false;
  }

private:
  uint8_t   m_IoOut10;        // I/O output port 10h
  uint8_t   m_pstrb;          // printer strobe 1:ON, 0:OFF
  bool      m_enablePrinter;  // flag : printer enable
};
