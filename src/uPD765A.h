/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on upd765a.h in xm8_170.zip
  (http://retropc.net/pi/xm8/xm8_170.zip)

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

#include "Arduino.h"

#include "emu.h"
#include "media.h"

                                // Main Status Register
#define S_D0B       (0x01)      // FDD 0 Busy
#define S_D1B       (0x02)      // FDD 1 Busy
#define S_D2B       (0x04)      // FDD 2 Busy
#define S_D3B       (0x08)      // FDD 3 Busy
#define S_CB        (0x10)      // FDC Busy
#define S_NDM       (0x20)      // Execution Mode
#define S_DIO       (0x40)      // Data Input/Output
#define S_RQM       (0x80)      // Request for Master

                                // Status Register 0
#define ST0_NR      (0x000008)  // Not Ready
#define ST0_EC      (0x000010)  // Equipment Check
#define ST0_SE      (0x000020)  // Seek End
#define ST0_AT      (0x000040)  // Abnormal termination of command, (AT)
#define ST0_IC      (0x000080)  // Invalid command issue, (IC)
#define ST0_AI      (0x0000C0)  // Abnormal termination because during command execution the ready srgnal from FDD changed state

                                // Status Register 1
#define ST1_MA      (0x000100)  // Missing Address Mark
#define ST1_NW      (0x000200)  // Not Writeable
#define ST1_ND      (0x000400)  // No Data
#define ST1_OR      (0x001000)  // Overrun
#define ST1_DE      (0x002000)  // Data Error
#define ST1_EN      (0x008000)  // End of Cylinder

                                // Status Register 2
#define ST2_MD      (0x010000)  // Missing Address Mark in Data Field
#define ST2_BC      (0x020000)  // Bad Cylinder
#define ST2_SN      (0x040000)  // Scan Not Satis fied
#define ST2_SH      (0x080000)  // Scan Equal Hit
#define ST2_NC      (0x100000)  // Wrong Cylinder
#define ST2_DD      (0x200000)  // Data Error in Data Field
#define ST2_CM      (0x400000)  // Control Mark

                                // Status Register 3
#define ST3_US0     (0x01)      // Uint Select 0
#define ST3_US1     (0x02)      // Unit Select 1
#define ST3_HD      (0x04)      // Head Address
#define ST3_TS      (0x08)      // Two-Side
#define ST3_T0      (0x10)      // Track 0
#define ST3_RY      (0x20)      // Ready
#define ST3_WP      (0x40)      // Write Protected
#define ST3_FT      (0x80)      // Fault

#define DRIVE_MASK  (0x03)

#define SIG_UPD765A_TC  1


#define REGISTER_PHASE_EVENT( phs, usec ) { \
  m_eventPhase = phs; \
  m_eventPhaseTimer = micros() + usec; \
}

#define REGISTER_PHASE_EVENT_NEW( phs, usec ) { \
  m_eventPhase = phs; \
  m_eventPhaseTimer = micros() + usec; \
}

#define CANCEL_EVENT() { \
  m_eventPhase = (PHASE)-1; \
  m_eventPhaseTimer = 0; \
}

#define REGISTER_EVENT( evt, usec ) { \
  m_eventID = evt; \
  m_eventTimer[evt] = micros() + usec; \
}


class uPD765A
{
public:
  typedef struct _TAG_FDD {
    uint8_t   track;
    uint8_t   result;
    bool      access;
  } _FDD, * _PFDD;

  enum EVENT {
    EVENT_PHASE = 0,
    EVENT_DRQ,
    EVENT_LOST,
    EVENT_RESULT7,
    EVENT_INDEX,
    EVENT_SEEK_DRV0,
    EVENT_SEEK_DRV1,
    EVENT_SEEK_DRV2,
    EVENT_SEEK_DRV3,
  };

  enum PHASE {
    PHASE_IDLE = 0,
    PHASE_CMD,
    PHASE_EXEC,
    PHASE_READ,
    PHASE_WRITE,
    PHASE_SCAN,
    PHASE_TC,
    PHASE_TIMER,
    PHASE_RESULT,
  };

  uPD765A();
  ~uPD765A();

  bool reset( bool * requestIRQ, media * disk, uint8_t * d88rwBuffer, SoundGenerator * soundGenerator );
  void softReset( void )                    { bgmSeekEnd(); }
  int readIO( int address );
  void writeIO( int address, int value );
  void write_signal( int id );
  void tick( void );

private:
  void set_irq( bool enableIRQ );
  void process_cmd( int cmd );
  void cmd_read_diagnostic( void );
  void cmd_specify( void );
  void cmd_sense_drive_status( void );
  void cmd_write_data( void );
  void cmd_read_data( void );
  void cmd_recalibrate( void );
  void cmd_sense_interrupt_status( void );
  void cmd_read_id( void );
  void cmd_write_id( void );
  void cmd_seek( void );
  void cmd_scan( void );
  void cmd_invalid( void );
  void seek( int drv, int trk );
  void seek_event( int drv );
  void read_data( bool deleted, bool scan );
  void write_data( bool deleted );
  uint32_t read_sector( void );
  uint32_t write_sector( bool deleted );
  uint8_t get_devstat( int drv );
  void set_hdu( uint8_t value );
  uint32_t check_cond( bool write );
  uint32_t find_id( void );
  uint32_t write_id( void );
  void get_sector_params( void );
  void shift_to_idle( void );
  void shift_to_cmd( int bufferCount );
  void shift_to_read( int bufferCount );
  void shift_to_write( int bufferCount );
  void shift_to_scan( int bufferCount );
  void shift_to_result( int bufferCount );
  void shift_to_result7( void );
  bool id_incr( void );
  int get_usec_to_exec_phase( void );
  void bgmSeekBegin( void );
  void bgmSeekEnd( void );
  static __inline bool checkTimer( uint32_t t )
  {
    if ( t != 0 && micros() > t )
      return true;
    return false;
  }

  bool *      m_requestIRQ;
  uint8_t *   m_d88rwBuffer;
  uint8_t *   m_sectorBuffer;
  size_t      m_sectorSize;
  _FDD        m_fdd[5];
  media *     m_media;

  uint8_t     m_hdu;
  uint8_t     m_hdue;
  uint8_t     m_id[4];
  uint8_t     m_eot;
  uint8_t     m_gpl;
  uint8_t     m_dtl;
  uint8_t     m_sc;
  uint8_t     m_fill;

  PHASE       m_phase;
  PHASE       m_prevphase;
  uint8_t     m_mainStatus;
  int         m_command;
  uint32_t    m_result;

  uint8_t *   m_buffer;
  uint8_t *   m_bufferPtr;
  int         m_bufferSize;
  int         m_bufferCount;
  PHASE       m_eventPhase;
  uint32_t    m_eventPhaseTimer;
  EVENT       m_eventID;
  uint32_t    m_eventTimer[9];
  uint32_t    m_bgmSeekTimer;

  bool        m_forceReady;
  int         m_stepRateTime;
  bool        m_noDmaMode;

  SoundGenerator    * m_soundGenerator;   // fabgl library : class SoundGenerator
  SamplesGenerator  * m_bgmSeek;          // fabgl library : class SamplesGenerator
};

