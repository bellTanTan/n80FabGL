/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on CMT.c in n80pi20210814.tar.gz
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

#include "emu.h"


class CMT
{
public:
  bool reset( uint8_t * vram, int cmtBuffSize, uint8_t * cmtBuffer )
  {
    m_cmtMode    = false;
    m_reset      = false;
    m_vram       = vram;
    m_buffsize   = 0;
    m_buff       = cmtBuffer;
    m_index      = 0;
    m_basCmtSave = false;
    m_monCmtSave = false;
    setScreenMode( 0 );
    return true;
  }

  bool readIO( int address, int * result )
  {
    if ( address == 0x20 )
    {
      if ( m_cmtMode )
        cmt_read( result );
      else
      {
#ifndef _DEBUG
        if ( Serial.available() )
          *result = Serial.read();
        else
#endif // !_DEBUG
          *result = 0x00;
      }
      return true;
    }
    if ( address == 0x21 )
    {
      if ( m_cmtMode )
        *result = 0x07;
      else
      {
        *result = 0x85;
#ifndef _DEBUG
        *result |= ( Serial.available() ? 0x02 : 0x00 );
#endif // !_DEBUG
      }
      return true;
    }
    return false;
  }

  bool writeIO( int address, int value )
  {
    if ( address == 0x20 )
    {
      if ( m_cmtMode )
        cmt_write( value );
#ifndef _DEBUG
      else
        Serial.write( value );
#endif // !_DEBUG
      return true;
    }
    if ( address == 0x21 )
    {
      if ( m_cmtMode )
        cmt_cmd( value );
      return true;
    }
    if ( address == 0x30 )
    {
      if ( value & 0x20 )
      {
        m_cmtMode = false;
        m_reset   = false;
      }
      else
      {
        m_cmtMode = true;
        m_reset   = true;
      }
    }
    return false;
  }

  void setScreenMode( int value )
  {
    switch ( value )
    {
      case 0: // 40x20
      case 1: // 40x25
        m_statusAdrs = 31 * 2;
        break;
      case 2: // 80x20
      case 3: // 80x25
        m_statusAdrs = 71;
        break;
    }
  }

  int getCmtBufferSize( void )              { return m_buffsize; }
  void setCmtBufferSize( int value )        { m_index = 0; m_buffsize = value; }
  void resetCmtBufferIndex( void )          { m_index = 0; }
  void resetBasCmtSave( void )              { m_basCmtSave = false; }
  void resetMonCmtSave( void )              { m_monCmtSave = false; }
  bool getBasCmtSave( void )                { return m_basCmtSave; }
  bool getMonCmtSave( void )                { return m_monCmtSave; }
  uint16_t getMonCmtSaveStartAdrs ( void )  { return m_monCmtSaveStartAdrs; }
  uint16_t getMonCmtSaveEndAdrs( void )     { return m_monCmtSaveEndAdrs; }

private:
  void cmt_read( int * result )
  {
    if ( m_reset == true )
    {
      m_reset = false;
      *result = 0xFF;
      return;
    }
    if ( m_buff == NULL )
    {
      *result = 0xFF;
      return;
    }
    if ( m_index >= m_buffsize )
      m_index = 0;
    *result = m_buff[ m_index++ ];
  }

  void cmt_cmd( int value )
  {
    if ( value == 0x40 )
    {
      m_reset = true;
      if ( m_buff )
        m_index = 0;
    }
#ifndef DONT_CMT_SAVE_INDICATOR
    if ( value == 0x11 )
      m_vram[ m_statusAdrs ] = '#';
#endif // !DONT_CMT_SAVE_INDICATOR
  }

  void cmt_write( int value )
  {
#ifndef DONT_CMT_SAVE_INDICATOR
    uint8_t code = m_vram[ m_statusAdrs ];
    if ( code != '*' )
      code = '*';
    else
      code = ' ';
    m_vram[ m_statusAdrs ] = code;
#endif // !DONT_CMT_SAVE_INDICATOR
    m_buff[ m_index++ ] = value;
    writeEndCheck();
  }

