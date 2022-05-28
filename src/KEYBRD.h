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

#include "emu.h"

typedef struct _TAG_KEY_TBL {
  VirtualKey  VKey[3];
  int         IOinAdrs;
  uint8_t     downBitMask;
} _KEY_TBL, * _PKEY_TBL;

static const _KEY_TBL _keyTbl[] PROGMEM = {
  { { VK_KP_HOME,         VK_KP_7,          VK_NONE, },        0x00, 0x7F, },  // KEY PAD 7                          10key 7
  { { VK_KP_RIGHT,        VK_KP_6,          VK_NONE, },        0x00, 0xBF, },  // KEY PAD 6                          10key 6
  { { VK_KP_CENTER,       VK_KP_5,          VK_NONE, },        0x00, 0xDF, },  // KEY PAD 5                          10key 5
  { { VK_KP_LEFT,         VK_KP_4,          VK_NONE, },        0x00, 0xEF, },  // KEY PAD 4                          10key 4
  { { VK_KP_PAGEDOWN,     VK_KP_3,          VK_NONE, },        0x00, 0xF7, },  // KEY PAD 3                          10key 3
  { { VK_KP_DOWN,         VK_KP_2,          VK_NONE, },        0x00, 0xFB, },  // KEY PAD 2                          10key 2
  { { VK_KP_END,          VK_KP_1,          VK_NONE, },        0x00, 0xFD, },  // KEY PAD 1                          10key 1
  { { VK_KP_INSERT,       VK_KP_0,          VK_NONE, },        0x00, 0xFE, },  // KEY PAD 0                          10key 0
  { { VK_KP_ENTER,        VK_RETURN,        VK_NONE, },        0x01, 0x7F, },  // KEY PAD Enter & Enter              RETURN
  { { VK_KP_DELETE,       VK_KP_PERIOD,     VK_NONE, },        0x01, 0xBF, },  // KEY PAD .                          10key .
  { { VK_KP_DIVIDE,       VK_NONE,          VK_NONE, },        0x01, 0xDF, },  // KEY PAD /                          10key ,
  { { VK_KP_MINUS,        VK_NONE,          VK_NONE, },        0x01, 0xEF, },  // KEY PAD -                          10key =
  { { VK_KP_PLUS,         VK_NONE,          VK_NONE, },        0x01, 0xF7, },  // KEY PAD +                          10key +
  { { VK_KP_MULTIPLY,     VK_NONE,          VK_NONE, },        0x01, 0xFB, },  // KEY PAD *                          10key *
  { { VK_KP_PAGEUP,       VK_KP_9,          VK_NONE, },        0x01, 0xFD, },  // KEY PAD 9                          10key 9
  { { VK_KP_UP,           VK_KP_8,          VK_NONE, },        0x01, 0xFE, },  // KEY PAD 8                          10key 8
  { { VK_g,               VK_G,             VK_NONE, },        0x02, 0x7F, },  // gG                                 gG
  { { VK_f,               VK_F,             VK_NONE, },        0x02, 0xBF, },  // fF                                 fF
  { { VK_e,               VK_E,             VK_NONE, },        0x02, 0xDF, },  // eE                                 eE
  { { VK_d,               VK_D,             VK_NONE, },        0x02, 0xEF, },  // dD                                 dD
  { { VK_c,               VK_C,             VK_NONE, },        0x02, 0xF7, },  // cC                                 cC
  { { VK_b,               VK_B,             VK_NONE, },        0x02, 0xFB, },  // bB                                 bB
  { { VK_a,               VK_A,             VK_NONE, },        0x02, 0xFD, },  // aA                                 aA
  { { VK_AT,              VK_GRAVEACCENT,   VK_NONE, },        0x02, 0xFE, },  // @`                                 @`
  { { VK_o,               VK_O,             VK_NONE, },        0x03, 0x7F, },  // oO                                 oO
  { { VK_n,               VK_N,             VK_NONE, },        0x03, 0xBF, },  // nN                                 nN
  { { VK_m,               VK_M,             VK_NONE, },        0x03, 0xDF, },  // mM                                 mM
  { { VK_l,               VK_L,             VK_NONE, },        0x03, 0xEF, },  // lL                                 lL
  { { VK_k,               VK_K,             VK_NONE, },        0x03, 0xF7, },  // kK                                 kK
  { { VK_j,               VK_J,             VK_NONE, },        0x03, 0xFB, },  // jJ                                 jJ
  { { VK_i,               VK_I,             VK_NONE, },        0x03, 0xFD, },  // iI                                 iI
  { { VK_h,               VK_H,             VK_NONE, },        0x03, 0xFE, },  // hH                                 hH
  { { VK_w,               VK_W,             VK_NONE, },        0x04, 0x7F, },  // wW                                 wW
  { { VK_v,               VK_V,             VK_NONE, },        0x04, 0xBF, },  // vV                                 vV
  { { VK_u,               VK_U,             VK_NONE, },        0x04, 0xDF, },  // uU                                 uU
  { { VK_t,               VK_T,             VK_NONE, },        0x04, 0xEF, },  // tT                                 tT
  { { VK_s,               VK_S,             VK_NONE, },        0x04, 0xF7, },  // sS                                 sS
  { { VK_r,               VK_R,             VK_NONE, },        0x04, 0xFB, },  // rR                                 rR
  { { VK_q,               VK_Q,             VK_NONE, },        0x04, 0xFD, },  // qQ                                 qQ
  { { VK_p,               VK_P,             VK_NONE, },        0x04, 0xFE, },  // pP                                 pP
  { { VK_MINUS,           VK_EQUALS,        VK_NONE, },        0x05, 0x7F, },  // -=                                 -=
  { { VK_CARET,           VK_TILDE,         VK_NONE, },        0x05, 0xBF, },  // ^~                                 ^~
  { { VK_RIGHTBRACKET,    VK_RIGHTBRACE,    VK_NONE, },        0x05, 0xDF, },  // ]}                                 ]}
  { { VK_YEN,             VK_VERTICALBAR,   VK_NONE, },        0x05, 0xEF, },  // ￥|                                ￥|
  { { VK_LEFTBRACKET,     VK_LEFTBRACE,     VK_NONE, },        0x05, 0xF7, },  // [{                                 [{
  { { VK_z,               VK_Z,             VK_NONE, },        0x05, 0xFB, },  // zZ                                 zZ
  { { VK_y,               VK_Y,             VK_NONE, },        0x05, 0xFD, },  // yY                                 yY
  { { VK_x,               VK_X,             VK_NONE, },        0x05, 0xFE, },  // xX                                 xX
  { { VK_7,               VK_QUOTE,         VK_NONE, },        0x06, 0x7F, },  // 7'                                 7'
  { { VK_6,               VK_AMPERSAND,     VK_NONE, },        0x06, 0xBF, },  // 6&                                 6&
  { { VK_5,               VK_PERCENT,       VK_NONE, },        0x06, 0xDF, },  // 5%                                 5%
  { { VK_4,               VK_DOLLAR,        VK_NONE, },        0x06, 0xEF, },  // 4$                                 4$
  { { VK_3,               VK_HASH,          VK_NONE, },        0x06, 0xF7, },  // 3#                                 3#
  { { VK_2,               VK_QUOTEDBL,      VK_NONE, },        0x06, 0xFB, },  // 2"                                 2"
  { { VK_1,               VK_EXCLAIM,       VK_NONE, },        0x06, 0xFD, },  // 1!                                 1!
  { { VK_0,               VK_SHIFT_0,       VK_NONE, },        0x06, 0xFE, },  // 0                                  0
  { { VK_BACKSLASH,       VK_UNDERSCORE,    VK_NONE, },        0x07, 0x7F, },  // \_                                 \_
  { { VK_SLASH,           VK_QUESTION,      VK_NONE, },        0x07, 0xBF, },  // /?                                 /?
  { { VK_PERIOD,          VK_GREATER,       VK_NONE, },        0x07, 0xDF, },  // .>                                 .>
  { { VK_COMMA,           VK_LESS,          VK_NONE, },        0x07, 0xEF, },  // ,<                                 ,<
  { { VK_SEMICOLON,       VK_PLUS,          VK_NONE, },        0x07, 0xF7, },  // ;+                                 ;+
  { { VK_COLON,           VK_ASTERISK,      VK_NONE, },        0x07, 0xFB, },  // :*                                 :*
  { { VK_9,               VK_RIGHTPAREN,    VK_NONE, },        0x07, 0xFD, },  // 9)                                 9)
  { { VK_8,               VK_LEFTPAREN,     VK_NONE, },        0x07, 0xFE, },  // 8(                                 8(
  { { VK_LCTRL,           VK_RCTRL,         VK_CAPSLOCK, },    0x08, 0x7F, },  // LEFT & RIGHT Ctrl & CapsLock       CTRL
  { { VK_LSHIFT,          VK_RSHIFT,        VK_NONE, },        0x08, 0xBF, },  // LEFT & RIGHT Shift                 SHIFT
  { { VK_KATAKANA_HIRAGANA_ROMAJI,
                          VK_NONE,          VK_NONE, },        0x08, 0xDF, },  // カタカナ ひらがな ローマ字          カナ
  { { VK_LALT,            VK_RALT,          VK_NONE, },        0x08, 0xEF, },  // LEFT & RIGHT Alt                   GRAPH
  { { VK_DELETE,          VK_INSERT,        VK_BACKSPACE, },   0x08, 0xF7, },  // Delete & Insert & Backspace        INS DEL
  { { VK_RIGHT,           VK_LEFT,          VK_NONE, },        0x08, 0xFB, },  // → & ←                            ←→
  { { VK_UP,              VK_DOWN,          VK_NONE, },        0x08, 0xFD, },  // ↑ & ↓                            ↓↑
  { { VK_HOME,            VK_NUMLOCK,       VK_NONE, },        0x08, 0xFE, },  // Home & Num Lock                    HOME CLR
  { { VK_TAB,             VK_NONE,          VK_NONE, },        0x09, 0x7F, },  // Tab                                ESC
  { { VK_SPACE,           VK_NONE,          VK_NONE, },        0x09, 0xBF, },  // Space                              SPACE
  { { VK_F5,              VK_NONE,          VK_NONE, },        0x09, 0xDF, },  // F5                                 f･5
  { { VK_F4,              VK_NONE,          VK_NONE, },        0x09, 0xEF, },  // F4                                 f･4
  { { VK_F3,              VK_NONE,          VK_NONE, },        0x09, 0xF7, },  // F3                                 f･3
  { { VK_F2,              VK_NONE,          VK_NONE, },        0x09, 0xFB, },  // F2                                 f･2
  { { VK_F1,              VK_NONE,          VK_NONE, },        0x09, 0xFD, },  // F1                                 f･1
  { { VK_ESCAPE,          VK_NONE,          VK_NONE, },        0x09, 0xFE, },  // Esc                                STOP
  { { VK_HANKAKU_ZENKAKU_KANJI,
                          VK_NONE,          VK_NONE, },        0xFE, 0xFF, },  // 半角/全角/漢字
                                                                               // (ローマ字カナ変換な IME ON/OFF 実装止め。需要(ヾﾉ･∀･`)ﾅｲﾅｲ)
  { { VK_F9,              VK_NONE,          VK_NONE, },        0xFF, 0xFF, },  // F9
  { { VK_F10,             VK_NONE,          VK_NONE, },        0xFF, 0xFF, },  // F10
  { { VK_F11,             VK_NONE,          VK_NONE, },        0xFF, 0xFF, },  // F11
  { { VK_F12,             VK_NONE,          VK_NONE, },        0xFF, 0xFF, },  // F12
#ifdef _KEYBRD_LAYOUT_ALLCHECK
  { { VK_PAUSE,           VK_NONE,          VK_NONE, },        0x0A, 0x7F, },  // Pause
  { { VK_SCROLLLOCK,      VK_NONE,          VK_NONE, },        0x0A, 0xBF, },  // Scroll Lock
  { { VK_PRINTSCREEN,     VK_NONE,          VK_NONE, },        0x0A, 0xDF, },  // Print Screen
  { { VK_PAGEUP,          VK_NONE,          VK_NONE, },        0x0A, 0xEF, },  // Page Up
  { { VK_PAGEDOWN,        VK_NONE,          VK_NONE, },        0x0A, 0xF7, },  // Page Down
  { { VK_END,             VK_NONE,          VK_NONE, },        0x0A, 0xFB, },  // End
  { { VK_F8,              VK_NONE,          VK_NONE, },        0x0A, 0xFD, },  // F8
  { { VK_F7,              VK_NONE,          VK_NONE, },        0x0A, 0xFE, },  // F7
  { { VK_F6,              VK_NONE,          VK_NONE, },        0x0B, 0x7F, },  // F6
  { { VK_APPLICATION,     VK_NONE,          VK_NONE, },        0x0B, 0xBF, },  // Application
  { { VK_RGUI,            VK_NONE,          VK_NONE, },        0x0B, 0xDF, },  // RIGHT GUI
  { { VK_HENKAN           VK_NONE,          VK_NONE, },        0x0B, 0xEF, },  // 変換
  { { VK_MUHENKAN,        VK_NONE,          VK_NONE, },        0x0B, 0xF7, },  // 無変換
  { { VK_LGUI,            VK_NONE,          VK_NONE, },        0x0B, 0xFB, },  // LEFT GUI
#endif // _KEYBRD_LAYOUT_ALLCHECK
};


