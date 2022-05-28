/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on FabGL/src/emudevs/graphicsadapter.h

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

#include <stdlib.h>
#include <stdint.h>

#include "fabgl.h"

using namespace fabgl;


class packeduPD3301adpter {

public:

  enum class Emulation {
    None,
    PC_Text_40x20_8Colors,
    PC_Text_40x25_8Colors,
    PC_Text_80x20_8Colors,
    PC_Text_80x25_8Colors,
  };


  packeduPD3301adpter();
  ~packeduPD3301adpter();

  void setEmulation( Emulation emulation );
  Emulation emulation()                       { return m_emulation; }

  bool enableVideo( bool value );

  void setVideoBuffer( void const * videoBuffer );

  void setFont( FontInfo const * font, FontInfo const * fontGraph, FontInfo const * fontPCG );
  void updatePCGFont( FontInfo const * fontPCG );
  void setCursorShape( int start, int end );
  void setCursorPos( int row, int column );
  void setCursorVisible( bool value )         { m_cursorVisible = value; }
  void enablePCG( bool value )                { m_enablePCG = value; }
  int getTextColumns()                        { return m_columns; }
  int getTextRows()                           { return m_rows; }
  int getGraphWidth()                         { return m_VGADCtrl.getViewPortWidth(); }
  int getGraphHeight()                        { return m_VGADCtrl.getViewPortHeight(); }
  bool VSync()                                { return m_VGADCtrl.VSync(); }

private:

  void cleanupFont();

  void freeLUT();
  void setupLUT();

  void setupEmulation( Emulation emulation );

  static void drawScanline_PC_Text_40x20_8Colors( void * arg, uint8_t * dest, int scanLine );
  static void drawScanline_PC_Text_40x25_8Colors( void * arg, uint8_t * dest, int scanLine );
  static void drawScanline_PC_Text_80x20_8Colors( void * arg, uint8_t * dest, int scanLine );
  static void drawScanline_PC_Text_80x25_8Colors( void * arg, uint8_t * dest, int scanLine );


  VGADirectController m_VGADCtrl;
  Emulation           m_emulation;
  uint8_t const *     m_videoBuffer;

  uint8_t *           m_rawLUT;

  uint32_t            m_frameCounter;

  FontInfo            m_font;
  FontInfo            m_fontGraph;
  FontInfo            m_fontPCG;  
  int16_t             m_columns;
  int16_t             m_rows;
  int16_t             m_cursorRow;
  int16_t             m_cursorCol;
  uint8_t             m_cursorStart;  // cursor shape scanline start
  uint8_t             m_cursorEnd;    // cursor shape scanline end
  bool                m_cursorVisible;
  uint8_t *           m_cursorGlyph;
  bool                m_enablePCG;
  bool                m_videoEnabled;
};
