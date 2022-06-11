/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on CRTC.c/CURSOR.c/DMAC.c/VRTC.c in n80pi20210814.tar.gz
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


#include "CRT.h"

// GRB to RGB order
// PC-8001(GRB bit order)  RGB bit orader
// color 0 BLACK           BLACK
// color 1 BLUE            BLUE
// color 2 RED             GREEN
// color 3 MAGENTA         CYAN
// color 4 GREEN           RED
// color 5 CYAN            MAGENTA
// color 6 YELLOW          YELLOW
// color 7 WHITE           WHITE
static const uint8_t GRB2RGB[] PROGMEM = { 0x00, 0x01, 0x04, 0x05, 0x02, 0x03, 0x06, 0x07 };

// RGB orader to FabGL Color order
// RGB bit orader  FabGL RGB orader
// BLACK           Color::Black
// BLUE            Color::BrightBlue
// GREEN           Color::BrightGreen
// CYAN            Color::BrightCyan
// RED             Color::BrightRed
// MAGENTA         Color::BrightMagenta
// YELLOW          Color::BrightYellow
// WHITE           Color::BrightWhite
static const Color RGB2FabGLColor[] PROGMEM = {
  Color::Black,
  Color::BrightBlue,
  Color::BrightGreen,
  Color::BrightCyan,
  Color::BrightRed,
  Color::BrightMagenta,
  Color::BrightYellow,
  Color::BrightWhite
};


bool CRT::reset( uint8_t *vram, uint8_t *frameBuffer )
{
  m_vram        = vram;
  m_frameBuffer = frameBuffer;

  m_crtc_burst_mode      = 0;   // off
  m_crtc_raster_per_char = 10;
  m_crtc_line_bytes      = 0;
  m_crtc_screen_mode     = 0;   // 40x20
  m_crtc_color_mode      = 1;   // mono
  m_crtc_width_mode      = 0;   // 40
  m_crtc_line_mode       = 0;   // 20
  m_crtc_attr_reverse    = 0;
  m_crtc_param_count     = 0;
  m_crtc_prev_cmd        = 0;
  m_crtc_text_visible    = _STOP_DISPLAY;

  m_dmac_penalty_base = 82;
  m_dmac_penarty      = 0;
  m_dmac_begin        = 0xF3C8;
  m_dmac_end          = 0xFF80;
  m_dmac_autoload     = 0;
	m_dmac_dmaact       = 0;
	for ( int ch = 0; ch < 4; ++ ch )
    memset( &m_dmac_ch[ch], 0, sizeof( m_dmac_ch[0] ) );

  vrtc_reset();

  m_cursor_x              = 0;
  m_cursor_y              = 0;
  m_cursor_new_col        = 0;
  m_cursor_new_row        = 0;
  m_cursor_enabled        = true;
  m_cursor_blinking       = true;
  m_cursor_cur_blink_cycl = 48;
  m_cursor_cur_blink_disp = 24;
  m_cursor_att_blink_cycl = 96;
  m_cursor_att_blink_disp = 72;

  memset( m_attr_eval_attr, 0, sizeof( m_attr_eval_attr ) );
  memset( m_attr_textcach,  0, sizeof( m_attr_textcach ) );
  memset( m_attr_attrcach,  0, sizeof( m_attr_attrcach ) );
  memset( m_attr_attrchg,   0, sizeof( m_attr_attrchg ) );
  memset( m_attr_sortstk,   0, sizeof( m_attr_sortstk ) );
  memset( m_attr_nextcolor, 0, sizeof( m_attr_nextcolor ) );
  memset( m_attr_nextfunc,  0, sizeof( m_attr_nextfunc ) );

  return true;
}


bool CRT::readIO( int address, int * result )
{
  if ( address == 0x40 )
  {
    uint8_t value = *result;
    if ( m_vrtc_flag )
      value |= 0x20;
    else
      value &= 0xDF;
    *result = value;
    return true;
  }
  return false;
}


