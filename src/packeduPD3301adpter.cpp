/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on FabGL/src/emudevs/graphicsadapter.cpp

  char attr : packed uPD3301 ≒ CGA
  b76543210
   |||||||+--- B
   ||||||+---- R
   |||||+----- G
   ||||+------ upper line
   |||+------- simple Graphics
   ||+-------- reverse
   |+--------- erase(secret)
   +---------- under line

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


#include "packeduPD3301adpter.h"


#pragma GCC optimize ("O2")


static const RGB222 CGAPalette[8] = {
  RGB222( 0, 0, 0 ),    // black
  RGB222( 0, 0, 3 ),    // blue
  RGB222( 0, 3, 0 ),    // green
  RGB222( 0, 3, 3 ),    // cyan
  RGB222( 3, 0, 0 ),    // red
  RGB222( 3, 0, 3 ),    // magenta
  RGB222( 3, 3, 0 ),    // yellow
  RGB222( 3, 3, 3 ),    // white
};


void packeduPD3301adpter::setupEmulation( Emulation emulation )
{
  static const struct {
    bool                 text;
    uint8_t              cursorStart;          // valid when text = true
    uint8_t              cursorEnd;            // valid when text = true
    uint8_t              scanlinesPerCallback;
    DrawScanlineCallback scanlineCallback;
    char const *         modeline;
    int16_t              viewportWidth;
    int16_t              viewportHeight;
    int16_t              addFontWidth;
    int16_t              addFontHeight;
  } emulationInfo[] = {
    // Emulation::PC_Text_40x20_8Colors (Packed uPD3301 ≒ CGA, Text Mode, 40x20x8)
    {  true, 0, 7, 2,      drawScanline_PC_Text_40x20_8Colors,  VGA_640x200_60HzD, 640, 200, 8, 2 },

    // Emulation::PC_Text_40x25_8Colors (Packed uPD3301 ≒ CGA, Text Mode, 40x25x8)
    {  true, 0, 7, 4,      drawScanline_PC_Text_40x25_8Colors,  VGA_640x200_60HzD, 640, 200, 8, 0 },

    // Emulation::PC_Text_80x20_8Colors (Packed uPD3301 ≒ CGA, Text Mode, 80x20x8)
    {  true, 0, 7, 2,      drawScanline_PC_Text_80x20_8Colors,  VGA_640x200_60HzD, 640, 200, 0, 2 },

    // Emulation::PC_Text_80x25_8Colors (Packed uPD3301 ≒ CGA, Text Mode, 80x25x8)
    {  true, 0, 7, 4,      drawScanline_PC_Text_80x25_8Colors,  VGA_640x200_60HzD, 640, 200, 0, 0 },
  };

  if ( emulation != Emulation::None )
  {
    auto info = emulationInfo + (int)emulation - 1;

    m_VGADCtrl.setDrawScanlineCallback( info->scanlineCallback, this );
    m_VGADCtrl.setScanlinesPerCallBack( info->scanlinesPerCallback );
    m_VGADCtrl.setResolution( info->modeline, info->viewportWidth, info->viewportHeight );

    if ( info->text )
    {
      setCursorShape( info->cursorStart, info->cursorEnd );
      m_columns = m_VGADCtrl.getViewPortWidth()  / ( m_font.width  + info->addFontWidth );
      m_rows    = m_VGADCtrl.getViewPortHeight() / ( m_font.height + info->addFontHeight );
    }
  }
}


packeduPD3301adpter::packeduPD3301adpter()
  : m_VGADCtrl( false ),
    m_emulation( Emulation::None ),
    m_videoBuffer( nullptr) ,
    m_rawLUT( nullptr ),
    m_cursorRow( 0 ),
    m_cursorCol( 0 ),
    m_cursorStart( 0 ),
    m_cursorEnd( 0 ),
    m_cursorVisible( false ),
    m_cursorGlyph( nullptr ),
    m_enablePCG( false) ,
    m_videoEnabled( true )
{
  m_font.data      = nullptr;
  m_fontGraph.data = nullptr;
  m_fontPCG.data   = nullptr;
  m_VGADCtrl.begin();
}


