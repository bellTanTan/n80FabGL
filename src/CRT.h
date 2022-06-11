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


#pragma once

#include "fabgl.h"
#include "fabutils.h"


using namespace fabgl;

#include "emu.h"

#define	_VRTC_DISP        (0x00)  // crt display
#define _VRTC_VBBGN       (0x01)  // crt enter vertical blanking
#define	_VRTC_VBEND       (0x02)  // crt leave vertical blanking

#define _NONE             (0)
#define _OCW1             (1)
#define _OCW5             (5)
#define _STOP_DISPLAY     (0x02)
#define _START_DISPLAY    (0xFD)
#define _OCW1_DM          (0x01)	// 0:normal, 1:reverse

#define _EN_DMA_CH2       (0x04)
#define _DISABLE_DMA_CH2  (0x04)
#define _ENABLE_DMA_CH2   (0xFB)
#define _DMA_UPDATE_FLAG  (0x10)
#define _AUTOLOAD         (0x80)


class CRT
{
public:
  // callbacks
  typedef void (* setScreenModeCallback)( void * context, int value );

  void setCallbacks( void * context, setScreenModeCallback setScreenMode )
  {
    m_context       = context;
    m_setScreenMode = setScreenMode;
  }

  bool reset( uint8_t * vram, uint8_t * frameBuffer );

  bool readIO( int address, int * result );

  bool writeIO( int address, int value );

  void tick( int cycles );

  uint8_t getCrtcTextVisible( void )      { return m_crtc_text_visible; }
  uint32_t getDmacPenarty( void )         { return m_dmac_penarty; }
  void setDmacPenarty( uint32_t value )   { m_dmac_penarty = value; }

private:
  void crtc_mode( int value );
  void crtc_cmd( int value );
  void crtc_data( int value );
  void dmac_ch_addr( int ch, int value );
  void dmac_ch_cnt( int ch, int value );
  void dmac_cmd( int value );
  void vrtc_reset( void );
  void vrtc_blanking( int ticks );
  void vrtc_renew( void );
  void cursor_enable( void );
  void cursor_disable( void );
  void cursor_move( void );
  void attr_cache( int line );
  void attr_movcolor( void );
  void attr_movattr( void );
  int attr_eval_attrib( int line );
  uint8_t attr_convfunc( uint8_t attr );
  void attr_invalid( void );
  void attr_set( int line, uint8_t flg );
  void vram2FrameBuffer( void );

  uint8_t     m_crtc_burst_mode;        // DMA burst mode 0:off/1:on
  uint8_t     m_crtc_chars_per_line;    // 2..129 chars per line
  uint8_t     m_crtc_lines_per_screen;  // lines count
  uint8_t     m_crtc_raster_per_char;   // raster lines per char
  uint8_t     m_crtc_ver_ret_lines;     // vertical return lines
  uint8_t     m_crtc_hol_ret_chars;     // holizontal return chars
  uint8_t     m_crtc_attribute_en;      // attribute 0:disabled/1:enabled
  uint8_t     m_crtc_attribute_mono;    // attribute 0:color/1:mono
  uint8_t     m_crtc_attribute_tp;      // attribute 0:non-transparent/1:transparent
  uint8_t     m_crtc_special_char;      // special char 0:disabled/1:enabled
  uint8_t     m_crtc_max_attr_cnt;      // max attribute counts
  uint16_t    m_crtc_line_bytes;        // char+attribute+sc size
	uint32_t    m_crtc_screen_mode;       // 0:40/20, 1:40/25, 2:80/20, 3:80/25
	uint8_t     m_crtc_color_mode;        // color mode        0:color, 1:mono
	uint8_t     m_crtc_width_mode;        // 80/40 colmun mode 0:40, 1:80
  uint8_t     m_crtc_line_mode;         // 20/25 line mode   0:20, 1:25
  uint8_t     m_crtc_attr_reverse;      // attribute color reverse mode flag
  uint16_t    m_crtc_param_count;       // amount of params already accepted.
  uint8_t     m_crtc_prev_cmd;          // current command which is waiting for params
  uint8_t     m_crtc_text_visible;      // text visible: 0:visible, not 0:invisible (b0:TXTDS/b1:crtc stop/b2:dmac stop)