  void writeEndCheck( void )
  {
    if ( m_index == 1 )
    {
      m_basCmtSave = false;
      m_monCmtSave = false;
      m_basCmtSaveCheck = ( ( m_buff[0] == 0xD3 ) ? true : false );
      m_monCmtSaveCheck = ( ( m_buff[0] == 0x3A ) ? true : false );
      if ( m_monCmtSaveCheck )
      {
        m_monCheckSize = 4;
        m_monCmtSaveCheckSeqNo = 10;
      }
    }
    if ( m_basCmtSaveCheck )
    {
      bool fHead = false;
      bool fEnd = false;
      if ( m_index > ( 10 + 12 ) )
      {
        uint8_t buf[16];
        memset( buf, 0xD3, 10 );
        fHead = ( memcmp( &m_buff[0], buf, 10 ) == 0 ? true : false );
        memset( buf, 0x00, 12 );
        int p = m_index - 12;
        fEnd  = ( memcmp( &m_buff[p], buf, 12 ) == 0 ? true : false );
      }
      if ( fHead == true && fEnd == true )
      {
        m_buffsize = m_index;
        m_basCmtSaveCheck = false;
        m_basCmtSave = true;
      }
    }
    if ( m_monCmtSaveCheck )
    {
      int p = m_index - 1;
      switch ( m_monCmtSaveCheckSeqNo )
      {
        case 10:
          if ( m_index == m_monCheckSize )
          {
            m_monCmtSaveStartAdrs  = (uint16_t)m_buff[ p - 2 ] << 8 | m_buff[ p - 1 ];
            m_monCmtSaveEndAdrs    = m_monCmtSaveStartAdrs;
            m_monCmtSaveCheckSeqNo = 30;
          }
          break;
        case 20:
          if ( m_index == m_monCheckSize )
          {
            m_monCmtSaveCheckSeqNo = 30;
          }
          break;
        case 30:
          if ( m_buff[p] == 0x3A )
            m_monCmtSaveCheckSeqNo = 40;
          break;
        case 40:
          if ( m_buff[p] == 0x00 )
            m_monCmtSaveCheckSeqNo = 50;
          else
          {
            m_monCmtSaveEndAdrs += m_buff[p];
            m_monCheckSize = m_index + m_buff[p] + 1;
            m_monCmtSaveCheckSeqNo = 20;
          }
          break;
        case 50:
          if ( m_buff[p] == 0x00 )
          {
            m_buffsize = m_index;
            m_monCmtSaveEndAdrs--;
            m_monCmtSaveCheckSeqNo = 0;
            m_monCmtSaveCheck = false;
            m_monCmtSave = true;
          }
          break;
      }
    }
  }

  bool        m_reset;                // reset flag
  uint8_t *   m_buff;                 // cmt data buff ptr
  uint32_t    m_buffsize;             // cmt data buff size
  uint32_t    m_index;                // buff current offset ptr
  bool        m_cmtMode;              // flag : cmt(true) , sio(false)
  bool        m_basCmtSave;           // flag : BASIC language type CMT buffer save completion
  bool        m_monCmtSave;           // flag : Machine language type CMT buffer save completion
  bool        m_basCmtSaveCheck;      // flag : BASIC language type CMT buffer save completion check
  bool        m_monCmtSaveCheck;      // flag : Machine language type CMT buffer save completion check
  int         m_monCmtSaveCheckSeqNo; // Machine language type CMT buffer save sequence processing number
  int         m_monCheckSize;         // Machine language type CMT buffer save sequence block check byte size
  uint16_t    m_monCmtSaveStartAdrs;  // Machine language type CMT save start address
  uint16_t    m_monCmtSaveEndAdrs;    // Machine language type CMT save end address
  uint8_t *   m_vram;                 // v-ram pointer
  int         m_statusAdrs;           // Status address of csave or mon W execution
};