class KEYBRD
{
public:
  // callbacks
  typedef void (* setVKF9Callback)( void * context, int value );
  typedef void (* setVKF10Callback)( void * context, int value );
  typedef void (* setVKF11Callback)( void * context, int value );
  typedef void (* setVKF12Callback)( void * context, int value );

  void setCallbacks( void * context, setVKF9Callback vkf9, setVKF10Callback vkf10, setVKF11Callback vkf11, setVKF12Callback vkf12 )
  {
    m_context = context;
    m_vkf9    = vkf9;
    m_vkf10   = vkf10;
    m_vkf11   = vkf11;
    m_vkf12   = vkf12;
  }

  bool reset( uint8_t * vram, Keyboard *keyboard )
  {
    memset( m_IoIn, 0xFF, sizeof( m_IoIn ) );
    memset( m_oldSpecialKeyDown, 0, sizeof( m_oldSpecialKeyDown ) );
    m_keyboard         = keyboard;
    m_shiftKeyTblPos   = -1;
    m_kanaKeyTblPos    = -1;
    m_kanaKeyAlternate = false;
    m_kanaKeyDown      = false;
    m_oldKanaKeyDown   = false;
    m_imeKeyAlternate  = false;
    m_imeKeyDown       = false;
    m_oldImeKeyDown    = false;
    m_vram             = vram;
    for ( int i = 0; i < ARRAY_SIZE( _keyTbl ); i++ )
    {
      if ( _keyTbl[i].VKey[0] == VK_LSHIFT
        && _keyTbl[i].VKey[1] == VK_RSHIFT )
        m_shiftKeyTblPos = i;
      if ( _keyTbl[i].VKey[0] == VK_KATAKANA_HIRAGANA_ROMAJI )
        m_kanaKeyTblPos = i;
    }
    setScreenMode( 0 );
    return true;
  }