bool CRT::writeIO( int address, int value )
{
  // CRTC mode
  if ( address == 0x30 )
  {
    crtc_mode( value );
    return true;
  }

  // CRTC data
  if ( address == 0x50 )
  {
    crtc_data( value );
    return true;
  }

  // CRTC cmd
  if ( address == 0x51 )
  {
    crtc_cmd( value );
    return true;
  }

  // DMAC adrs : I/O Port 0x60/0x62/0x64/0x66
  if ( address == 0x60 || address == 0x62 || address == 0x64 || address == 0x66 )
  {
    int ch = ( address - 0x60 ) / 2;
    dmac_ch_addr( ch, value );
    return true;
  }

  // DMAC count : I/O Port 0x61/0x63/0x65/0x67
  if ( address == 0x61 || address == 0x63 || address == 0x65 || address == 0x67 )
  {
    int ch = ( address - 0x60 ) / 2;
    dmac_ch_cnt( ch, value );
    return true;
  }

  // DMAC cmd : I/O Port 0x68
  if ( address == 0x68 )
  {
    dmac_cmd( value );
    return true;
  }

  return false;
}


void CRT::tick( int cycles )
{
  vrtc_blanking( cycles );
}


void CRT::crtc_mode( int value )
{
  uint8_t ah;
  bool changed = false;
  // 桁変更チェック
  ah = value & 0x01;
  if ( m_crtc_width_mode != ah )
  {
    m_crtc_width_mode = ah;
    ah <<= 1;
    ah += m_crtc_line_mode;
    m_crtc_screen_mode = ah;
    m_setScreenMode( m_context, m_crtc_screen_mode );
    changed = true;
  }
  // カラーモード変更チェック
  ah = ( value & 0x02 ) >> 1;
  if ( m_crtc_color_mode != ah )
  {
    m_crtc_color_mode = ah;
    changed = true;
  }
  if ( changed )
    attr_invalid();
}


void CRT::crtc_cmd( int value )
{
  switch ( ( value & 0xE0 ) >> 5 )
  {
    case 0:	// icw/ocw1 (reset/stop display)
      m_crtc_prev_cmd         = _OCW1;
      m_crtc_param_count      = 0;
      m_crtc_text_visible    |= _STOP_DISPLAY;
      m_dmac_penarty          = 0;
      m_vrtc_dma_remain_chars = 0;
      break;
    case 1:	// ocw2 (start display)
      m_crtc_prev_cmd     = _NONE;
      m_crtc_param_count  = 0;
      m_crtc_attr_reverse = value & _OCW1_DM ? 1 : 0;
      attr_invalid();
      m_crtc_text_visible &= _START_DISPLAY;
      break;
    case 4:	// ocw5 (load cursor position)
      m_crtc_prev_cmd    = _OCW5;
      m_crtc_param_count = 0;
      if ( value & 0x01 )
        cursor_enable();
      else
        cursor_disable();
      break;

    case 2:	// ocw3 (set interrupt mask)
    case 3:	// ocw4 (read light pen)
    case 5:	// ocw6 (reset interrupt)
    case 6:	// ocw7 (reset counters)
    case 7:	// ocw8
    default:
      m_crtc_prev_cmd = _NONE;
      m_crtc_param_count = 0;
      break;
  }
}


