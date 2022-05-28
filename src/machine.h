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

#pragma GCC optimize ("O2")

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_int_wdt.h"

#include "fabgl.h"
#include "fabutils.h"
#include "emudevs/Z80.h"

using namespace fabgl;

#include "emu.h"
#include "packeduPD3301adpter.h"
#include "BUZZER.h"
#include "CALENDAR.h"
#include "CMT.h"
#include "CRT.h"
#include "DISK.h"
#include "KEYBRD.h"
#include "PCG.h"
#include "PRINTER.h"


////////////////////////////////////////////////////////////////////////////////////
// Machine

typedef void (*MenuCallback)();


class Machine
{
public:

  enum requestCpuCmd {
    cmdNon = 0,
    cmdReset,
    cmdAnyBreak,
    cmdSelBreak,
    cmdStep,
    cmdContinue,
    cmdSetPCSP,
    cmdEspRestart,
  };

  typedef struct _TAG_MSG_REQUEST_CPU_CMD {
    requestCpuCmd   cmd;
    uint16_t        PC;
    uint16_t        SP;
    uint16_t        BP;
  } msgRequestCpuCmd, *pmsgRequestCpuCmd;

  Machine();

  ~Machine();

  int init( bool selDiskRomEnable = true );

  bool romload( const char * fileName );

  bool fontload( const char * fileName );

  bool attachIO( int IOSize );

  bool attachRAM( int RAMSize );

  bool attachCMT( int CMTBufferSize );

  void setMenuCallback( MenuCallback value ) { m_menuCallback = value; }

  void run( void );
  void backGround( void );

  static int readByte( void * context, int address );
  static void writeByte( void * context, int address, int value );

  static int readWord( void * context, int addr )              { return readByte( context, addr ) | ( readByte( context, addr + 1) << 8 ); }
  static void writeWord( void * context, int addr, int value ) { writeByte( context, addr, value & 0xFF ); writeByte( context, addr + 1, value >> 8 ); }

  static int readIO( void * context, int address );
  static void writeIO( void * context, int address, int value );

  packeduPD3301adpter * cga( void )         { return &m_CGA; }
  void menu2FileName( const char * value )  { strcpy( m_menu2FileName, value ); }
  void menuCmdReset( void )                 { m_menuCmdReset = true; }
  void menuCmdEspRestart( void )            { m_menuCmdEspRestart = true; }
  bool getDiskRomEnable( void )             { return m_diskRomEnable; }
  bool get2kRomLoaded( void )               { return m_DISK.get2kRomLoaded(); }

  bool setDiskImage( int drive, bool writeProtected, const char * imgFileName );
  const char * getDiskImageFileName( int drive );  

  bool cmtSave( const char * cmtFileName );
  bool cmtLoad( const char * cmtFileName );
  bool n80Load( const char * n80FileName );

  void outputEsp32Spec( void )
  {
    _DEBUG_PRINT( "\r\n" );
#ifdef ESP_ARDUINO_VERSION
    _DEBUG_PRINT( "ESP_ARDUINO_VERSION(0x%04X)\r\n", ESP_ARDUINO_VERSION );
#endif // ESP_ARDUINO_VERSION
    _DEBUG_PRINT( "FABGL_ESP_IDF_VERSION(0x%04X)\r\n", FABGL_ESP_IDF_VERSION );
    _DEBUG_PRINT( "Internal Total Heap %d\r\n", ESP.getHeapSize() );
    _DEBUG_PRINT( "Internal Free  Heap %d\r\n", ESP.getFreeHeap() );
    _DEBUG_PRINT( "SPIRam Total Heap %d\r\n", ESP.getPsramSize() );
    _DEBUG_PRINT( "SPIRam Free  Heap %d\r\n", ESP.getFreePsram() );
    _DEBUG_PRINT( "Flash Size %d\r\n", ESP.getFlashChipSize() );
    _DEBUG_PRINT( "Flash Speed %d\r\n", ESP.getFlashChipSpeed() );
    _DEBUG_PRINT( "Chip Revision %d\r\n", ESP.getChipRevision() );
    _DEBUG_PRINT( "CPU Freq. %d MHz\r\n", ESP.getCpuFreqMHz() );
    _DEBUG_PRINT( "CPU Cores %d\r\n", ESP.getChipCores() );
    _DEBUG_PRINT( "SDK %s\r\n\r\n", ESP.getSdkVersion() );
  }

  void outputEsp32FreeHeap( void )
  {
    _DEBUG_PRINT( "Internal Free Heap %d\r\n", ESP.getFreeHeap() );
    _DEBUG_PRINT( "SPIRam   Free Heap %d\r\n", ESP.getFreePsram() );
    _DEBUG_PRINT( "MALLOC_CAP_32BIT : %d bytes (largest %d bytes)\r\n",
                  heap_caps_get_free_size( MALLOC_CAP_32BIT ),
                  heap_caps_get_largest_free_block( MALLOC_CAP_32BIT ) );
    _DEBUG_PRINT( "MALLOC_CAP_8BIT  : %d bytes (largest %d bytes)\r\n",
                  heap_caps_get_free_size( MALLOC_CAP_8BIT ),
                  heap_caps_get_largest_free_block( MALLOC_CAP_8BIT ) );
    _DEBUG_PRINT( "MALLOC_CAP_DMA   : %d bytes (largest %d bytes)\r\n\r\n",
                  heap_caps_get_free_size( MALLOC_CAP_DMA ),
                  heap_caps_get_largest_free_block( MALLOC_CAP_DMA ) );
  }

private:
  static void cpuTask( void * pvParameters );