  bool readIO( int address, int * result )
  {
    int keybrdIoEnd = 0x09;
#ifdef _KEYBRD_LAYOUT_ALLCHECK
    keybrdIoEnd = 0x0F;
#endif // _KEYBRD_LAYOUT_ALLCHECK
    if ( address >= 0x00 && address <= keybrdIoEnd )
    {
      uint8_t value = m_IoIn[address];
      *result = value;
      return true;
    }
    return false;
  }

  bool writeIO( int address, int value )
  {
    return false;
  }

  void tick( void )
  {
    bool keyDown[3];
    bool checkShiftDown;
    bool checkShiftUp;
    bool kanaKey;
    bool imeKey;
    bool specialKey;
    for ( int i = 0; i < ARRAY_SIZE( _keyTbl ); i++ )
    {
      checkShiftDown = false;
      checkShiftUp   = false;
      kanaKey        = false;
      imeKey         = false;
      specialKey     = false;
      if ( i == m_shiftKeyTblPos )
        checkShiftUp = true;
      if ( ( _keyTbl[i].IOinAdrs == 0x08 && _keyTbl[i].downBitMask == 0xF7 )
        || ( _keyTbl[i].IOinAdrs == 0x08 && _keyTbl[i].downBitMask == 0xFB )
        || ( _keyTbl[i].IOinAdrs == 0x08 && _keyTbl[i].downBitMask == 0xFD ) )
        checkShiftDown = true;
      if ( _keyTbl[i].IOinAdrs == 0x08 && _keyTbl[i].downBitMask == 0xDF )
        kanaKey = true;
      if ( _keyTbl[i].IOinAdrs == 0xFE && _keyTbl[i].downBitMask == 0xFF )
        imeKey = true;
      if ( _keyTbl[i].IOinAdrs == 0xFF && _keyTbl[i].downBitMask == 0xFF )
        specialKey = true;
      keyDown[0] = ( _keyTbl[i].VKey[0] != VK_NONE && m_keyboard->isVKDown( _keyTbl[i].VKey[0] ) ) == true ? true : false;
      keyDown[1] = ( _keyTbl[i].VKey[1] != VK_NONE && m_keyboard->isVKDown( _keyTbl[i].VKey[1] ) ) == true ? true : false;
      keyDown[2] = ( _keyTbl[i].VKey[2] != VK_NONE && m_keyboard->isVKDown( _keyTbl[i].VKey[2] ) ) == true ? true : false;
      if ( checkSpecialKey( specialKey, _keyTbl[i].VKey[0], keyDown[0] ) )
        continue;
      if ( keyDown[0] || keyDown[1] || keyDown[2] )
      {
        if ( checkShiftDown )
        {
          if ( !keyDown[0] && keyDown[1] && !keyDown[2] )
            setKeyDown( m_shiftKeyTblPos );
        }
        setKeyDown( i );
        alternateKey( kanaKey, imeKey );
      }
      else
      {
        if ( checkShiftUp )
        {
          keyDown[0] = ( m_IoIn[ 0x08 ] & ( (uint8_t)( ~0xF7 ) & 0xFF ) ) == 0 ? true : false;
          keyDown[1] = ( m_IoIn[ 0x08 ] & ( (uint8_t)( ~0xFB ) & 0xFF ) ) == 0 ? true : false;
          keyDown[2] = ( m_IoIn[ 0x08 ] & ( (uint8_t)( ~0xFD ) & 0xFF ) ) == 0 ? true : false;
          if ( keyDown[0] || keyDown[1] || keyDown[2] )
            continue;
        }
        if ( imeKey )
        {
          m_imeKeyDown = false;
          if ( m_imeKeyAlternate )
            continue;
        }
        else if ( kanaKey )
        {
          m_kanaKeyDown = false;
          if ( m_kanaKeyAlternate )
            continue;
        }
        setKeyUp( i );
      }
    }

    if ( m_kanaKeyAlternate )
      updateKanaLockStatus();
    if ( m_imeKeyAlternate )
      updateImeLockStatus();

    m_oldKanaKeyDown = m_kanaKeyDown;
    if ( m_keyboard->virtualKeyAvailable() )
      m_keyboard->emptyVirtualKeyQueue();
  }

