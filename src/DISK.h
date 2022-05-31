/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on FabGL/examples/VGA/Altair8800/src/machine.h and
  FabGL/examples/VGA/PCEmulator/machine.h
  (http://www.fabglib.org/)

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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "fabgl.h"
#include "fabutils.h"
#include "emudevs/Z80.h"

using namespace fabgl;

#include "media.h"
#include "uPD765A.h"


class DISK
{
public:
  enum requestCpuCmd {
    cmdNon = 0,
    cmdReset,
    cmdAnyBreak,
    cmdSelBreak,
    cmdStep,
    cmdContinue,
  };

  typedef struct _TAG_MSG_REQUEST_CPU_CMD {
    requestCpuCmd   cmd;
    uint16_t        BP;
  } msgRequestCpuCmd, *pmsgRequestCpuCmd;

  DISK();

  ~DISK();

  bool romLoad( const char * fileName );

  bool insert( int drive, bool writeProtected, const char * imgFileName );

  void eject( int drive );

  void setScreenMode( int value );

  bool reset( uint8_t * vram, bool diskEnable, SoundGenerator * soundGenerator );
  void softReset( void )                    { m_uPD765A.softReset(); }

  static int readByte( void * context, int address );
  static void writeByte( void * context, int address, int value );

  static int readWord( void * context, int addr )              { return readByte( context, addr ) | ( readByte( context, addr + 1) << 8 ); }
  static void writeWord( void * context, int addr, int value ) { writeByte( context, addr, value & 0xFF ); writeByte( context, addr + 1, value >> 8 ); }

  static int readIO( void * context, int address );
  static void writeIO( void * context, int address, int value );

  bool pcReadIO( int address, int * result, int pc = 0 );
  bool pcWriteIO( int address, int value, int pc = 0 );

  bool get2kRomLoaded( void )               { return m_2kRomLoaded; }

#ifdef _DEBUG
  void debugBreak( bool any, int address );
  void debugContinue( void );
  void debugStep( void );
#endif // _DEBUG

  void registerDump( void * context, const char * msg = NULL );
  void memoryDump( int address, int size );
  void ioDump( int type, int address );

  int getRAMSize( void )                    { return m_RAMSize; }
  int getIOSize( void )                     { return m_IOSize; }
  int getTaskSuspended( void )              { return m_taskSuspended; }
  void taskResume( void )                   { taskCheckResume( 0x80 ); }

private:
  static void cpuTask( void * pvParameters );

  void taskCheckSuspend( DISK * m );
  void taskRequestSuspend( DISK * m );
  void taskCheckResume( int value );
  void updateDiskAccessIndicator( bool clear = false );

#ifdef _DEBUG
  void setRequestCpuCmd( requestCpuCmd cmd, uint16_t BP = 0 );
#endif // _DEBUG

  media           m_media[MAX_DRIVE];     // Media DISK
  uint8_t *       m_RAM;                  // PC-8031-2W/PC-80S31 Memory Map
                                          // 0000h~07FFh : ROM
                                          // 0800h~3FFFh : NON
                                          // 4000h~4FFFh : RAM fd Write Buffer
                                          // 5000h~5FFFh : RAM fd Read Buffer
                                          // 6000h~7EFFh : RAM User Area
                                          // 7F00h~7FFFh : RAM Work Area
  int             m_RAMSize;              // VM memory size
  uint8_t *       m_IoIn;                 // VM I/O input port buffer
  uint8_t *       m_IoOut;                // VM I/O output port buffer
  int             m_IOSize;               // VM I/O size
  Z80             m_Z80;                  // fabgl library : class Z80
  uPD765A         m_uPD765A;              // uPD765A
  uint8_t *       m_d88ReadWriteBuffer;   // D88 Type disk read/write buffer (1Sector = 16+256bytes)
  uint8_t *       m_vram;                 // v-ram pointer
  int             m_statusAdrs;           // Status address of disk access
  const char *    m_indicatorMessage;     // Disk access indicator motor status display message
  int             m_indicatorSeqNo;       // Disk access indicator motor status display sequence no
  int             m_indicatorNextSeqNo;   // Disk access indicator motor status display next sequence no
  int             m_disk2Pc8255PortC;     // Floppy Disk -> PC 8255 Port C write data
  int             m_pc2Disk8255PortC;     // PC -> Floppy Disk 8255 Port C write data
  bool            m_diskEnable;           // flag : Floppy Disk enable
  bool            m_2kRomLoaded;          // flag : 2K bytes rom type loaed
  volatile bool   m_requestIRQ;           // flag : IRQ request
  volatile bool   m_requestTaskSuspend;   // flag : Task Suspend request
  volatile bool   m_taskSuspended;        // falg : Task Suspended
  volatile int    m_timerSuspend;         // Remaining scan time of the Task Suspend
  int             m_timerSuspendReset;    // Scan time of the Task Suspend
  uint32_t        m_beforeTickTime;       // tick time microsec

#ifdef _DEBUG
  QueueHandle_t   m_queueHandle;          // queue handle
#endif // _DEBUG

  TaskHandle_t    m_taskHandle;           // task handle
  portMUX_TYPE    m_mutexIoIn;            // mutex I/O input
  portMUX_TYPE    m_mutexIoOut;           // mutex I/O output
  uint32_t        m_ticksCounter;         // tick counter
};
