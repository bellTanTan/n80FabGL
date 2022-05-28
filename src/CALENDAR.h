/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on PD1990.c in n80pi20210814.tar.gz
  (http://home1.catvmics.ne.jp/~kanemoto/dist/n80pi20210814.tar.gz) 


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
#include <time.h>
#include <sys/time.h>


class CALENDAR
{
public:
  bool reset( bool enableFloppyDisk = false, bool enablePrinter = false )
  {
    m_IoIn40      = 0x0F;
    m_cstrb       = 0;
    m_cclk        = 0;
    m_shiften     = 0;
    m_cmd         = 0;
    m_datain      = 0;
    m_inReg40bit  = 0;
    m_outReg40bit = 0;
    m_year        = 1970;
    if ( enableFloppyDisk )
      m_IoIn40 &= (uint8_t)( ~0x08 );
    if ( enablePrinter )
      m_IoIn40 &= (uint8_t)( ~0x01 );
    return true;
  }

  void setReadIO( int address, int value )
  {
    if ( address == 0x40 )
    {
      m_IoIn40 = value;
    }
  }

  bool readIO( int address, int * result )
  {
    if ( address == 0x40 )
    {
      *result = m_IoIn40;
      return true;
    }
    return false;
  }

  bool writeIO( int address, int value )
  {
    if ( address == 0x10 )
    {
      m_datain = value & 0x08;
      m_cmd    = value & 0x07;
      return true;
    }
    if ( address == 0x40 )
    {
      calender_mode( value );
      return true;
    }
    return false;
  }

private:
  void calender_mode( int value )
  {
    // strobe signal check
    uint8_t ah = value;
    uint8_t bl = ah & 0x02;
    uint8_t al = m_cstrb & 0x02;
    al ^= bl;
    // strobe edge detect
    if ( al != 0 )
    {
      m_cstrb = bl;
      // strobe edge H -> L
      if ( bl == 0 )
      {
        // uPC1990 command
        switch ( m_cmd )
        {
          case 0x00: // Register Hold : DATA OUT = 1Hz
            m_shiften = 0;
            break;
          case 0x01: // Register Shift : DATA OUT = [LSB]
            m_shiften = 1;
            break;
          case 0x02: // Time Set & Counter Hold : DATA OUT = [LSB]
            m_shiften = 0;
            SetDateTime();
            SetTimeSecLSB();
            break;
          case 0x03: // Time Read : DATA OUT = 0.5Hz
            m_shiften = 0;
            GetDateTime();
            SetTimeSecLSB();
            break;
          case 0x04: // TP = 64Hz Set
          case 0x05: // TP = 256Hz Set
          case 0x06: // TP = 2048Hz Set
          case 0x07: // TEST MODE Set
          default:
            break;
        }
      }
    }
    // CLK strobe check
    ah = value & 0x04;
    if ( m_shiften == 0 )
    {
      m_cclk = ah;
      return;
    }
    al = m_cclk & 0x04;
    al ^= ah;
    if ( al == 0 )
      return;
    // CLK strobe edge (L -> H) detect
    m_cclk = ah;
    if ( ah == 0 )
    {
      if ( m_datain )
        m_inReg40bit |= (uint64_t)0x10000000000;
      m_inReg40bit >>= 1;
      m_outReg40bit >>= 1;
      SetTimeSecLSB();
    }
  }

  uint8_t bin2bcd( uint16_t data )
  {
    uint16_t d0 = data % 100;
    uint16_t d1 = d0 / 10;
    uint16_t d2 = d0 % 10;
    d1 <<= 4;
    d1 += d2;
    uint8_t result = d1 & 0xFF;
    return result;
  }

  uint16_t bcd2bin( uint8_t data )
  {
    uint16_t d0 = data / 16;
    uint16_t d1 = data % 16;
    d0 *= 10;
    d0 += d1;
    uint16_t result = d0;
    return result;
  }

  void  GetDateTime( void )
  {
    struct timeval tval;
    struct tm *nowLocalTime;
    gettimeofday( &tval, NULL );
    nowLocalTime = localtime( &tval.tv_sec );
    m_year   = nowLocalTime->tm_year + 1900;
    int mon  = nowLocalTime->tm_mon + 1;
    int wday = bin2bcd( nowLocalTime->tm_wday );
    int mday = bin2bcd( nowLocalTime->tm_mday );
    int hour = bin2bcd( nowLocalTime->tm_hour );
    int min  = bin2bcd( nowLocalTime->tm_min );
    int sec  = bin2bcd( nowLocalTime->tm_sec );
    m_outReg40bit = mon;
    m_outReg40bit <<= 4;
    m_outReg40bit |= wday;
    m_outReg40bit <<= 8;
    m_outReg40bit |= mday;
    m_outReg40bit <<= 8;
    m_outReg40bit |= hour;
    m_outReg40bit <<= 8;
    m_outReg40bit |= min;
    m_outReg40bit <<= 8;
    m_outReg40bit |= sec;
  }

  void SetDateTime( void )
  {
    struct timeval tval;
    struct tm nowLocalTime;
    memset( &tval, 0, sizeof( tval ) );
    memset( &nowLocalTime, 0, sizeof( nowLocalTime ) );
    int sec  = m_inReg40bit & 0xFF;
    m_inReg40bit >>= 8;
    int min  = m_inReg40bit & 0xFF;
    m_inReg40bit >>= 8;
    int hour = m_inReg40bit & 0xFF;
    m_inReg40bit >>= 8;
    int mday = m_inReg40bit & 0xFF;
    m_inReg40bit >>= 8;
    int wday = m_inReg40bit & 0x0F;
    m_inReg40bit >>= 4;
    int mon  = m_inReg40bit & 0x0F;
    nowLocalTime.tm_year = m_year - 1900;
    nowLocalTime.tm_mon  = mon - 1;
    nowLocalTime.tm_wday = bcd2bin( wday );
    nowLocalTime.tm_mday = bcd2bin( mday );
    nowLocalTime.tm_hour = bcd2bin( hour );
    nowLocalTime.tm_min  = bcd2bin( min );
    nowLocalTime.tm_sec  = bcd2bin( sec );
    tval.tv_sec = mktime( &nowLocalTime );
    settimeofday( &tval, NULL );
  }

  void  SetTimeSecLSB( void )
  {
    uint8_t value = m_IoIn40;
    value &= 0xEF;
    if ( m_outReg40bit & 0x01 )
      value |= 0x10;
    m_IoIn40 = value;
  }

  uint8_t   m_IoIn40;       // I/O input port 40h
  uint8_t   m_cstrb;        // calender command strobe    2:ON, 0:OFF
  uint8_t   m_cclk;         // calender shift clock       4:ON, 0:OFF
  uint8_t   m_shiften;      // shift enable flag          1:ENABLE, 0:DISABLE
  uint8_t   m_cmd;          // calender command           0:Register Hold : DATA OUT = 1Hz
                            //                            1:Register Shift : DATA OUT = [LSB]
                            //                            2:Time Set & Counter Hold : DATA OUT = [LSB]
                            //                            3:Time Read : DATA OUT = 0.5Hz
                            //                            4:TP = 64Hz Set
                            //                            5:TP = 256Hz Set
                            //                            6:TP = 2048Hz Set
                            //                            7:TEST MODE Set
  uint8_t   m_datain;       // calender data bit          8:ON, 0:OFF
  uint64_t  m_inReg40bit;   // in  40bit shift register
  uint64_t  m_outReg40bit;  // out 40bit shift register
  int       m_year;         // gettimeofday()/settimeofday()
};