void CRT::crtc_data( int value )
{
  uint16_t cnt = m_crtc_param_count++;
  uint8_t al, ah;

  switch ( m_crtc_prev_cmd )
  {
    case _OCW1:
      switch ( cnt )
      {
        case 0:	// screen format 1
          m_crtc_burst_mode = ( value & 0x80 ) == 0 ? 1 : 0;
          m_crtc_chars_per_line = ( value & 0x7F ) + 2;
          break;
        case 1:	// screen format 2
          ah = ( value & 0x3F ) + 1;
          al = ah == 25 ? 1 : 0;
          m_crtc_lines_per_screen = ah;
          if ( m_crtc_line_mode != al )
          {
            m_crtc_line_mode = al;
            if ( m_crtc_width_mode == 1 )
              al += 0x02;
            m_crtc_screen_mode = al;
            m_setScreenMode( m_context, m_crtc_screen_mode );
            attr_invalid();
            vrtc_reset(); // change crtc sync cycle
          }
          // cursor/attribute blinking rate
          ah = ( ( value & 0xC0 ) >> 6 );
          switch ( ah )
          {
            case 0:
              m_cursor_cur_blink_cycl = 16;
              m_cursor_cur_blink_disp = 8;
              m_cursor_att_blink_cycl = 32;
              m_cursor_att_blink_disp = 24;
              break;
            case 1:
              m_cursor_cur_blink_cycl = 32;
              m_cursor_cur_blink_disp = 16;
              m_cursor_att_blink_cycl = 64;
              m_cursor_att_blink_disp = 48;
              break;
            case 3:
              m_cursor_cur_blink_cycl = 64;
              m_cursor_cur_blink_disp = 32;
              m_cursor_att_blink_cycl = 128;
              m_cursor_att_blink_disp = 96;
              break;
            default:
              m_cursor_cur_blink_cycl = 48;
              m_cursor_cur_blink_disp = 24;
              m_cursor_att_blink_cycl = 96;
              m_cursor_att_blink_disp = 72;
              break;
          }
          break;
        case 2:	// screen format 3
          m_crtc_raster_per_char = ( value & 0x1F ) + 1;
          break;
        case 3:	// screen format 4
          m_crtc_ver_ret_lines = ( ( value & 0xE0 ) >> 5 ) + 1;
          m_crtc_hol_ret_chars = ( value & 0x1F ) + 1;
          break;
        case 4:	// screen format 5
          ah = ( ( value & 0xE0 ) >> 5 );	// AT1/AT0/SC
          switch ( ah )
          {
            case 5/*0b101*/:	// non-transparent mono, special char disabled
              m_crtc_attribute_en   = 1;
              m_crtc_attribute_mono = 1;
              m_crtc_attribute_tp   = 0;
              m_crtc_special_char   = 0;
              break;
            case 4/*0b100*/:	// non-transparent mono, special char enabled
              m_crtc_attribute_en   = 1;
              m_crtc_attribute_mono = 1;
              m_crtc_attribute_tp   = 0;
              m_crtc_special_char   = 1;
              break;
            case 0/*0b000*/:	// transparent mono, special char enabled
              m_crtc_attribute_en   = 1;
              m_crtc_attribute_mono = 1;
              m_crtc_attribute_tp   = 1;
              m_crtc_special_char   = 1;
              break;
            case 2/*0b010*/:	// transparent color, special char enabled
              m_crtc_attribute_en   = 1;
              m_crtc_attribute_mono = 0;
              m_crtc_attribute_tp   = 1;
              m_crtc_special_char   = 1;
              break;
            //case 0b001:		// no attribute, special char disabled
            default:
            m_crtc_attribute_en   = 0;
            m_crtc_attribute_mono = 1;  // N/A
            m_crtc_attribute_tp   = 0;  // N/A
            m_crtc_special_char   = 0;  // N/A
            break;
          }
          al = ( value & 0x1F ) + 1;  // attribute bytes
          m_crtc_line_bytes = m_crtc_chars_per_line;
          if ( m_crtc_attribute_en == 1 )
          {
            m_crtc_line_bytes += ( al * 2 );
            // in non-transparent mode, special char area follow the char area 
            // in transparent mode, special char area is included at tail of attribute
            if ( m_crtc_special_char == 1 && m_crtc_attribute_tp == 0 )
              m_crtc_line_bytes += 2;
          }
          break;
        default:
          m_crtc_prev_cmd = _NONE;
          break;
      }
      break;

    case _OCW5:
      if ( cnt == 0 )
        m_cursor_new_col = value & 0x7F;
      else
      {
        m_cursor_new_row = value & 0x3F;
        m_crtc_prev_cmd = _NONE;
      }
      cursor_move();
      break;
  }
}