  void setScreenMode( int value )
  {
    switch ( value )
    {
      case 0: // 40x20
      case 1: // 40x25
        m_width_mode = 0;
        m_statusAdrs = 35 * 2;
        updateKanaLockStatus();
        break;
      case 2: // 80x20
      case 3: // 80x25
        m_width_mode = 1;
        m_statusAdrs = 74;
        updateKanaLockStatus();
        break;
    }
  }

private:
  void alternateKey( bool kanaKey, bool imeKey )
  {
    // ローマ字カナ変換な IME を実装しようと思ったけど需要無いと思うし止めたｗ
    // ガチでヘボン式実装しても流派あるし。華道/茶道かよ(ワロス)
    if ( imeKey )
    {
      if ( !m_imeKeyDown )
      {
        m_imeKeyDown = true;
        if ( !m_imeKeyAlternate )
        {
          if ( !m_oldImeKeyDown && m_imeKeyDown )
          {
            m_imeKeyAlternate = true;
            updateImeLockStatus();
          }
        }
        else
        {
          if ( !m_oldImeKeyDown && m_imeKeyDown )
          {
            m_imeKeyAlternate = false;
            updateImeLockStatus();
          }
        }
      }
    }
    else if ( kanaKey )
    {
      if ( !m_kanaKeyDown )
      {
        m_kanaKeyDown = true;
        if ( !m_kanaKeyAlternate )
        {
          if ( !m_oldKanaKeyDown && m_kanaKeyDown )
          {
            m_kanaKeyAlternate = true;
            updateKanaLockStatus();
          }
        }
        else
        {
          if ( !m_oldKanaKeyDown && m_kanaKeyDown )
          {
            m_kanaKeyAlternate = false;
            updateKanaLockStatus();
          }
        }
      }
    }
  }