packeduPD3301adpter::~packeduPD3301adpter()
{
  enableVideo( false );
  cleanupFont();
  freeLUT();
  if ( m_cursorGlyph )
    heap_caps_free( m_cursorGlyph );
  m_cursorGlyph = nullptr;
}


bool packeduPD3301adpter::enableVideo( bool value )
{
  if ( value == m_videoEnabled )
    return m_videoEnabled;

  m_videoEnabled = value;

  if ( m_videoEnabled )
  {
    setupEmulation( m_emulation );
    m_VGADCtrl.run();
  }
  else
    m_VGADCtrl.end();

  return !m_videoEnabled;
}


void packeduPD3301adpter::setEmulation( Emulation emulation )
{
  if ( m_emulation != emulation )
  {
    bool videoWasEnabled = enableVideo( false );
    freeLUT();
    m_emulation = emulation;
    if ( m_emulation != Emulation::None )
    {
      setupEmulation( emulation );
      setupLUT();
      enableVideo( videoWasEnabled );
    }
  }
}


void packeduPD3301adpter::freeLUT()
{
  if ( m_rawLUT )
    heap_caps_free( m_rawLUT );
  m_rawLUT = nullptr;
}


void packeduPD3301adpter::setupLUT()
{
  switch ( m_emulation )
  {
    case Emulation::None:
      break;

    case Emulation::PC_Text_40x20_8Colors:
    case Emulation::PC_Text_40x25_8Colors:
    case Emulation::PC_Text_80x20_8Colors:
    case Emulation::PC_Text_80x25_8Colors:
      // each LUT item contains half pixel (an index to 8 colors palette)
      if ( !m_rawLUT )
        m_rawLUT = (uint8_t *)heap_caps_malloc( 8, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
      for ( int i = 0; i < 8; ++i )
        m_rawLUT[i] = m_VGADCtrl.createRawPixel( CGAPalette[i] );
      break;
  }
}


void packeduPD3301adpter::setVideoBuffer( void const * videoBuffer )
{
  m_videoBuffer = (uint8_t *)videoBuffer;
}


void packeduPD3301adpter::cleanupFont()
{
  if ( m_font.data )
    heap_caps_free( (void *)m_font.data );
  if ( m_fontGraph.data )
    heap_caps_free( (void *)m_fontGraph.data );
  if ( m_fontPCG.data )
    heap_caps_free( (void *)m_fontPCG.data );
  m_font.data      = nullptr;
  m_fontGraph.data = nullptr;
  m_fontPCG.data   = nullptr;
}


void packeduPD3301adpter::setFont( FontInfo const * font, FontInfo const * fontGraph, FontInfo const * fontPCG )
{
  cleanupFont();
  if ( font )
  {
    m_font = *font;
    // copy font data into internal RAM
    auto size = 256 * ( ( m_font.width + 7) / 8 ) * m_font.height;
    m_font.data = (uint8_t const *)heap_caps_malloc( size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
    memcpy( (void *)m_font.data, font->data, size );
  }
  if ( fontGraph )
  {
    m_fontGraph = *fontGraph;
    // copy font data into internal RAM
    auto size = 256 * ( ( m_fontGraph.width + 7) / 8 ) * m_fontGraph.height;
    m_fontGraph.data = (uint8_t const *)heap_caps_malloc( size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
    memcpy( (void *)m_fontGraph.data, fontGraph->data, size );
  }
  if ( fontPCG )
  {
    m_fontPCG = *fontPCG;
    // copy font data into internal RAM
    auto size = 256 * ( ( m_fontPCG.width + 7) / 8 ) * m_fontPCG.height;
    m_fontPCG.data = (uint8_t const *)heap_caps_malloc( size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
    memcpy( (void *)m_fontPCG.data, fontPCG->data, size );
  }
}


void packeduPD3301adpter::updatePCGFont( FontInfo const * fontPCG )
{
  if ( fontPCG )
  {
    auto size = 256 * ( ( m_fontPCG.width + 7) / 8 ) * m_fontPCG.height;
    memcpy( (void *)m_fontPCG.data, fontPCG->data, size );
  }
}


void packeduPD3301adpter::setCursorShape( int start, int end )
{
  if ( start != m_cursorStart || end != m_cursorEnd )
  {
    m_cursorStart = start;
    m_cursorEnd   = end;

    // readapt start->end to the actual font height to make sure the cursor is always visible
    if ( start <= end && end >= m_font.height )
    {
      int h = end - start;
      end   = m_font.height - 1;
      start = end - h;
    }

    int charWidthInBytes = ( m_font.width + 7 ) / 8;
    if ( !m_cursorGlyph )
      m_cursorGlyph = (uint8_t *)heap_caps_malloc( charWidthInBytes * m_font.height, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
    memset( m_cursorGlyph, 0, charWidthInBytes * m_font.height );
    if ( end >= start && start >= 0 && start < m_font.height && end < m_font.height )
      memset( m_cursorGlyph + ( start * charWidthInBytes ), 0xFF, ( end - start + 1 ) * charWidthInBytes );
  }
}


void packeduPD3301adpter::setCursorPos( int row, int column )
{
  m_cursorRow = row;
  m_cursorCol = column;
}


void IRAM_ATTR packeduPD3301adpter::drawScanline_PC_Text_40x20_8Colors( void * arg, uint8_t * dest, int scanLine )
{
  auto ga = (packeduPD3301adpter *)arg;

  constexpr int CHARWIDTH        = 8;
  constexpr int CHARHEIGHT       = 8;
  constexpr int CHARWIDTHINBYTES = (CHARWIDTH + 7) / 8;
  constexpr int CHARSIZEINBYTES  = CHARWIDTHINBYTES * CHARHEIGHT;
  constexpr int COLUMNS          = 40;
  constexpr int SCREENWIDTH      = 640;
  constexpr int LINES            = 4;

  if ( scanLine == 0 )
    ++ga->m_frameCounter;

  int charScanline = scanLine % ( CHARHEIGHT + 2 );
  int textRow      = scanLine / ( CHARHEIGHT + 2 );
  if ( charScanline != 0 )
    charScanline--;

  uint8_t const * fontData      = ga->m_font.data      + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataGraph = ga->m_fontGraph.data + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataPCG   = ga->m_fontPCG.data   + ( charScanline * CHARWIDTHINBYTES );

  uint8_t * rawLUT = ga->m_rawLUT;

  uint8_t const * curItem = ga->m_videoBuffer + ( textRow * COLUMNS * 2 );

  bool showCursor = ga->m_cursorVisible && ga->m_cursorRow == textRow && ( ( ga->m_frameCounter & 0x1F ) < 0x0F );
  int cursorCol = ga->m_cursorCol;

  for ( int textCol = 0; textCol < COLUMNS; ++textCol )
  {
    int charIdx  = *curItem++;
    int charAttr = *curItem++;

    bool upperLine  =  ( charAttr & 0x08 );
    bool normalFont = !( charAttr & 0x10 );
    bool reverse    =  ( charAttr & 0x20 );
    bool erase      =  ( charAttr & 0x40 );
    bool underLine  =  ( charAttr & 0x80 );

    uint8_t color = charAttr & 0x07;
    uint8_t bg = reverse ? rawLUT[ color ] : rawLUT[0];
    uint8_t fg = reverse ? ( erase ? rawLUT[ color ] : rawLUT[0] ) : ( erase ? rawLUT[0] : rawLUT[ color ] );

    const uint8_t colors[2] = { bg, fg };

    uint8_t const * charBitmapPtr = ga->m_enablePCG
                                    ? fontDataPCG   + charIdx * CHARSIZEINBYTES
                                    : normalFont
                                    ? fontData      + charIdx * CHARSIZEINBYTES
                                    : fontDataGraph + charIdx * CHARSIZEINBYTES;

    auto destptr = dest;

    if ( showCursor && textCol == cursorCol )
    {
      uint8_t const * cursorBitmapPtr = ga->m_cursorGlyph + ( charScanline * CHARWIDTHINBYTES );

      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;
        uint32_t cusrsorBitmap = *cursorBitmapPtr;

        if ( charScanline == 0 && charRow == 0 )
        {
          cusrsorBitmap = 0x00;
          if ( upperLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        if ( charScanline == 7 && charRow == ( LINES / 2 ) - 1 )
        {
          cusrsorBitmap = 0x00;
          if ( underLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        charBitmap ^= cusrsorBitmap;

        *(destptr + 0) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 8) = colors[(bool)(charBitmap & 0x04)];
        *(destptr + 9) = colors[(bool)(charBitmap & 0x04)];
        *(destptr +10) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +11) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +12) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +13) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +14) = colors[(bool)(charBitmap & 0x02)];
        *(destptr +15) = colors[(bool)(charBitmap & 0x02)];

        destptr += SCREENWIDTH;
        if ( charScanline == 0 && charRow == 0 )
          ;
        else
        {
          charBitmapPtr += CHARWIDTHINBYTES;
          cursorBitmapPtr += CHARWIDTHINBYTES;
        }
      }
    }
    else
    {
      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;

        if ( charScanline == 0 && charRow == 0 )
        {
          if ( upperLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        if ( charScanline == 7 && charRow == ( LINES / 2 ) - 1 )
        {
          if ( underLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        *(destptr + 0) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 8) = colors[(bool)(charBitmap & 0x04)];
        *(destptr + 9) = colors[(bool)(charBitmap & 0x04)];
        *(destptr +10) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +11) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +12) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +13) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +14) = colors[(bool)(charBitmap & 0x02)];
        *(destptr +15) = colors[(bool)(charBitmap & 0x02)];

        destptr += SCREENWIDTH;
        if ( charScanline == 0 && charRow == 0 )
          ;
        else
          charBitmapPtr += CHARWIDTHINBYTES;
      }
    }

    dest += 16;
  }
}


void IRAM_ATTR packeduPD3301adpter::drawScanline_PC_Text_40x25_8Colors( void * arg, uint8_t * dest, int scanLine )
{
  auto ga = (packeduPD3301adpter *)arg;

  constexpr int CHARWIDTH        = 8;
  constexpr int CHARHEIGHT       = 8;
  constexpr int CHARWIDTHINBYTES = (CHARWIDTH + 7) / 8;
  constexpr int CHARSIZEINBYTES  = CHARWIDTHINBYTES * CHARHEIGHT;
  constexpr int COLUMNS          = 40;
  constexpr int SCREENWIDTH      = 640;
  constexpr int LINES            = 8;

  if ( scanLine == 0 )
    ++ga->m_frameCounter;

  int charScanline = scanLine & ( CHARHEIGHT - 1 );
  int textRow      = scanLine / CHARHEIGHT;

  uint8_t const * fontData      = ga->m_font.data      + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataGraph = ga->m_fontGraph.data + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataPCG   = ga->m_fontPCG.data   + ( charScanline * CHARWIDTHINBYTES );

  uint8_t * rawLUT = ga->m_rawLUT;

  uint8_t const * curItem = ga->m_videoBuffer + ( textRow * COLUMNS * 2 );

  bool showCursor = ga->m_cursorVisible && ga->m_cursorRow == textRow && ( ( ga->m_frameCounter & 0x1F ) < 0x0F );
  int cursorCol = ga->m_cursorCol;

  for ( int textCol = 0; textCol < COLUMNS; ++textCol )
  {
    int charIdx  = *curItem++;
    int charAttr = *curItem++;

    bool upperLine  =  ( charAttr & 0x08 );
    bool normalFont = !( charAttr & 0x10 );
    bool reverse    =  ( charAttr & 0x20 );
    bool erase      =  ( charAttr & 0x40 );
    bool underLine  =  ( charAttr & 0x80 );

    uint8_t color = charAttr & 0x07;
    uint8_t bg = reverse ? rawLUT[ color ] : rawLUT[0];
    uint8_t fg = reverse ? ( erase ? rawLUT[ color ] : rawLUT[0] ) : ( erase ? rawLUT[0] : rawLUT[ color ] );

    const uint8_t colors[2] = { bg, fg };

    uint8_t const * charBitmapPtr = ga->m_enablePCG
                                    ? fontDataPCG   + charIdx * CHARSIZEINBYTES
                                    : normalFont
                                    ? fontData      + charIdx * CHARSIZEINBYTES
                                    : fontDataGraph + charIdx * CHARSIZEINBYTES;

    auto destptr = dest;

    if ( showCursor && textCol == cursorCol )
    {
      uint8_t const * cursorBitmapPtr = ga->m_cursorGlyph + ( charScanline * CHARWIDTHINBYTES );

      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;

        if ( ( scanLine % LINES ) == 0 && charRow == 0 && upperLine )
          charBitmap |= 0xFF;

        if ( ( scanLine % LINES ) != 0 && charRow == ( ( LINES / 2 ) - 1 ) && underLine )
          charBitmap |= 0xFF;

        charBitmap ^= *cursorBitmapPtr;

        *(destptr + 0) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 8) = colors[(bool)(charBitmap & 0x04)];
        *(destptr + 9) = colors[(bool)(charBitmap & 0x04)];
        *(destptr +10) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +11) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +12) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +13) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +14) = colors[(bool)(charBitmap & 0x02)];
        *(destptr +15) = colors[(bool)(charBitmap & 0x02)];

        destptr += SCREENWIDTH;
        charBitmapPtr += CHARWIDTHINBYTES;
        cursorBitmapPtr += CHARWIDTHINBYTES;
      }
    }
    else
    {
      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;

        if ( ( scanLine % LINES ) == 0 && charRow == 0 && upperLine )
          charBitmap |= 0xFF;

        if ( ( scanLine % LINES ) != 0 && charRow == ( ( LINES / 2 ) - 1 ) && underLine )
          charBitmap |= 0xFF;

        *(destptr + 0) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 8) = colors[(bool)(charBitmap & 0x04)];
        *(destptr + 9) = colors[(bool)(charBitmap & 0x04)];
        *(destptr +10) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +11) = colors[(bool)(charBitmap & 0x08)];
        *(destptr +12) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +13) = colors[(bool)(charBitmap & 0x01)];
        *(destptr +14) = colors[(bool)(charBitmap & 0x02)];
        *(destptr +15) = colors[(bool)(charBitmap & 0x02)];

        destptr += SCREENWIDTH;
        charBitmapPtr += CHARWIDTHINBYTES;
      }
    }

    dest += 16;
  }
}