void CRT::dmac_ch_addr( int ch, int value )
{
  if ( m_dmac_ch[ch]._addr_expect_hl == 0)
  {
    m_dmac_ch[ch]._addr_expect_hl = 1;
    m_dmac_ch[ch]._addr_lo        = value;
  }
  else
  {
    m_dmac_ch[ch]._addr_expect_hl = 0;
    m_dmac_ch[ch]._addr_hi        = value;
    uint16_t addr = ( (uint16_t)m_dmac_ch[ch]._addr_hi << 8 ) + ( (uint16_t)m_dmac_ch[ch]._addr_lo );
    m_dmac_ch[ch]._addr = addr;
    if ( ch == 2 )
    {
      m_dmac_begin = addr;
      if ( m_dmac_autoload == 1 )
      {
        m_dmac_ch[3]._addr = addr;
      }
    }
  }
}


void CRT::dmac_ch_cnt( int ch, int value )
{
  if ( m_dmac_ch[ch]._cnt_expect_hl == 0 )
  {
    m_dmac_ch[ch]._cnt_expect_hl = 1;
    m_dmac_ch[ch]._cnt_lo        = value;
  }
  else
  {
    m_dmac_ch[ch]._cnt_expect_hl = 0;
    m_dmac_ch[ch]._cnt_hi        = value & 0x3F;
    m_dmac_ch[ch]._wr_mode       = ( ( value & 0xC0 ) >> 6);
    uint16_t count = ( (uint16_t)m_dmac_ch[ch]._cnt_hi << 8 ) + ( (uint16_t)m_dmac_ch[ch]._cnt_lo ) + 1;
    m_dmac_ch[ch]._cnt    = count;
    m_dmac_ch[ch]._remain = count;
    if ( ch == 2 )
    {
      m_dmac_end = m_dmac_begin + count;
      if ( m_dmac_autoload == 1 )
      {
        m_dmac_ch[3]._cnt    = count;
        m_dmac_ch[3]._remain = count;
      }
    }
  }
}


void CRT::dmac_cmd( int value )
{
  uint8_t en = value & _EN_DMA_CH2;
  if ( en )
    m_crtc_text_visible &= _ENABLE_DMA_CH2;
  else
  {
    m_crtc_text_visible |= _DISABLE_DMA_CH2;
    m_dmac_penarty = 0;
  }
  m_dmac_dmaact = en;
  m_dmac_autoload = (value & _AUTOLOAD) != 0 ? 1 : 0;
  // clear except byte order
  for ( int ch = 0; ch < 4; ++ ch )
  {
    m_dmac_ch[ch]._addr_expect_hl = 0;
    m_dmac_ch[ch]._cnt_expect_hl = 0;
  }
}


void CRT::vrtc_reset( void )
{
  if ( m_crtc_line_mode == 1 )
  {
    // 25lines
    m_vrtc_hsync_dsp_state = 179;   // horizontal display interval (44.70us)
    m_vrtc_hsync_all_state = 250;   // horizontal blanking interval(17.88us/62.58us)
    m_vrtc_vsync_bck_state = 13994; // vertical blanking interval  (3.504ms)
  }
  else
  {
    // 20lines
    m_vrtc_hsync_dsp_state = 179;   // horizontal display interval (44.70us)
    m_vrtc_hsync_all_state = 250;   // horizontal blanking interval(17.88us/62.58us)
    m_vrtc_vsync_bck_state = 14996; // vertical blanking interval  (3.755ms)
  }
  m_vrtc_vsync_line       = 0;
  m_vrtc_vsync_state      = 0;
  m_vrtc_hsync_state      = 0;
  m_vrtc_flag             = 0;
  m_vrtc_hsync_start_flg  = 1;
  m_vrtc_dma_remain_chars = 0;
}