  uint32_t    m_dmac_penalty_base;      // DMA bus stop penalty base T-states
  uint32_t    m_dmac_penarty;           // DMA transfer penarty
  uint32_t  	m_dmac_begin;             // DMA begin addr
	uint32_t    m_dmac_end;               // DMA end addr
	uint8_t     m_dmac_autoload;          // DMA autoload mode 0:off/1:on
	uint8_t     m_dmac_dmaact;            // DMA active flag
	struct _N80DMACH_tag {
  	uint8_t   _addr_lo;
  	uint8_t   _addr_hi;
  	uint8_t   _addr_expect_hl;          // expect addr 0:lo/1:hi
  	uint8_t   _cnt_lo;
  	uint8_t   _cnt_hi;
  	uint8_t   _cnt_expect_hl;           // expect cnt 0:lo/1:hi
  	uint8_t   _wr_mode;                 // 00:verify, 01:write, 10:read
  	uint16_t  _addr;                    // ch address
  	uint16_t  _cnt;                     // terminal count
  	uint16_t  _remain;
  } m_dmac_ch[4];                       // DMA 4 channels

  uint8_t     m_vrtc_hsync_start_flg;   // hsync start 1:yes, 0:no
  uint8_t     m_vrtc_flag;              // vertical blanking flag
  uint16_t    m_vrtc_dma_remain_chars;  // holizontal remain chars via dma transfer
  uint32_t    m_vrtc_blink;             // blanking interval
  int         m_vrtc_vsync_line;        // current line
  uint32_t    m_vrtc_vsync_state;       // current vsync T-state counter
  uint32_t    m_vrtc_hsync_state;       // current hsync T-state counter
  uint32_t    m_vrtc_hsync_dsp_state;   // h-freq disp:44.70us=179T/blank:17.88us=71T
  uint32_t    m_vrtc_hsync_all_state;   // h-freq 62.58us=250T()
  uint32_t    m_vrtc_vsync_bck_state;   // v-freq 16.02ms=64080T(disp:12.52ms=50080T/blank:3.504ms=14016T)

  uint16_t    m_cursor_cur_blink_cycl;  // cursor blinking cycle
  uint16_t    m_cursor_att_blink_cycl;  // attribute blinking cycle
  uint16_t    m_cursor_cur_blink_disp;  // cursor blinking display period
  uint16_t    m_cursor_att_blink_disp;  // attribute blinking display period
  uint8_t     m_cursor_x;               // cursor location x (col)
  uint8_t     m_cursor_y;               // cursor location y (row)
  uint8_t     m_cursor_new_col;         // cursor new location x
  uint8_t     m_cursor_new_row;         // cursor new location y
  bool        m_cursor_enabled;         // cursor enable
  bool        m_cursor_blinking;        // cursor blink status
  bool        m_cursor_attr_blinking;	  // attribute blink status

  uint8_t     m_attr_eval_attr[80];     // evaluated attribute
  uint8_t     m_attr_textcach[80];      // text cach
  uint8_t     m_attr_attrcach[40];      // attr cach
  uint8_t     m_attr_attrchg[25];       // last changed attr
  uint8_t     m_attr_sortstk[22];       // attr area sort stack
  uint8_t     m_attr_headzero;          // area start zero flag
  uint8_t     m_attr_nextcolor[25];     // attr effective color
  uint8_t     m_attr_nextfunc[25];      // attr effective func
  uint8_t     m_attr_prevcolor;         // previous color
  uint8_t     m_attr_prevfunc;          // previous func

  void *                m_context;      // callbacks
  setScreenModeCallback m_setScreenMode;
                                        // callback : setScreenMode()
  uint8_t *   m_vram;                   // v-ram pointer
  uint8_t *   m_frameBuffer;            // packeduPD3301 ≒ CGA frame buffer pointer
};