void IRAM_ATTR packeduPD3301adpter::drawScanline_PC_Text_80x20_8Colors( void * arg, uint8_t * dest, int scanLine )
{
  auto ga = (packeduPD3301adpter *)arg;

  constexpr int CHARWIDTH        = 8;
  constexpr int CHARHEIGHT       = 8;
  constexpr int CHARWIDTHINBYTES = (CHARWIDTH + 7) / 8;
  constexpr int CHARSIZEINBYTES  = CHARWIDTHINBYTES * CHARHEIGHT;
  constexpr int COLUMNS          = 80;
  constexpr int SCREENWIDTH      = 640;
  constexpr int LINES            = 4;

  if ( scanLine == 0 )
    ++ga->m_frameCounter;

  int charScanline = scanLine % ( CHARHEIGHT + 2 );
  int textRow      = scanLine / ( CHARHEIGHT + 2 );
  if ( charScanline != 0 )
    charScanline--;

  uint8_t const * fontData      = ga->m_font.data      + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataGraph = ga->m_fontGraph.data + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataPCG   = ga->m_fontPCG.data   + ( charScanline * CHARWIDTHINBYTES );

  uint8_t * rawLUT = ga->m_rawLUT;

  uint8_t const * curItem = ga->m_videoBuffer + ( textRow * COLUMNS * 2 );

  bool showCursor = ga->m_cursorVisible && ga->m_cursorRow == textRow && ( ( ga->m_frameCounter & 0x1F ) < 0x0F );
  int cursorCol = ga->m_cursorCol;

  for ( int textCol = 0; textCol < COLUMNS; ++textCol )
  {
    int charIdx  = *curItem++;
    int charAttr = *curItem++;

    bool upperLine  =  ( charAttr & 0x08 );
    bool normalFont = !( charAttr & 0x10 );
    bool reverse    =  ( charAttr & 0x20 );
    bool erase      =  ( charAttr & 0x40 );
    bool underLine  =  ( charAttr & 0x80 );

    uint8_t color = charAttr & 0x07;
    uint8_t bg = reverse ? rawLUT[ color ] : rawLUT[0];
    uint8_t fg = reverse ? ( erase ? rawLUT[ color ] : rawLUT[0] ) : ( erase ? rawLUT[0] : rawLUT[ color ] );

    const uint8_t colors[2] = { bg, fg };

    uint8_t const * charBitmapPtr = ga->m_enablePCG
                                    ? fontDataPCG   + charIdx * CHARSIZEINBYTES
                                    : normalFont
                                    ? fontData      + charIdx * CHARSIZEINBYTES
                                    : fontDataGraph + charIdx * CHARSIZEINBYTES;

    auto destptr = dest;

    if ( showCursor && textCol == cursorCol )
    {
      uint8_t const * cursorBitmapPtr = ga->m_cursorGlyph + ( charScanline * CHARWIDTHINBYTES );

      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;
        uint32_t cusrsorBitmap = *cursorBitmapPtr;

        if ( charScanline == 0 && charRow == 0 )
        {
          cusrsorBitmap = 0x00;
          if ( upperLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        if ( charScanline == 7 && charRow == ( LINES / 2 ) - 1 )
        {
          cusrsorBitmap = 0x00;
          if ( underLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        charBitmap ^= cusrsorBitmap;

        *(destptr + 0) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x02)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x01)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x08)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x04)];

        destptr += SCREENWIDTH;
        if ( charScanline == 0 && charRow == 0 )
          ;
        else
        {
          charBitmapPtr += CHARWIDTHINBYTES;
          cursorBitmapPtr += CHARWIDTHINBYTES;
        }
      }
    }
    else
    {
      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;

        if ( charScanline == 0 && charRow == 0 )
        {
          if ( upperLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        if ( charScanline == 7 && charRow == ( LINES / 2 ) - 1 )
        {
          if ( underLine )
            charBitmap = 0xFF;
          else
            charBitmap = 0x00;
        }

        *(destptr + 0) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x02)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x01)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x08)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x04)];

        destptr += SCREENWIDTH;
        if ( charScanline == 0 && charRow == 0 )
          ;
        else
          charBitmapPtr += CHARWIDTHINBYTES;
      }
    }

    dest += 8;
  }
}