  void makeGraphFont( void );
  void softReset( void * context );
  void setRequestCpuCmd( requestCpuCmd cmd, uint16_t PC = 0, uint16_t SP = 0, uint16_t BP = 0 );

  static void setScreenMode( void * context, int value );
  static void setPCGMode( void * context, int value );
  static void updatePCGFont( void * context );
  static void vkf9( void * context, int value );
  static void vkf10( void * context, int value );
  static void vkf11( void * context, int value );
  static void vkf12( void * context, int value );

#ifdef _DEBUG
  void debugBreak( bool any, int address );
  void debugContinue( void );
  void debugStep( void );
  void debugHelp( void );
  void debugCommand( void );
#endif // _DEBUG

  void registerDump( void * context, const char * msg = NULL );
  void ioDump( int type, int address );
  void memoryDump( int address, int size );
  void memoryLoad( int loadAdrs, const char * fileName );
  void memorySave( int startAdrs, int endAdrs );

  uint8_t *       m_RAM;                    // VM memory : ROM 0x0000 ~ 0x7FFF , RAM 0x8000 ~ 0xFFFF
  uint8_t *       m_RAMExt;                 // VM memory : ext RAM 0000h ~ 7FFFh
  uint8_t *       m_FONT;                   // font data : 256 * 8 * 3set
  int             m_RAMSize;                // VM memory size
  int             m_RAMExtSize;             // VM memory ext RAM size
  int             m_IOSize;                 // VM I/O size
  uint8_t *       m_IoIn;                   // VM I/O input port buffer
  uint8_t *       m_IoOut;                  // VM I/O output port buffer
  uint8_t *       m_fileIOBuf;              // CMT/N80/bin file read write buffer
  int             m_fileIOBufSize;          // CMT/N80/bin file read write buffer size
  uint8_t *       m_n80LoadRam;             // N80 file load ram
  int             m_n80LoadRamSize;         // N80 file load ram size
  uint8_t *       m_cmtBuffer;              // CMT buffer
  int             m_cmtBufferSize;          // CMT buffer size
  uint8_t         m_portE2writeData;        // I/O output port E2h write data
                                            // 76543210
                                            //    |   +-- ext RAM Read
                                            //    +------ ext RAM Write
  MenuCallback    m_menuCallback;           // menu

static uint8_t *  s_videoMemory;            // fabgl VGA memory
  uint8_t *       m_frameBuffer;            // fabgl VGA memory
  packeduPD3301adpter
                  m_CGA;                    // packed uPD3301 ≒ CGA adpter
  PS2Controller   m_PS2Controller;          // fabgl library : class PS2Controller
  Keyboard *      m_keyboard;               // fabgl library : class Keyboard
  SoundGenerator  m_soundGenerator;         // fabgl library : class SoundGenerator
  Z80             m_Z80;                    // fabgl library : class Z80
  FontInfo        m_FontPC8001;             // fabgl struct PC-8001 normal font
  FontInfo        m_FontPC8001Graph;        // fabgl struct PC-8001 simple graphics font
  FontInfo        m_FontPCG;                // fabgl struct PCG-8100 font
  int             m_PCGMode;                // PCG-8100 0:OFF 1:ON
  char            m_menu2FileName[256];     // menu select load file name
  char            m_fdImgFileName[4][256];  // floppy disk 0 ~ 3 image file name (*.D88)
  bool            m_printerEnable;          // flag : printer enable
  bool            m_diskRomEnable;          // flag : floppy disk rom enable
  bool            m_diskEnable;             // flag : floppy disk enable
  volatile bool   m_menuCmdReset;           // flag : menu to CPU Reset
  volatile bool   m_menuCmdEspRestart;      // flag : menu to ESP32 restart
  bool            m_cmtLoad;                // flag : CMT type load end
  bool            m_n80Load;                // flag : N80 type load end
  int             m_n80LoadSize;            // N80 file load size
  uint16_t        m_requestPC;              // request set PC
  uint16_t        m_requestSP;              // request set SP

  BUZZER          m_BUZZER;                 // PC-8001 Device BUZZER
  CALENDAR        m_CALENDAR;               // PC-8001 Device CALENDAR
  CMT             m_CMT;                    // PC-8001 Device CMT
  CRT             m_CRT;                    // PC-8001 Device CRT
  DISK            m_DISK;                   // PC-80S31 (=PC-8801mkII SR/FR DISK.ROM 2Kbytes)
  KEYBRD          m_KEYBRD;                 // PC-8001 Device KEYBRD
  PCG             m_PCG;                    // HAL PCG-8100
  PRINTER         m_PRINTER;                // PC-8001 Device PRINTER

#ifdef _DEBUG
  char            m_serialRecvBuf[64];      // debug command Serial recive buffer
  int             m_serialRecvIndex;        // debug command Serial recive buffer index
  int             m_lastDumpIoInAdrs;       // debug command I/O input port last adderss
  int             m_lastDumpIoOutAdrs;      // debug command I/O output port last adderss
  int             m_lastDumpMemoryAdrs;     // debug command Memory dump last adderss
  int             m_lastDumpDiskIoInAdrs;   // debug command Floppy DISK uint I/O input port last adderss
  int             m_lastDumpDiskIoOutAdrs;  // debug command Floppy DISK uint I/O output port last adderss
  int             m_lastDumpDiskMemoryAdrs; // debug command Floppy DISK uint Memory dump last adderss
  QueueHandle_t   m_queueHandle;            // queue handle
#endif // _DEBUG

  TaskHandle_t    m_taskHandle;             // task handle
  uint32_t        m_ticksCounter;           // tick counter
  long            m_timerDiskCpuSuspend;
  long            m_timerDiskCpuSuspendReset;
  uint32_t        m_beforeTickTime;         // tick time microsec
};