  bool checkSpecialKey( bool specialKey, VirtualKey vkey, bool keyDown )
  {
    if ( !specialKey ) return false;
    int keyNo = -1;
    switch ( (int)vkey )
    {
      case VK_F9:
        keyNo = 0;
        break;
      case VK_F10:
        keyNo = 1;
        break;
      case VK_F11:
        keyNo = 2;
        break;
      case VK_F12:
        keyNo = 3;
        break;
    }
    if ( keyNo == -1 ) return false;
    if ( keyDown )
    {
      VirtualKeyItem vItem;
      m_keyboard->getNextVirtualKey( &vItem );
      if ( !m_oldSpecialKeyDown[keyNo] )
      {
        switch ( keyNo )
        {
          case 0:
            m_vkf9( m_context, vItem.SHIFT );
            break;
          case 1:
            m_vkf10( m_context, vItem.SHIFT );
            break;
          case 2:
            m_vkf11( m_context, vItem.SHIFT );
            break;
          case 3:
            m_vkf12( m_context, vItem.SHIFT );
            break;
        }
        m_oldSpecialKeyDown[keyNo] = keyDown;
      }
    }
    else
      m_oldSpecialKeyDown[keyNo] = keyDown;
    return true;
  }

  void setKeyDown( int keyTblPos )
  {
    if ( keyTblPos < 0 || keyTblPos >= ARRAY_SIZE( _keyTbl ) ) return;
    if ( _keyTbl[keyTblPos].IOinAdrs >= 0x10 ) return;
    uint8_t value = m_IoIn[ _keyTbl[keyTblPos].IOinAdrs ];
    value &= _keyTbl[keyTblPos].downBitMask;
    m_IoIn[ _keyTbl[keyTblPos].IOinAdrs ] = value;
  }