void IRAM_ATTR packeduPD3301adpter::drawScanline_PC_Text_80x25_8Colors( void * arg, uint8_t * dest, int scanLine )
{
  auto ga = (packeduPD3301adpter *)arg;

  constexpr int CHARWIDTH        = 8;
  constexpr int CHARHEIGHT       = 8;
  constexpr int CHARWIDTHINBYTES = (CHARWIDTH + 7) / 8;
  constexpr int CHARSIZEINBYTES  = CHARWIDTHINBYTES * CHARHEIGHT;
  constexpr int COLUMNS          = 80;
  constexpr int SCREENWIDTH      = 640;
  constexpr int LINES            = 8;

  if ( scanLine == 0 )
    ++ga->m_frameCounter;

  int charScanline = scanLine & ( CHARHEIGHT - 1 );
  int textRow      = scanLine / CHARHEIGHT;

  uint8_t const * fontData      = ga->m_font.data      + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataGraph = ga->m_fontGraph.data + ( charScanline * CHARWIDTHINBYTES );
  uint8_t const * fontDataPCG   = ga->m_fontPCG.data   + ( charScanline * CHARWIDTHINBYTES );

  uint8_t * rawLUT = ga->m_rawLUT;

  uint8_t const * curItem = ga->m_videoBuffer + ( textRow * COLUMNS * 2 );

  bool showCursor = ga->m_cursorVisible && ga->m_cursorRow == textRow && ( ( ga->m_frameCounter & 0x1F ) < 0x0F );
  int cursorCol = ga->m_cursorCol;

  for ( int textCol = 0; textCol < COLUMNS; ++textCol )
  {
    int charIdx  = *curItem++;
    int charAttr = *curItem++;

    bool upperLine  =  ( charAttr & 0x08 );
    bool normalFont = !( charAttr & 0x10 );
    bool reverse    =  ( charAttr & 0x20 );
    bool erase      =  ( charAttr & 0x40 );
    bool underLine  =  ( charAttr & 0x80 );

    uint8_t color = charAttr & 0x07;
    uint8_t bg = reverse ? rawLUT[ color ] : rawLUT[0];
    uint8_t fg = reverse ? ( erase ? rawLUT[ color ] : rawLUT[0] ) : ( erase ? rawLUT[0] : rawLUT[ color ] );

    const uint8_t colors[2] = { bg, fg };

    uint8_t const * charBitmapPtr = ga->m_enablePCG
                                    ? fontDataPCG   + charIdx * CHARSIZEINBYTES
                                    : normalFont
                                    ? fontData      + charIdx * CHARSIZEINBYTES
                                    : fontDataGraph + charIdx * CHARSIZEINBYTES;

    auto destptr = dest;

    if ( showCursor && textCol == cursorCol )
    {
      uint8_t const * cursorBitmapPtr = ga->m_cursorGlyph + ( charScanline * CHARWIDTHINBYTES );

      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;

        if ( ( scanLine % LINES ) == 0 && charRow == 0 && upperLine )
          charBitmap |= 0xFF;

        if ( ( scanLine % LINES ) != 0 && charRow == ( ( LINES / 2 ) - 1 ) && underLine )
          charBitmap |= 0xFF;

        charBitmap ^= *cursorBitmapPtr;

        *(destptr + 0) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x02)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x01)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x08)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x04)];

        destptr += SCREENWIDTH;
        charBitmapPtr += CHARWIDTHINBYTES;
        cursorBitmapPtr += CHARWIDTHINBYTES;
      }
    }
    else
    {
      for ( int charRow = 0; charRow < LINES / 2; ++charRow )
      {
        uint32_t charBitmap = *charBitmapPtr;

        if ( ( scanLine % LINES ) == 0 && charRow == 0 && upperLine )
          charBitmap |= 0xFF;

        if ( ( scanLine % LINES ) != 0 && charRow == ( ( LINES / 2 ) - 1 ) && underLine )
          charBitmap |= 0xFF;

        *(destptr + 0) = colors[(bool)(charBitmap & 0x20)];
        *(destptr + 1) = colors[(bool)(charBitmap & 0x10)];
        *(destptr + 2) = colors[(bool)(charBitmap & 0x80)];
        *(destptr + 3) = colors[(bool)(charBitmap & 0x40)];
        *(destptr + 4) = colors[(bool)(charBitmap & 0x02)];
        *(destptr + 5) = colors[(bool)(charBitmap & 0x01)];
        *(destptr + 6) = colors[(bool)(charBitmap & 0x08)];
        *(destptr + 7) = colors[(bool)(charBitmap & 0x04)];

        destptr += SCREENWIDTH;
        charBitmapPtr += CHARWIDTHINBYTES;
      }
    }

    dest += 8;
  }
}