void CRT::vrtc_blanking( int ticks )
{
  int T = ticks;

  int result = _VRTC_DISP;
  uint8_t rasidx;

  if ( m_vrtc_vsync_line < 200)
  {
    // hsync start
    if ( m_vrtc_hsync_start_flg == 1 )
    {
      m_vrtc_hsync_start_flg = 0;
      // raster index of line
      rasidx = m_vrtc_vsync_line % m_crtc_raster_per_char;
      if ( rasidx == 0 )
      {
        // dma underrun error occured
        if ( m_vrtc_dma_remain_chars > 0 )
        {
          // ignore in emulator
          ;
        }
        // DMA transfer start (CPU WAIT) if dma ch.2 is active or crts start display
        if ( m_crtc_burst_mode == 1 )
        {
          // dma bulk transfer request (burst mode)
          m_dmac_penarty = ( m_crtc_line_bytes / 2 ) * 11;
          m_vrtc_dma_remain_chars = 0;
        }
        else
        {	
          // dma char by char transfer request
          m_vrtc_dma_remain_chars = m_crtc_line_bytes;
          m_dmac_penarty = 0;
        }
      }
    }
    // dma transfer
    if ( ( m_crtc_text_visible & 0x06 ) == 0 )
    {
      if ( m_vrtc_dma_remain_chars > 0 )
      {
        --m_vrtc_dma_remain_chars;
        m_dmac_penarty += ( m_vrtc_dma_remain_chars & 0x01 ) == 1 ? 5 : 6;
      }
    }
    else
    {
      // not-visible (crtc/dma stopped)
      m_vrtc_dma_remain_chars = 0;
      m_dmac_penarty = 0;
    }
    // vertical display interval
    m_vrtc_hsync_state += T;
    if ( m_vrtc_hsync_state <= m_vrtc_hsync_dsp_state )
    {
      // horizontal display interval
      ;
    }
    else if ( m_vrtc_hsync_state <= m_vrtc_hsync_all_state )
    {
      // horizontal blanking interval
      ;
    }
    else
    {
      // end of 1-line display
      if ( ++m_vrtc_vsync_line >= 200 )
      {
        // vertical blanking interval start
        m_vrtc_vsync_state = ( m_vrtc_hsync_state - m_vrtc_hsync_all_state );
        m_vrtc_flag = 1;
        vrtc_renew();
        m_dmac_penarty = 0;
        result = _VRTC_VBBGN;
      }
      else
        m_vrtc_hsync_state -= m_vrtc_hsync_all_state;
      m_vrtc_hsync_start_flg = 1;
    }
  }
  else
  {
    // vertical blanking interval
    m_vrtc_vsync_state += T;
    if ( m_vrtc_vsync_state >= m_vrtc_vsync_bck_state )
    {
      // end of VBI
      m_vrtc_vsync_line  = 0;
      m_vrtc_hsync_state = m_vrtc_vsync_state - m_vrtc_vsync_bck_state;
      m_vrtc_flag = 0;
      result = _VRTC_VBEND;
    }
  }

  if ( result == _VRTC_VBEND )
    vram2FrameBuffer();
}


void CRT::vrtc_renew( void )
{
  uint32_t cur_nible, att_nible;
  ++m_vrtc_blink;
  cur_nible = m_vrtc_blink % m_cursor_cur_blink_cycl;
  att_nible = m_vrtc_blink % m_cursor_att_blink_cycl;
  m_cursor_blinking      = cur_nible < m_cursor_cur_blink_disp ? 1 : 0;
  m_cursor_attr_blinking = att_nible < m_cursor_att_blink_disp ? 1 : 0;
}


void CRT::cursor_enable( void )
{
  m_cursor_enabled = true;
}


void CRT::cursor_disable( void )
{
  if ( m_cursor_enabled )
  {
    m_cursor_enabled = false;
    attr_set( m_cursor_y, 1 );
  }
}


void CRT::cursor_move( void )
{
  if ( !m_cursor_enabled )
  {
    m_cursor_x = m_cursor_new_col;
    m_cursor_y = m_cursor_new_row;
  }
  else
  {
    if ( m_cursor_new_row != m_cursor_y )
    {
      attr_set( m_cursor_new_row, 1 );
      attr_set( m_cursor_y, 1 );
      m_cursor_x        = m_cursor_new_col;
      m_cursor_y        = m_cursor_new_row;
      m_cursor_blinking = true;
    }
    else if ( m_cursor_new_col != m_cursor_x )
    {
      attr_set( m_cursor_y, 1 );
      m_cursor_x        = m_cursor_new_col;
      m_cursor_blinking = true;
    }
  }
}