  void setKeyUp( int keyTblPos )
  {
    if ( keyTblPos < 0 || keyTblPos >= ARRAY_SIZE( _keyTbl ) ) return;
    if ( _keyTbl[keyTblPos].IOinAdrs >= 0x10 ) return;
    uint8_t value = m_IoIn[ _keyTbl[keyTblPos].IOinAdrs ];
    value |= ( (uint8_t)~_keyTbl[keyTblPos].downBitMask ) & 0xFF;
    m_IoIn[ _keyTbl[keyTblPos].IOinAdrs ] = value;
  }

  void updateKanaLockStatus( void )
  {
#ifndef DONT_KANA_LOCK_INDICATOR
    const char *msg = NULL;
    if ( m_kanaKeyAlternate )
      // \xB6 : PC-8001 font 'カ'
      // \xC5 : PC-8001 font 'ナ'
      msg = "[\xB6\xC5]";
    else
      msg = "    ";
    int len = strlen( msg );
    for ( int i = 0; i < len; i++ )
    {
      if ( m_width_mode == 0 )
        m_vram[ m_statusAdrs + ( 2 * i ) ] = msg[i];
      else
        m_vram[ m_statusAdrs + i ] = msg[i];
    }
#endif // !DONT_KANA_LOCK_INDICATOR
  }

  void updateImeLockStatus( void )
  {
#ifndef DONT_IME_LOCK_INDICATOR
    const char *msg = NULL;
    if ( m_imeKeyAlternate )
      msg = "[IME]";
    else
      msg = "     ";
    int len = strlen( msg );
    for ( int i = 0; i < len; i++ )
    {
      if ( m_width_mode == 0 )
        m_vram[ m_statusAdrs + ( 2 * i ) ] = msg[i];
      else
        m_vram[ m_statusAdrs + i ] = msg[i];
    }
#endif // !DONT_IME_LOCK_INDICATOR
  }

  uint8_t     m_IoIn[16];                 // I/O input port 00h ~ 0Fh
  int         m_shiftKeyTblPos;           // PC-8001 shift key table postion
  int         m_kanaKeyTblPos;            // PC-8001 kana key table postion
  bool        m_kanaKeyAlternate;         // flag : KANA alternate
  bool        m_kanaKeyDown;              // flag : KANA key down
  bool        m_oldKanaKeyDown;           // flag : KANA key down for previous
  bool        m_imeKeyAlternate;          // flag : IME alternate
  bool        m_imeKeyDown;               // flag : IME key down
  bool        m_oldImeKeyDown;            // flag : IME key down for previous
  bool        m_oldSpecialKeyDown[4];     // flag : special Key down for previous
  uint8_t *   m_vram;                     // PC-8001 V-RAM address
  int         m_width_mode;               // width mode 0:40 1:80
  int         m_statusAdrs;               // Status address of カナ/IME lock
  void *      m_context;                  // callbacks
  setVKF9Callback  m_vkf9;                // callback : vkf9()
  setVKF10Callback m_vkf10;               // callback : vkf10()
  setVKF11Callback m_vkf11;               // callback : vkf11()
  setVKF12Callback m_vkf12;               // callback : vkf12()
  Keyboard *  m_keyboard;                 // fabgl library : class Keyboard
};

