/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on PCG.c in n80pi20210814.tar.gz
  (http://home1.catvmics.ne.jp/~kanemoto/dist/n80pi20210814.tar.gz) 

* Please contact trinity09181718@gmail.com if you need a commercial license.
* This software is available under GPL v3.

* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Founvalueion, either version 3 of the License, or
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

using namespace fabgl;

#define PCG_CTL_REG_BIT_HIADDR  (0x03)
#define PCG_CTL_REG_BIT_SPK0    (0x08)
#define PCG_CTL_REG_BIT_STROBE  (0x10)
#define PCG_CTL_REG_BIT_FNTSRC  (0x20)    // copy|move font bit pattern
#define PCG_CTL_REG_BIT_SPK1    (0x40)
#define PCG_CTL_REG_BIT_SPK2    (0x80)
#define PCG_8253_CTL_REG_SCx    (0xC0)    // affect channel
#define PCG_8253_CTL_REG_RWx    (0x30)    // mode of r/w counter value


class PCG
{
public:
  // callbacks
  typedef void (* setPCGModeCallback)( void * context, int value );
  typedef void (* updatePCGFontCallback)( void * context );

  PCG()
  {
    m_soundGenerator = NULL;
  }

  void setCallbacks( void * context, setPCGModeCallback setPCGMode, updatePCGFontCallback updatePCGFont )
  {
    m_context       = context;
    m_setPCGMode    = setPCGMode;
    m_updatePCGFont = updatePCGFont;
  }

  bool reset( FontInfo const * font, FontInfo const * fontPCG, SoundGenerator * soundGenerator )
  {
    m_pcg_lo  = 0;
    m_pcg_hi  = 0;
    m_pcg_dat = 0;
    memset( m_pcg_freq,   0, sizeof( m_pcg_freq ) );
    memset( m_timmr_hi,   0, sizeof( m_timmr_hi ) );
    memset( m_timmr_lo,   0, sizeof( m_timmr_lo ) );
    memset( m_timmr_mode, 0, sizeof( m_timmr_mode ) );
    memset( m_timmr_cont, 0, sizeof( m_timmr_cont ) );
    memset( m_speaker,    0, sizeof( m_speaker ) );

    m_param_hz = 3993600; // 3.9936MHz
    m_pcg_mode = 0;

    if ( m_soundGenerator )
    {
      sound_off( 0 );
      sound_off( 1 );
      sound_off( 2 );
      return true;
    }

    m_font    = font;
    m_fontPCG = fontPCG;

    m_soundGenerator = soundGenerator;
    m_wave[0] = new SquareWaveformGenerator();
    m_wave[1] = new SquareWaveformGenerator();
    m_wave[2] = new SquareWaveformGenerator();
    if ( !m_wave[0] || !m_wave[1] || !m_wave[2] ) return false;

    m_soundGenerator->attach( m_wave[0] );
    m_soundGenerator->attach( m_wave[1] );
    m_soundGenerator->attach( m_wave[2] );

    return true;
  }

  bool readIO( int address, int * result )
  {
    return false;
  }

  bool writeIO( int address, int value )
  {
    _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X PCG\r\n", __func__, __LINE__, address, value );
    hal_mode( address, value );
    return true;
  }

private:
  void set_pcg_freq( int ch )
  {
    uint16_t timer;
    uint32_t freq, prev;

    timer = ( ( (uint16_t)m_timmr_hi[ch]) << 8 ) + m_timmr_lo[ch];
    // freq = 4MHz(CPU clock) / i8253 Counter
    if ( timer == 0 )
      // 0除算防止
      timer = 1;
    freq = (uint32_t) ( (double)m_param_hz / (double)timer + 0.5 );
    prev = m_pcg_freq[ch];
    if ( prev != freq )
    {
      m_pcg_freq[ch] = (uint16_t)freq;
      if ( m_speaker[ch] == 1 )
      {
        sound_on( (uint16_t)freq, ch );
      }
    }
}

  void hal_mode( int address, int value )
  {
    uint16_t offset;
    int ch;

    switch ( address )
    {
      case 0x00:  // save PCG char value
        m_pcg_dat = value;
        break;
      case 0x01:  // PCG RAM low address
        m_pcg_lo = value;
        break;
      case 0x02:  // PCG sound on/off
        if ( value & PCG_CTL_REG_BIT_SPK0 )
        {
          if ( m_speaker[0] == 0 )
          {
            m_speaker[0] = 1;
            sound_on( m_pcg_freq[0], 0 );
          }
        }
        else
        {
          m_speaker[0] = 0;
          sound_off( 0 );
        }
        if ( value & PCG_CTL_REG_BIT_SPK1 )
        {
          if ( m_speaker[1] == 0 )
          {
            m_speaker[1] = 1;
            sound_on( m_pcg_freq[1], 1 );
          }
        }
        else
        {
          m_speaker[1] = 0;
          sound_off( 1 );
        }
        if ( value & PCG_CTL_REG_BIT_SPK2 )
        {
          if ( m_speaker[2] == 0 )
          {
            m_speaker[2] = 1;
            sound_on( m_pcg_freq[2], 2 );
          }
        }
        else
        {
          m_speaker[2] = 0;
          sound_off( 2 );
        }
        // PCG RAM high address
        if ( value & PCG_CTL_REG_BIT_STROBE )
        {
          // address A8, A9 bits
          m_pcg_hi = value & PCG_CTL_REG_BIT_HIADDR;
          offset = ( ( (uint16_t)m_pcg_hi ) << 8 ) + m_pcg_lo;
          uint8_t const * src = m_font->data;
          uint8_t *       des = (uint8_t *)m_fontPCG->data;
          if ( value & PCG_CTL_REG_BIT_FNTSRC )
            // copy value from font rom
            des[1024 + offset] = src[1024 + offset];
          else
            // set PCG font data
            des[1024 + offset] = m_pcg_dat;
          m_updatePCGFont( m_context );
        }
        break;
      case 0x03:  // PCG ON/OFF
        m_pcg_mode = ( value & 0x08 ) ? 1 : 0;
        m_setPCGMode( m_context, m_pcg_mode );
        break;
      case 0x0C:  // i8253 timmer #0 counter
      case 0x0D:  // i8253 timmer #1 counter
      case 0x0E:  // i8253 timmer #2 counter
        ch = address - 0x0C;
        switch ( m_timmr_mode[ch] )
        {
          case 3:  // load/store L/H order
            if ( --m_timmr_cont[ch] == 1 )
            {
              m_timmr_lo[ch] = value;
            }
            else
            {
              m_timmr_hi[ch] = value;
              m_timmr_cont[ch] = 2;
              set_pcg_freq( ch );
            }
            break;
          case 2:  // load/store H
            m_timmr_hi[ch] = value;
            set_pcg_freq( ch );
            break;
          case 1:  // load/store L
            m_timmr_lo[ch] = value;
            break;
        }
        break;
      case 0x0F:  // i8253 mode register
        // get timer ch #0, #1,#2
        ch = ( value & PCG_8253_CTL_REG_SCx ) >> 6;
        if ( ch < 3 )
        {
          // ignore if read-back command is set
          switch ( ( value & PCG_8253_CTL_REG_RWx ) >> 4 )
          {
            case 3:
              m_timmr_cont[ch] = 2;
              m_timmr_mode[ch] = 3;
              break;
            case 2:
              m_timmr_cont[ch] = 1;
              m_timmr_mode[ch] = 2;
              m_timmr_lo[ch]   = 0;
              break;
            case 1:
              m_timmr_cont[ch] = 1;
              m_timmr_mode[ch] = 1;
              m_timmr_lo[ch]   = 0;
              break;
            default:
              m_timmr_cont[ch] = 0;
              m_timmr_mode[ch] = 0;
              break;
          }
        }
        break;
    }
  }

  void sound_on( uint16_t freq, int ch )
  {
    m_wave[ch]->setFrequency( freq );
    m_wave[ch]->enable( true );
  }

  void sound_off( int ch )
  {
    m_wave[ch]->enable( false );
  }

  uint16_t    m_pcg_freq[3];    // PCG i8253 timmer #2 (adjust)
  uint8_t     m_timmr_hi[3];    // PCG i8253 low byte
  uint8_t     m_timmr_lo[3];    // PCG i8253 low byte
  uint8_t     m_timmr_mode[3];  // PCG i8253 mode register
  uint8_t     m_timmr_cont[3];
  uint8_t     m_pcg_lo;         // PCG RAM low address
  uint8_t     m_pcg_hi;         // PCG RAM high address
  uint8_t     m_pcg_dat;        // PCG char value
  uint8_t     m_speaker[3];     // PCG speaker on/off flags

  uint32_t    m_param_hz;       // PC-8001 clock
  uint8_t     m_pcg_mode;       // PCG-8100 mode 0:OFF 1:ON

  void *                  m_context;          // callbacks
  setPCGModeCallback      m_setPCGMode;       // callback : setPCGMode()
  updatePCGFontCallback   m_updatePCGFont;    // callback : updatePCGFont()
  FontInfo const *        m_font;             // PC-8001 font
  FontInfo const *        m_fontPCG;          // PCG font
  SoundGenerator *        m_soundGenerator;   // fabgl library : class SoundGenerator
  WaveformGenerator *     m_wave[3];          // fabgl library : class WaveformGenerator
};