void CRT::attr_cache( int line )
{
  uint8_t * vram = m_vram;
  int i, s, r;
  uint8_t * sort = &m_attr_attrcach[0];
  uint8_t * src  = &vram[(120 * line)];

  // VRAM 80 桁分をコピー
  for ( i = 0; i < 80; ++i )
    m_attr_textcach[i] = *src++;

  // VRAM 属性 40 桁を桁昇順でコピーする
  for ( i = 0; i < 20; ++i )
  {
    sort[i * 2 + 0] = 0x50;
    sort[i * 2 + 1] = 0;
  }
  for ( i = 0, s = 0; i < 20; ++i )
  {
    uint8_t colv = (*src++ & 0x7F);
    uint8_t attr = *src++;
    // 機能属性はメモリ中の並び通り
    sort[i * 2 + 1] = attr;
    // 桁指定部は昇順ソート
    if ( i == 0 )
    {
      sort[s * 2] = colv;
      ++s;
    }
    else if ( colv == sort[(s - 1) * 2] )
    {
      // 同一桁の指定は無視
      ;
    }
    else if ( colv > sort[(s - 1) * 2] )
    {
      sort[s * 2] = colv;
      ++s;
    }
    else
    {
      int find = 0;
      // 重複桁指定は無視する
      for ( r = 0; r < s; ++r )
      {
        if ( sort[r * 2] == colv )
        {
          find = 1;
          break;
        }
      }
      if ( !find )
      {
        // 昇順並べ替え
        for ( r = s; r > 0; --r )
        {
          if ( sort[(r - 1) * 2] > colv )
          {
            uint8_t c         = sort[(r - 1) * 2];
            sort[r * 2]       = c;
            sort[(r - 1) * 2] = colv;
          }
          else
            break;
        }
        ++s;
      }
    }
  }
}


void CRT::attr_movcolor( void )
{
  int i, j;
  uint8_t pac, prev_color = m_attr_prevcolor;

  // 色属性
  if ( m_crtc_color_mode == 1 )
  {
    // モノクロ時は白一色
    memset( &m_attr_eval_attr[0], 7, 80 );
  }
  else
  {
    int start_col = -1;
    memset( &m_attr_eval_attr[0], 0, 80 );
    for ( i = 0; i < 20; ++i )
    {
      uint8_t attr = m_attr_attrcach[i * 2 + 1];
      // 色属性指定の場合
      if ( attr & 0x08 )
      {
        int cols = m_attr_attrcach[i * 2];
        // 色属性開始位置の保存
        if ( start_col == -1 )
          start_col = cols > 80 ? 80 : cols;

        if ( cols < 80 )
        {
          // GRB -> RGB
          pac = GRB2RGB[(attr >> 5) & 0x07];
          // 簡易グラフィック属性
          if ( attr & 0x10 )
            pac |= 0x10;
          // 次行繰越色
          m_attr_prevcolor = pac;
          // プレーン属性配列書き込み
          for ( j = cols; j < 80; ++j )
            m_attr_eval_attr[j] = pac;
        }
      }
    }
    // 途中から開始されている場合は最終色属性で埋める
    if ( start_col > 0 )
    {
      for ( i = 0; i < start_col; ++i )
      {
        m_attr_eval_attr[i] = prev_color;
      }
    }
  }
}


void CRT::attr_movattr( void )
{
  int i, j;
  uint8_t prev_func = m_attr_prevfunc;
  uint8_t mask = m_crtc_color_mode == 0 ? 0x17 : 0x07;

  int start_col = -1;
  for ( i = 0; i < 20; ++i )
  {
    uint8_t attr = m_attr_attrcach[i * 2 + 1];
    // 機能属性指定の場合
    if ( m_crtc_color_mode == 1 || !( attr & 0x08 ) )
    {
      int cols = m_attr_attrcach[i * 2];
      // 機能属性開始位置の保存
      if ( start_col == -1 )
        start_col = cols > 80 ? 80 : cols;

      if ( cols < 80 )
      {
        // PD3301属性 -> PAC
        uint8_t pac = attr_convfunc( attr );
        // 点滅が指定されている場合は、ブリンク時に消す
        // 但し、消去属性が指定されている場合は、そちらを優先する
        if ( ( attr & 0x03 ) == 0x02 && !m_cursor_attr_blinking )
        {
          pac |= 0x40;
        }
        for ( j = cols; j < 80; ++j )
        {
          m_attr_eval_attr[j] &= mask;
          m_attr_eval_attr[j] |= pac;
        }
        // 次行繰越機能
        m_attr_prevfunc = pac;
      }
    }
  }
  // 途中から開始されている場合は最終色属性で埋める
  if ( start_col > 0 )
  {
    for ( i = 0; i < start_col; ++i )
    {
      m_attr_eval_attr[i] &= mask;
      m_attr_eval_attr[i] |= ( prev_func & mask );
    }
  }
  // 強制反転モードの場合
  if ( m_crtc_attr_reverse )
  {
    for ( i = 0; i < 80; ++i )
      m_attr_eval_attr[i] ^= 0x20;
  }
}


int CRT::attr_eval_attrib( int line )
{
  int next = line == 24 ? 0 : line + 1, result = 0;

  if ( line < 0 || line > 24 )
    return 0;

  m_attr_prevcolor = m_attr_nextcolor[line];
  m_attr_prevfunc  = m_attr_nextfunc[line];

  // 属性評価
  attr_movcolor();
  attr_movattr();

  // カーソル明滅の更新
  if ( m_cursor_y == line && m_cursor_blinking && m_cursor_enabled )
  {
    // reverse
    m_attr_eval_attr[m_cursor_x] ^= 0x20;
    if ( !( m_attr_eval_attr[m_cursor_x] & 0x07 ) )
      // color white
      m_attr_eval_attr[m_cursor_x] |= 0x07;
  }

  // 次行への繰越色が変更になった場合
  if ( m_attr_nextcolor[next] != m_attr_prevcolor )
    result = 1;
  m_attr_nextcolor[next] = m_attr_prevcolor;
  m_attr_nextfunc[next]  = m_attr_prevfunc;

  return result;
}


uint8_t CRT::attr_convfunc( uint8_t attr )
{
  uint8_t maskBit, al, cl = 0;
  if ( m_crtc_color_mode == 0 )
    maskBit = 0x37;
  else
    maskBit = 0xF7;

  al = attr & maskBit;
  if ( al & 0x01 )  // 消去
    cl |= 0x40;
  if ( al & 0x04 )  // 反転
    cl |= 0x20;
  if ( al & 0x80 )  // グラフィック
    cl |= 0x10;
  if ( al & 0x10 )  // 上線
    cl |= 0x08;
  if ( al & 0x20 )  // 下線
    cl |= 0x80;

  return cl;
}


void CRT::attr_invalid( void )
{
  for ( int i = 0; i < 25; ++i )
    m_attr_attrchg[i] = 1;
}


void CRT::attr_set( int line, uint8_t flg )
{
  if ( line < 0 || line > 24 )
    return;
  m_attr_attrchg[line] = flg;
}


void CRT::vram2FrameBuffer( void )
{
  bool change_attr = false, change_prev = false;
  int colAddCount = m_crtc_screen_mode == 0 || m_crtc_screen_mode == 1 ? 2 : 1;
  uint8_t * frameBuffer = m_frameBuffer;
  for ( int row = 0; row < 25; row++ )
  {
    if ( m_attr_attrchg[row] == 1 || change_prev )
    {
      attr_cache( row );
      change_attr = true;
    }
    else
      change_attr = false;

    if ( change_attr )
      change_prev = attr_eval_attrib( row ) == 1 ? true : false;

    if ( !change_attr )
      continue;

    if ( ( m_crtc_screen_mode == 0 || m_crtc_screen_mode == 2 ) && row >= 20 )
      // 40x20 or 80x20 & row >= 20
      continue;

    uint8_t * txts = &m_attr_textcach[0];
    uint8_t * pacs = &m_attr_eval_attr[0];
    for ( int col = 0; col < 80; col += colAddCount )
    {
      uint8_t code = *txts;
      uint8_t pack = *pacs;
      if ( m_crtc_text_visible != 0 )
        pack = 0x40;
      *(frameBuffer++) = code;
      *(frameBuffer++) = pack;
      txts += colAddCount;
      pacs += colAddCount;
    }
  }
}

