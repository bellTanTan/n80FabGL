/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on FabGL/examples/VGA/Altair8800/src/machine.cpp and
  FabGL/examples/VGA/PCEmulator/machine.cpp
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


#include "Arduino.h"

#include "machine.h"

uint8_t * Machine::s_videoMemory;


Machine::Machine()
{
  m_RAM               = NULL;
  m_RAMExt            = NULL;
  m_FONT              = NULL;
  m_IoIn              = NULL;
  m_IoOut             = NULL;
  m_cmtBuffer         = NULL;
  m_menuCallback      = NULL;
  m_RAMSize           = 0;
  m_RAMExtSize        = 0;
  m_IOSize            = 0;
  m_cmtBufferSize     = 0;
  m_portE2writeData   = 0;
  m_PCGMode           = 0;
  m_printerEnable     = true;
  m_diskRomEnable     = false;
  m_diskEnable        = false;
  m_menuCmdReset      = false;
  m_menuCmdEspRestart = false;
  m_cmtLoad           = false;
  m_n80Load           = false;
  m_n80LoadRam        = NULL;
  m_n80LoadRamSize    = 0;
  m_n80LoadSize       = 0;
  m_fileIOBuf         = NULL;
  m_fileIOBufSize     = 0;
  m_requestPC         = 0;
  m_requestSP         = 0;
  m_ticksCounter      = 0;
  memset( m_menu2FileName, 0, sizeof( m_menu2FileName ) );
  memset( m_fdImgFileName, 0, sizeof( m_fdImgFileName ) );

  sprintf( m_version, "n80FabGL v%d.%d.%d SDK %s",
           N80FABGL_VERSION_MAJOR,
           N80FABGL_VERSION_MINOR,
           N80FABGL_VERSION_REVISION,
           ESP.getSdkVersion() );

  m_Z80.setCallbacks( this, readByte, writeByte, readWord, writeWord, readIO, writeIO );

  m_timerDiskCpuSuspendReset = 30 * 60 * 1000 * 1000;
  m_timerDiskCpuSuspend      = m_timerDiskCpuSuspendReset;

#ifdef _DEBUG
  m_serialRecvIndex        = 0;
  m_lastDumpIoInAdrs       = 0;
  m_lastDumpIoOutAdrs      = 0;
  m_lastDumpMemoryAdrs     = 0;
  m_lastDumpDiskIoInAdrs   = 0;
  m_lastDumpDiskIoOutAdrs  = 0;
  m_lastDumpDiskMemoryAdrs = 0;
  memset( m_serialRecvBuf, 0, sizeof( m_serialRecvBuf ) );
#endif // _DEBUG
}


Machine::~Machine()
{
}


int Machine::init( bool selDiskRomEnable )
{
  s_videoMemory = (uint8_t *)heap_caps_malloc( VIDEO_MEM_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
  if ( !s_videoMemory ) return -1;
  m_frameBuffer = s_videoMemory;
  for ( int i = 0; i < VIDEO_MEM_SIZE; i++ )
    m_frameBuffer[i] = 0;

  if ( !FileBrowser::mountSDCard( false, SD_MOUNT_PATH ) ) return -2;

  int layoutJP = -1;
  for ( int i = 0; i < SupportedLayouts::count(); i++ )
  {
    if ( strcmp( SupportedLayouts::shortNames()[ i ], "JP" ) == 0 )
    {
      layoutJP = i;
      break;
    }
  }
  if ( layoutJP == -1 ) return -3;

  // setup Keyboard (default configuration)
  PS2Preset preset = PS2Preset::KeyboardPort0_MousePort1;
#ifdef NARYA_2_0
  preset = PS2Preset::KeyboardPort0;
#endif
  m_PS2Controller.begin( preset );
  // setup keyboard layout Default: "JP"
  m_keyboard = m_PS2Controller.keyboard();
  m_keyboard->setLayout( SupportedLayouts::layouts()[ layoutJP ] );

  // IO
  if ( !attachIO( 256 ) ) return -4;

  // RAM
  if ( !attachRAM( 65536 ) ) return -5;

  // CMT buffer
  if ( !attachCMT( 65536 ) ) return -6;

  // ROM
  if ( !romload( PC8001ROM ) )
    if ( !romload( PC8801_N80ROM ) )
     return -7;

  // FONT
  if ( !fontload( PC8001FONT ) ) return -8;

  // DISK ROM
  m_diskRomEnable = m_DISK.romLoad( PC80312W_ROM );
  if ( !m_diskRomEnable )
    m_diskRomEnable = m_DISK.romLoad( PC80S31_ROM );
    if ( !m_diskRomEnable )
      m_diskRomEnable = m_DISK.romLoad( DISK_ROM );
  m_diskEnable = m_diskRomEnable;
  if ( m_diskRomEnable && !selDiskRomEnable )
    m_diskEnable = false;

  // VGA
  m_CGA.setVideoBuffer( m_frameBuffer );
  m_CGA.setFont( &m_FontPC8001, &m_FontPC8001Graph, &m_FontPCG );
  m_CGA.setEmulation( packeduPD3301adpter::Emulation::PC_Text_40x20_8Colors );
  m_CGA.enablePCG( m_PCGMode );
  m_CGA.enableVideo( true );

  // Device callbacks
  m_CRT.setCallbacks( this, setScreenMode );
  m_KEYBRD.setCallbacks( this, vkf9, vkf10, vkf11, vkf12 );
  m_PCG.setCallbacks( this, setPCGMode, updatePCGFont );

  // Device reset
  uint8_t * vram = &m_RAM[0xF300];
  bool result[8];
  result[0] = m_BUZZER.reset( &m_soundGenerator );
  result[1] = m_CALENDAR.reset( m_diskEnable, m_printerEnable );
  result[2] = m_CMT.reset( vram, m_cmtBufferSize, m_cmtBuffer );
  result[3] = m_CRT.reset( vram, m_frameBuffer );
  result[4] = m_DISK.reset( vram, m_diskEnable, &m_soundGenerator );
  result[5] = m_KEYBRD.reset( vram, m_keyboard );
  result[6] = m_PCG.reset( &m_FontPC8001, &m_FontPCG, &m_soundGenerator );
  result[7] = m_PRINTER.reset( m_printerEnable );
  if ( !result[0] ) return -9;
  if ( !result[4] ) return -10;
  if ( !result[6] ) return -11;

#ifdef _DEBUG
  // Queue
  m_queueHandle = xQueueCreate( 10, sizeof( msgRequestCpuCmd ) );
  //debugBreak( true, 0 );
  //debugBreak( false, 0x813D );
  //debugBreak( false, 0xC000 );
  //debugBreak( false, 0x6002 );
#endif // _DEBUG

  m_beforeTickTime = micros();

  return 0;
}


void Machine::softReset( void * context )
{
  auto m = (Machine *)context;
  uint8_t * vram = &m->m_RAM[0xF300];
  m->m_portE2writeData = 0;
  m->m_PCGMode         = 0;
  m->m_cmtLoad         = false;
  m->m_n80Load         = false;
  m->m_CGA.enablePCG( m_PCGMode );
  m->m_BUZZER.reset( &m->m_soundGenerator );
  m->m_CALENDAR.reset( m->m_diskEnable, m->m_printerEnable );
  m->m_CMT.reset( vram, m->m_cmtBufferSize, m->m_cmtBuffer );
  m->m_CRT.reset( vram, m->m_frameBuffer );
  m->m_DISK.softReset();
  m->m_KEYBRD.reset( vram, m->m_keyboard );
  m->m_PCG.reset( &m_FontPC8001, &m_FontPCG, &m_soundGenerator );
  m->m_PRINTER.reset( m->m_printerEnable );
  m->setScreenMode( context, 0 );
  m->m_Z80.reset();
  m->m_Z80.setPC( 0 );
}


bool Machine::romload( const char * fileName )
{
  if ( !m_RAM ) return false;
  bool romload = false;
  char path[256];
  strcpy( path, SD_MOUNT_PATH );
  strcat( path, fileName );
  auto fp = fopen( path, "r" );
  if ( !fp ) return false;
  fseek( fp, 0, SEEK_END );
  size_t fileSize = ftell( fp );
  fseek( fp, 0, SEEK_SET );
  if ( fileSize == 24576 || fileSize == 32768 )
  {
    memset( &m_RAM[0], 0xFF, 32768 );
    size_t result = fread( &m_RAM[0], 1, fileSize, fp );
    if ( result == fileSize )
      romload = true;
  }
  fclose( fp );
  if ( romload && fileSize == 24576 )
  {
    strcpy( path, SD_MOUNT_PATH );
    strcat( path, PC8001_USEROM );
    auto fp = fopen( path, "r" );
    if ( fp )
    {
      fseek( fp, 0, SEEK_END );
      size_t fileSize = ftell( fp );
      fseek( fp, 0, SEEK_SET );
      if ( fileSize > 8192 )
        fileSize = 8192;
      fread( &m_RAM[0x6000], 1, fileSize, fp );
      fclose( fp );
    }
  }
  return romload;
}


bool Machine::fontload( const char * fileName )
{
  size_t fontSize = 256 * 8 * 3;
  m_FONT = (uint8_t *)ps_malloc( fontSize );
  if ( !m_FONT ) return false;
  memset( m_FONT, 0, fontSize ); 
  bool fontload = false;
  char path[256];
  strcpy( path, SD_MOUNT_PATH );
  strcat( path, fileName );
  auto fp = fopen( path, "r" );
  if ( !fp ) return false;
  fseek( fp, 0, SEEK_END );
  size_t fileSize = ftell( fp );
  fseek( fp, 0, SEEK_SET );
  if ( fileSize == 2048 || fileSize == 4096 )
  {
    size_t result = fread( &m_FONT[0], 1, fileSize, fp );
    if ( result == fileSize )
      fontload = true;
  }
  fclose( fp );
  if ( !fontload ) return false;
  if ( fileSize == 2048 )
    makeGraphFont();

  m_FontPC8001.pointSize = 6;
  m_FontPC8001.width     = 8;
  m_FontPC8001.height    = 8;
  m_FontPC8001.ascent    = 7;
  m_FontPC8001.inleading = 0;
  m_FontPC8001.exleading = 0;
  m_FontPC8001.flags     = 0;
  m_FontPC8001.weight    = 400;
  m_FontPC8001.charset   = 255;
  m_FontPC8001.data      = &m_FONT[0];
  m_FontPC8001.chptr     = NULL;
  m_FontPC8001.codepage  = 437;

  m_FontPC8001Graph.pointSize = 6;
  m_FontPC8001Graph.width     = 8;
  m_FontPC8001Graph.height    = 8;
  m_FontPC8001Graph.ascent    = 7;
  m_FontPC8001Graph.inleading = 0;
  m_FontPC8001Graph.exleading = 0;
  m_FontPC8001Graph.flags     = 0;
  m_FontPC8001Graph.weight    = 400;
  m_FontPC8001Graph.charset   = 255;
  m_FontPC8001Graph.data      = &m_FONT[2048];
  m_FontPC8001Graph.chptr     = NULL;
  m_FontPC8001Graph.codepage  = 437;

  m_FontPCG.pointSize = 6;
  m_FontPCG.width     = 8;
  m_FontPCG.height    = 8;
  m_FontPCG.ascent    = 7;
  m_FontPCG.inleading = 0;
  m_FontPCG.exleading = 0;
  m_FontPCG.flags     = 0;
  m_FontPCG.weight    = 400;
  m_FontPCG.charset   = 255;
  m_FontPCG.data      = &m_FONT[4096];
  m_FontPCG.chptr     = NULL;
  m_FontPCG.codepage  = 437;

  // copy master font to PCG font
  const uint8_t * src = m_FontPC8001.data;
  uint8_t *       des = (uint8_t *)m_FontPCG.data;
  memcpy( des, src, 2048 );

  // Font definition in PCG FCH
  uint8_t * fontPCG_CodeFC = (uint8_t *)&m_FontPCG.data[0xFC*8];
  fontPCG_CodeFC[0] = 0xBD;
  fontPCG_CodeFC[1] = 0x5E;
  fontPCG_CodeFC[2] = 0xCE;
  fontPCG_CodeFC[3] = 0xB4;
  fontPCG_CodeFC[4] = 0xA5;
  fontPCG_CodeFC[5] = 0x01;
  fontPCG_CodeFC[6] = 0x05;
  fontPCG_CodeFC[7] = 0x8A;

  return true;
}


void Machine::makeGraphFont( void )
{
  for ( int i = 0; i < 16; i++ )
  {
    uint64_t * p = (uint64_t *)&m_FONT[2048+i*8*16];
    uint64_t bitData1 = 0x0F0F;
    uint64_t bitData2 = 0xF0F0;
    uint64_t bitData3 = 0;
    if ( i & 0x01 )
      bitData3 |= bitData1;
    if ( i & 0x02 )
      bitData3 |= ( bitData1 << 16 );
    if ( i & 0x04 )
      bitData3 |= ( bitData1 << 32 );
    if ( i & 0x08 )
      bitData3 |= ( bitData1 << 48 );
    for ( int j = 0; j < 16; j++ )
    {
      uint64_t setData = bitData3;
      if ( j & 0x01 )
        setData |= bitData2;
      if ( j & 0x02 )
        setData |= ( bitData2 << 16 );
      if ( j & 0x04 )
        setData |= ( bitData2 << 32 );
      if ( j & 0x08 )
        setData |= ( bitData2 << 48 );
      p[j] = setData;	
    }
  }
}


bool Machine::attachIO( int IOSize )
{
  m_IoIn = (uint8_t *)ps_malloc( IOSize );
  if ( !m_IoIn ) return false;
  memset( m_IoIn, 0xFF, IOSize );
  m_IoOut = (uint8_t *)ps_malloc( IOSize );
  if ( !m_IoOut ) return false;
  memset( m_IoOut, 0x00, IOSize );
  m_IOSize = IOSize;
  return true;
}


bool Machine::attachRAM( int RAMSize )
{
  m_RAM = (uint8_t *)ps_malloc( RAMSize );
  if ( !m_RAM ) return false;
  m_RAMSize = RAMSize;
  memset( m_RAM, 0x00, RAMSize );
  if ( RAMSize == 65536 )
  {
    for ( int i = 0x8000; i <= 0x9FFF; i += 2 )
    {
      m_RAM[i+0] = 0x00;
      m_RAM[i+1] = 0xFF;
    }
    for ( int i = 0xA000; i <= 0xBFFF; i += 2 )
    {
      m_RAM[i+0] = 0xFF;
      m_RAM[i+1] = 0x00;
    }
    for ( int i = 0xC000; i <= 0xFFFF; i += 128 )
    {
      memset( &m_RAM[i+0],  0xFF, 64 );
      memset( &m_RAM[i+64], 0x00, 64 );
    }
  }
  size_t size = 65536;
  m_fileIOBuf = (uint8_t *)malloc( size );
  if ( !m_fileIOBuf ) return false;
  m_fileIOBufSize = size;
  memset( m_fileIOBuf, 0x00, m_fileIOBufSize );
  size = 32768;
  m_RAMExt = (uint8_t *)ps_malloc( size );
  if ( !m_RAMExt ) return false;
  m_RAMExtSize = size;
  memset( m_RAMExt, 0x00, m_RAMExtSize );
  size = 65536;
  m_n80LoadRam = (uint8_t *)ps_malloc( size );
  if ( !m_n80LoadRam ) return false;
  m_n80LoadRamSize = size;
  memset( m_n80LoadRam, 0x00, m_n80LoadRamSize );
  return true;
}


bool Machine::attachCMT( int CMTBufferSize )
{
  m_cmtBuffer = (uint8_t *)ps_malloc( CMTBufferSize );
  if ( !m_cmtBuffer ) return false;
  m_cmtBufferSize = CMTBufferSize;
  memset( m_cmtBuffer, 0, m_cmtBufferSize );
  return true;
}


void Machine::run( void  )
{
  UBaseType_t uxPriority   = 1;
  const BaseType_t xCoreID = 0;
  xTaskCreatePinnedToCore( &cpuTask, "mainCpuTask", 4096, this, uxPriority, &m_taskHandle, xCoreID );
}


void IRAM_ATTR Machine::cpuTask( void * pvParameters )
{
  auto m = (Machine *)pvParameters;
#ifdef _DEBUG
  msgRequestCpuCmd msg;
  bool enableSelBreadAdrs = false;
  int seqNo = 0;
  uint16_t selBreakAdrs = 0;
#endif // _DEBUG
  m->m_Z80.reset();
  m->m_Z80.setPC( 0 );

  while ( true )
  {
    // time in microseconds
    uint32_t t0 = micros();
    int cycles = 0;

#ifdef _DEBUG
    msg.cmd = cmdNon;
    xQueueReceive( m->m_queueHandle, &msg, 0 );
    switch ( (int)msg.cmd )
    {
      case cmdReset:
        enableSelBreadAdrs = false;
        m->softReset( pvParameters );
        m->registerDump( pvParameters, "reset" );
        seqNo = 0;
        break;
      case cmdAnyBreak:
        m->registerDump( pvParameters, "break" );
        seqNo = 10;
        break;
      case cmdSelBreak:
        enableSelBreadAdrs = true;
        selBreakAdrs = msg.BP;
        break;
      case cmdStep:
        seqNo = 20;
        break;
      case cmdContinue:
        seqNo = 30;
        break;
      case cmdSetPCSP:
        m->m_portE2writeData = 0;
        memcpy( &m->m_RAM[0x8000], &m->m_n80LoadRam[0x8000], m->m_n80LoadSize );
        m->m_Z80.reset();
        m->m_Z80.setPC( msg.PC );
        m->m_Z80.writeRegWord( Z80_SP, msg.SP );
        m->registerDump( pvParameters, "set PCSP" );
        seqNo = 10;
        break;
      case cmdEspRestart:
        esp_restart();
        break;
    }
#else
    if ( m->m_menuCmdReset )
    {
      m->m_menuCmdReset = false;
      m->softReset( pvParameters );
    }
    if ( m->m_n80Load )
    {
      m->m_n80Load = false;
      m->m_portE2writeData = 0;
      memcpy( &m->m_RAM[0x8000], &m->m_n80LoadRam[0x8000], m->m_n80LoadSize );
      m->m_Z80.reset();
      m->m_Z80.setPC( m->m_requestPC );
      m->m_Z80.writeRegWord( Z80_SP, m->m_requestSP );
    }
#endif // _DEBUG

    uint32_t dmacPenarty = m->m_CRT.getDmacPenarty();
    if ( dmacPenarty > 0 && ( m->m_ticksCounter & 3 ) != 3 )
    {
      if ( dmacPenarty > 10 )
      {
        cycles = 10;
        dmacPenarty -= cycles;
      }
      else
      {
        cycles = dmacPenarty;
        dmacPenarty = 0;
      }
      m->m_CRT.setDmacPenarty( dmacPenarty );
    }
    else
    {
#ifdef _DEBUG
      switch ( seqNo )
      {
        case 0:
          if ( enableSelBreadAdrs )
          {
            if ( m->m_Z80.getPC() == selBreakAdrs )
            {
              m->registerDump( pvParameters, "break" );
              seqNo = 10;
              break;
            }
          }
          cycles = m->m_Z80.step();
          break;
        case 10:
          // non
          break;
        case 20:
          cycles = m->m_Z80.step();
          m->registerDump( pvParameters, "step" );
          seqNo = 10;
          break;
        case 30:
          cycles = m->m_Z80.step();
          m->registerDump( pvParameters, "continue" );
          seqNo = 0;
          break;
      }
#else
      cycles = m->m_Z80.step();
#endif // _DEBUG
    }

    if ( m->m_CRT.getCrtcTextVisible() == 0 )
    {
      // time in microseconds
      uint32_t t1 = micros();
      int dt = t1 - t0;
      if ( dt < 0 )
        dt = t1 + ( 0xFFFFFFFF - t0 );

      // at 2MHz each cycle last 0.5us, so instruction time is cycles*0.5, that is cycles/2
      // NEC PC-8001 4MHz
      int t = ( cycles / 4 ) - dt;
      if ( t > 0 )
        delayMicroseconds( t );
    }

    // tick counter up
    m->m_ticksCounter++;
  }
}


void Machine::backGround( void )
{
  uint32_t nowTickTime = micros();
  int dt = nowTickTime - m_beforeTickTime;
  if ( dt < 0 )
    dt = nowTickTime + ( 0xFFFFFFFF - m_beforeTickTime );
  m_beforeTickTime = nowTickTime;

  m_CRT.tick( 10 * 1000 );
  m_KEYBRD.tick();

  if ( m_CMT.getBasCmtSave() )
  {
    m_CMT.resetBasCmtSave();
    char fileName[64];
    memset( fileName, 0, sizeof( fileName ) );
    memcpy( fileName, &m_cmtBuffer[0x0A], 6 );
    strcat( fileName, CMT_FILE_EXTENSION );
    cmtSave( fileName );
  }

  if ( m_CMT.getMonCmtSave() )
  {
    m_CMT.resetMonCmtSave();
    uint16_t startAdrs = m_CMT.getMonCmtSaveStartAdrs();
    uint16_t endAdrs   = m_CMT.getMonCmtSaveEndAdrs();
    char fileName[64];
    sprintf( fileName, "mon%04X-%04X%s", startAdrs, endAdrs, CMT_FILE_EXTENSION );
    cmtSave( fileName );
  }

  if ( m_diskEnable )
  {
    // DISK CPU Task を 1 時間近く Suspend 放置すると Resume 出来なくなった(ｗ)
    // 対策として 30 分周期に Resume するようにした。
    // Suspend 開始判定は  DISK CPU Task 側にて関連 I/O アクセス最終から 5 秒後。
    if ( m_DISK.getTaskSuspended() )
    {
      m_timerDiskCpuSuspend -= dt;
      if ( m_timerDiskCpuSuspend < 0 )
      {
        m_timerDiskCpuSuspend = m_timerDiskCpuSuspendReset;
        m_DISK.taskResume();
      }
    }
    else
      m_timerDiskCpuSuspend = m_timerDiskCpuSuspendReset;
  }

#ifdef _DEBUG
  if ( Serial.available() )
  {
    int recvData = Serial.read();
    m_serialRecvBuf[m_serialRecvIndex] = recvData;
    m_serialRecvIndex++;
    if ( m_serialRecvIndex >= ARRAY_SIZE( m_serialRecvBuf ) )
      m_serialRecvIndex = 0;
    if ( recvData == 0x0A )
      debugCommand();
  }
#endif // _DEBUG
}


int Machine::readByte( void * context, int address )
{
  auto m = (Machine *)context;
  int value = 0xFF;
  if ( address >= 0 && address < 32768 )
  {
    if ( m->m_portE2writeData & 0x01 )
      value = m->m_RAMExt[address];
    else
      value = m->m_RAM[address];
  }
  else
    value = m->m_RAM[address];
  return value;
}


void Machine::writeByte( void * context, int address, int value )
{
  auto m = (Machine *)context;
  if ( address >= 0 && address < 32768 )
  {
    if ( m->m_portE2writeData & 0x10 )
    {
      m->m_RAMExt[address] = value;
    }
#ifdef _ENABLE_USERROMAREA_RAM
    else
    {
      if ( address >= 0x6000 && address <= 0x7FFF )
        m->m_RAM[address] = value;
    }
#endif // _ENABLE_USERROMAREA_RAM
  }
  else
    m->m_RAM[address] = value;
}


int Machine::readIO( void * context, int address )
{
  auto m = (Machine *)context;
  int value = 0xFF;
  int pc = m->m_Z80.getPC() - 1;

  switch ( address )
  {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
#ifdef _KEYBRD_LAYOUT_ALLCHECK
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
#endif // _KEYBRD_LAYOUT_ALLCHECK
      m->m_KEYBRD.readIO( address, &value );
      break;
#ifndef _KEYBRD_LAYOUT_ALLCHECK
    case 0x0A:
    case 0x0B:
      // ignore
      break;
#endif // !_KEYBRD_LAYOUT_ALLCHECK
    case 0x20:
    case 0x21:
      m->m_CMT.readIO( address, &value );
      break;
    case 0x40:
      m->m_CALENDAR.readIO( address, &value );
      m->m_CRT.readIO( address, &value );
      m->m_CALENDAR.setReadIO( address, value );
      break;
    case 0xD1:
    case 0xD8:
    case 0xDA:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] IEEE-488\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xE2:
      value = m->m_portE2writeData & 0x0F;
      break;
    case 0xFC:
    case 0xFE:
      m->m_DISK.pcReadIO( address, &value, pc );
      break;
    default:
      // not handlded!
      _DEBUG_PRINT( "%s(%d):0x%02X [PC 0x%04X] not handlded!\r\n", __func__, __LINE__, address, pc );
      break;
  }

  m->m_IoIn[address] = value;

  return value;
}


void Machine::writeIO( void * context, int address, int value )
{
  auto m = (Machine *)context;
  int pc = m->m_Z80.getPC() - 1;
  m->m_IoOut[address] = value;

  switch ( address )
  {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
      m->m_PCG.writeIO( address, value );
      break;
    case 0x10:
      m->m_CALENDAR.writeIO( address, value );
      m->m_PRINTER.writeIO( address, value );
      break;
    case 0x20:
    case 0x21:
      m->m_CMT.writeIO( address, value );
      break;
    case 0x30:
      m->m_CMT.writeIO( address, value );
      m->m_CRT.writeIO( address, value );
      break;
    case 0x50:
    case 0x51:
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
      m->m_CRT.writeIO( address, value );
      break;
    case 0x31:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] control #2\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0x40:
      m->m_BUZZER.writeIO( address, value );
      m->m_CALENDAR.writeIO( address, value );
      m->m_PRINTER.writeIO( address, value );
      break;
    case 0xC0:
    case 0xC1:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] RS-232C ch#1\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xC2:
    case 0xC3:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] RS-232C ch#2\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
    case 0xCB:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0xCF:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] RS-232C disable\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xD0:
    case 0xD1:
    case 0xD2:
    case 0xD3:
    case 0xDC:
    case 0xDE:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] IEEE-488\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xE0:
    case 0xE1:
    case 0xE3:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] extMemory Mode\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xE2:
      m->m_portE2writeData = value;
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] extMemory Mode\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xE4:  // INT8~15
    case 0xE5:  // INT0~7
    case 0xE6:  // Realtime INT
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] interrupt mask\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xE7:
      // ignore
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] extROM\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xFD:
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] DISK\r\n", __func__, __LINE__, address, value, pc );
      m->m_DISK.pcWriteIO( address, value, pc );
      break;
    case 0xFE:
    case 0xFF:
      m->m_DISK.pcWriteIO( address, value, pc );
      break;
    default:
      // not handlded!
      _DEBUG_PRINT( "%s(%d):0x%02X 0x%02X [PC 0x%04X] not handlded!\r\n", __func__, __LINE__, address, value, pc );
      break;
  }
}


void Machine::setRequestCpuCmd( requestCpuCmd cmd, uint16_t PC, uint16_t SP, uint16_t BP )
{
#ifdef _DEBUG
  msgRequestCpuCmd msg;
  m_menuCmdReset      = false;
  m_menuCmdEspRestart = false;
  msg.cmd = cmd;
  msg.PC  = PC;
  msg.SP  = SP;
  msg.BP  = BP;
  xQueueSend( m_queueHandle, &msg, 0 );
#else
  switch ( (int)cmd )
  {
    case cmdReset:
      m_menuCmdReset = true;
      break;
    case cmdSelBreak:
    case cmdStep:
    case cmdSetPCSP:
      // non
      break;
    case cmdAnyBreak:
      vTaskSuspend( m_taskHandle );
      break;
    case cmdContinue:
      vTaskResume( m_taskHandle );
      break;
    case cmdEspRestart:
      esp_restart();
      break;
  }
#endif // _DEBUG
  m_cmtLoad = false;
}


void Machine::setScreenMode( void * context, int value )
{
  auto m = (Machine *)context;
  packeduPD3301adpter::Emulation disptype = packeduPD3301adpter::Emulation::None;
  switch ( value )
  {
    case 0: // 40x20
      disptype = packeduPD3301adpter::Emulation::PC_Text_40x20_8Colors;
      break;
    case 1: // 40x25
      disptype = packeduPD3301adpter::Emulation::PC_Text_40x25_8Colors;
      break;
    case 2: // 80x20
      disptype = packeduPD3301adpter::Emulation::PC_Text_80x20_8Colors;
      break;
    case 3: // 80x25
      disptype = packeduPD3301adpter::Emulation::PC_Text_80x25_8Colors;
      break;
  }
  if ( disptype == packeduPD3301adpter::Emulation::None ) return;
  m->m_CGA.setEmulation( disptype );
  m->m_CMT.setScreenMode( value );
  m->m_KEYBRD.setScreenMode( value );
  m->m_DISK.setScreenMode( value );
}


void Machine::setPCGMode( void * context, int value )
{
  auto m = (Machine *)context;
  m->m_PCGMode = value;
  m->m_CGA.enablePCG( value );
  _DEBUG_PRINT( "PCG-8100 mode:%s\r\n", ( value == 0 ) ? "OFF" : "ON" );
}


void Machine::updatePCGFont( void * context )
{
  auto m = (Machine *)context;
  m->m_CGA.updatePCGFont( &m->m_FontPCG );
}


void Machine::vkf9( void * context, int value )
{
  auto m = (Machine *)context;
  m->setRequestCpuCmd( cmdAnyBreak );

  memset( m->m_menu2FileName, 0, sizeof( m->m_menu2FileName ) );
  m->m_menuCallback();
  m->m_keyboard->enableVirtualKeys( true, true );

  if ( strlen( m->m_menu2FileName ) > 0 )
  {
    char * p = strrchr( m->m_menu2FileName, '.' );
    if ( p )
    {
      char ext[16];
      memset( ext, 0, sizeof( ext ) );
      strncpy( ext, p, sizeof( ext ) - 1 );
      if ( strcasecmp( ext, CMT_FILE_EXTENSION ) == 0 )
        m->m_cmtLoad = m->cmtLoad( m->m_menu2FileName );
      if ( strcasecmp( ext, N80_FILE_EXTENSION ) == 0 )
        m->m_n80Load = m->n80Load( m->m_menu2FileName );
    }
  }

  m->m_beforeTickTime = micros();
  if ( m->m_menuCmdReset )
    m->setRequestCpuCmd( cmdReset );
  if ( m->m_menuCmdEspRestart )
    m->setRequestCpuCmd( cmdEspRestart );
  if ( m->m_n80Load )
    m->setRequestCpuCmd( cmdSetPCSP, m->m_requestPC, m->m_requestSP, 0 );

  m->setRequestCpuCmd( cmdContinue );
}


void Machine::vkf10( void * context, int value )
{
  auto m = (Machine *)context;
  m->m_PCGMode ^= 1;
  m->setPCGMode( context, m->m_PCGMode );
}


void Machine::vkf11( void * context, int value )
{
  auto m = (Machine *)context;
  m->setRequestCpuCmd( cmdAnyBreak );
  //
  // 専用機ｗ
  // .n80 ファイル生成は t88tool.exe を使うと良いです(何をいまさらｗ)
  // Windows (32bit/64bit) コマンドプロンプト内で動作します。
  // https://bugfire2009.ojaru.jp/download.html#t88tool
  // 
  // .n80 生成例
  // (1) xxx.cmt(マシン語) 実行開始アドレス 8150h
  // c:\t88tool -n -E 8150 -f "xxx.n80" "xxx.cmt" 
  //
  // (2) yyy.cmt (BASIC) と  monyyy.cmt (マシン語) の場合
  // c:\t88tool -n -f "yyy.n80" "yyy.bas" "monyyy.cmt"
  //
  if ( !value )
  {
    // VK_F11 押下
    m->m_n80Load = m->n80Load( "Bug Fire.n80" );
  }
  else
  {
    // SHIFT + VK_F11 押下
    m->m_n80Load = m->n80Load( "Scramble.n80" );
  }
  if ( m->m_n80Load )
    m->setRequestCpuCmd( cmdSetPCSP, m->m_requestPC, m->m_requestSP, 0 );
  m->setRequestCpuCmd( cmdContinue );
}


void Machine::vkf12( void * context, int value )
{
  auto m = (Machine *)context;
  if ( value )
    m->setRequestCpuCmd( cmdReset );
  else
    m->setRequestCpuCmd( cmdEspRestart );
}


bool Machine::setDiskImage( int drive, bool writeProtected, const char * imgFileName )
{
  char path[256];
  if ( drive >= 0 && drive < ARRAY_SIZE( m_fdImgFileName ) )
    memset( m_fdImgFileName[drive], 0, sizeof( m_fdImgFileName[drive] ) );
  m_DISK.eject( drive );
  if ( !( imgFileName && imgFileName[0] != 0 ) ) return true;
  sprintf( path, "%s%s/%s", SD_MOUNT_PATH, PC8001MEDIA_DISK, imgFileName );
  bool result = m_DISK.insert( drive, writeProtected, path );
  if ( !result ) return false;
  if ( drive >= 0 && drive < ARRAY_SIZE( m_fdImgFileName ) )
    strncpy( m_fdImgFileName[drive], imgFileName, sizeof( m_fdImgFileName[drive] ) );
  return true;
}


const char * Machine::getDiskImageFileName( int drive )
{
  const char * fileName = NULL;
  if ( drive >= 0 && drive < ARRAY_SIZE( m_fdImgFileName ) )
    fileName = m_fdImgFileName[drive];
  return fileName;
}


bool Machine::cmtSave( const char * cmtFileName )
{
  bool fileSave = false;
  char path[256];
  sprintf( path, "%s%s/%s", SD_MOUNT_PATH, PC8001MEDIA, cmtFileName );
  auto fp = fopen( path, "w" );
  if ( !fp ) return false;
  size_t fileSize = m_CMT.getCmtBufferSize();
  size_t result   = 0;
  if ( fileSize <= m_fileIOBufSize )
  {
    memcpy( m_fileIOBuf, m_cmtBuffer, fileSize );
    result = fwrite( m_fileIOBuf, 1, fileSize, fp );
  }
  if ( result == fileSize )
    fileSave = true;
  fclose( fp );
  if ( fileSave )
  {
    m_CMT.resetCmtBufferIndex();
    _DEBUG_PRINT( "cmtsave end. \"%s\"\r\n", cmtFileName );
  }
  else
    _DEBUG_PRINT( "cmtsave failed. \"%s\" fileSave(%d) fileSize(%d) result(%d)\r\n",
                  cmtFileName, fileSave, fileSize, result );
  return fileSave;
}


bool Machine::cmtLoad( const char * cmtFileName )
{
  bool fileLoad = false;
  char path[256];
  sprintf( path, "%s%s/%s", SD_MOUNT_PATH, PC8001MEDIA, cmtFileName );
  auto fp = fopen( path, "r" );
  if ( !fp ) return false;
  fseek( fp, 0, SEEK_END );
  size_t fileSize = ftell( fp );
  size_t result   = 0;
  fseek( fp, 0, SEEK_SET );
  if ( fileSize <= m_fileIOBufSize && fileSize <= m_cmtBufferSize )
  {
    result = fread( m_fileIOBuf, 1, fileSize, fp );
    if ( result == fileSize )
    {
      memcpy( m_cmtBuffer, m_fileIOBuf, fileSize );
      fileLoad = true;
    }
  }
  fclose( fp );
  if ( fileLoad )
  {
    m_CMT.setCmtBufferSize( fileSize );
    _DEBUG_PRINT( "cmtload end. \"%s\"\r\n", cmtFileName );
  }
  else
    _DEBUG_PRINT( "cmtload failed. \"%s\" fileLoad(%d) fileSize(%d) result(%d)\r\n",
                  cmtFileName, fileLoad, fileSize, result );
  return fileLoad;
}


bool Machine::n80Load( const char * n80FileName )
{
  m_n80LoadSize = 0;
  bool fileLoad = false;
  char path[256];
  sprintf( path, "%s%s/%s", SD_MOUNT_PATH, PC8001MEDIA, n80FileName );
  auto fp = fopen( path, "r" );
  if ( !fp ) return false;
  fseek( fp, 0, SEEK_END );
  size_t fileSize = ftell( fp );
  size_t result   = 0;
  fseek( fp, 0, SEEK_SET );
  if ( fileSize <= m_fileIOBufSize && fileSize <= m_n80LoadRamSize )
  {
    result = fread( m_fileIOBuf, 1, fileSize, fp );
    if ( result == fileSize )
    {
      memcpy( &m_n80LoadRam[0x8000], m_fileIOBuf, fileSize );
      fileLoad = true;
    }
  }
  fclose( fp );
  if ( fileLoad )
  {
    m_n80LoadSize = fileSize;
    m_requestPC = 0xFF3D;
    memcpy( &m_requestSP, &m_n80LoadRam[0xFF3E], 2 );
    _DEBUG_PRINT( "n80load end. \"%s\"\r\n", n80FileName );
  }
  else
    _DEBUG_PRINT( "n80load failed. \"%s\" fileLoad(%d) fileSize(%d) result(%d)\r\n",
                  n80FileName, fileLoad, fileSize, result );
  return fileLoad;
}


#ifdef _DEBUG
void Machine::debugBreak( bool any, int address )
{
  if ( any )
    setRequestCpuCmd( cmdAnyBreak );
  else
    setRequestCpuCmd( cmdSelBreak, 0, 0, address );
}


void Machine::debugContinue( void )
{
  setRequestCpuCmd( cmdContinue );
}


void Machine::debugStep( void )
{
  setRequestCpuCmd( cmdStep );
}
#endif // _DEBUG


void Machine::registerDump( void * context, const char * msg )
{
  auto m = (Machine *)context;
  uint16_t reg[7];
  uint16_t PC = m->m_Z80.getPC();
  reg[0] = m->m_Z80.readRegWord( Z80_BC );
  reg[1] = m->m_Z80.readRegWord( Z80_DE );
  reg[2] = m->m_Z80.readRegWord( Z80_HL );
  reg[3] = m->m_Z80.readRegWord( Z80_AF );
  reg[4] = m->m_Z80.readRegWord( Z80_IX );
  reg[5] = m->m_Z80.readRegWord( Z80_IY );
  reg[6] = m->m_Z80.readRegWord( Z80_SP );
  uint8_t flag = reg[3] & 0x00FF;
  if ( msg )
    _MSG_PRINT( "\r\n%s\r\n", msg );
  _MSG_PRINT( "MAIN CPU\r\nAF:%04X BC:%04X DE:%04X HL:%04X  S Z Y H X PV N C\r\n", reg[3], reg[0], reg[1], reg[2] );
  _MSG_PRINT( "IX:%04X IY:%04X SP:%04X PC:%04X  %d %d %d %d %d  %d %d %d\r\n",
              reg[4], reg[5], reg[6], PC,
              ( flag & Z80_S_FLAG )  != 0 ? true : false, 
              ( flag & Z80_Z_FLAG )  != 0 ? true : false, 
              ( flag & Z80_Y_FLAG )  != 0 ? true : false, 
              ( flag & Z80_H_FLAG )  != 0 ? true : false, 
              ( flag & Z80_X_FLAG )  != 0 ? true : false, 
              ( flag & Z80_PV_FLAG ) != 0 ? true : false, 
              ( flag & Z80_N_FLAG )  != 0 ? true : false, 
              ( flag & Z80_C_FLAG )  != 0 ? true : false );
}


void Machine::ioDump( int type, int address )
{
  uint8_t * ioBuf = ( type == 0 ) ? &m_IoIn[0] : &m_IoOut[0];
  if ( address < 0 || address >= m_IOSize ) return;
  _MSG_PRINT( "MAIN CPU\r\n%s Adrs %02X : %02X\r\n", ( type == 0 ) ? "IN" : "OUT", address, ioBuf[address] );
}


void Machine::memoryDump( int address, int size )
{
  if ( address < 0 || address >= m_RAMSize || ( address + size ) > m_RAMSize ) return;
  _MSG_PRINT( "MAIN CPU\r\nADRS +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F 0123456789ABCDEF\r\n" );
  for ( int i = address; i < ( address + size ); i += 16 )
  {
    char hexbuf[128];
    char ascbuf[64];
    sprintf( hexbuf, "%04X ", i );
    memset( ascbuf, 0, sizeof( ascbuf ) );
    for ( int j = 0; j < 16; j++ )
    {
      uint8_t data = 0xFF;
      int adrs = i + j;
      if ( adrs >= 0 && adrs < 32768 )
      {
        if ( m_portE2writeData & 0x01 )
          data = m_RAMExt[i+j];
        else
          data = m_RAM[i+j];
      }
      else
        data = m_RAM[i+j];
      char work[16];
      sprintf( work, "%02X ", data );
      strcat( hexbuf, work );
      if ( data >= 0x20 && data < 0x7F )
        ascbuf[j] = data;
      else
        ascbuf[j] = '.';
    }
    _MSG_PRINT( "%s%s\r\n", hexbuf, ascbuf );
  }
}


void Machine::memoryLoad( int loadAdrs, const char * fileName )
{
  char path[256];
  bool loadEnd = false;
  sprintf( path, "%s%s/%s", SD_MOUNT_PATH, PC8001MEDIA, fileName );
  size_t loadSize = 0;
  auto fp = fopen( path, "r" );
  if ( fp )
  {
    fseek( fp, 0, SEEK_END );
    loadSize = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    if ( ( loadAdrs + loadSize ) > m_fileIOBufSize )
      loadSize = m_fileIOBufSize - loadAdrs;
    size_t result = fread( &m_fileIOBuf[loadAdrs], 1, loadSize, fp );
    if ( result == loadSize )
    {
      for ( int adrs = loadAdrs; adrs < ( loadAdrs + loadSize ); adrs++ )
      {
        uint8_t data = m_fileIOBuf[adrs];
        if ( adrs >= 0 && adrs < 32768 )
        {
          if ( m_portE2writeData & 0x10 )
            m_RAMExt[adrs] = data;
          else
            m_RAM[adrs] = data;
        }
        else
          m_RAM[adrs] = data;
      }
      loadEnd = true;
    }
    fclose( fp );
  }
  if ( loadEnd )
    _MSG_PRINT( "%s -> %04X~%04X load end.\r\n", path, loadAdrs, ( loadAdrs + loadSize - 1 ) );
  else
    _MSG_PRINT( "%s load failed.\r\n", path );
}


void Machine::memorySave( int startAdrs, int endAdrs )
{
  char path[256];
  bool saveEnd = false;
  sprintf( path, "%s%s/%04X-%04X.bin", SD_MOUNT_PATH, PC8001MEDIA, startAdrs, endAdrs );
  auto fp = fopen( path, "w" );
  if ( fp )
  {
    size_t saveSize = endAdrs - startAdrs + 1;
    uint8_t data;
    for ( int adrs = startAdrs; adrs <= endAdrs; adrs++ )
    {
      if ( adrs >= 0 && adrs < 32768 )
      {
        if ( m_portE2writeData & 0x01 )
          data = m_RAMExt[adrs];
        else
          data = m_RAM[adrs];
      }
      else
        data = m_RAM[adrs];
      m_fileIOBuf[adrs] = data;
    }
    size_t result = fwrite( &m_fileIOBuf[startAdrs], 1, saveSize, fp );
    if ( result == saveSize )
      saveEnd = true;
    fclose( fp );
  }
  if ( saveEnd )
    _MSG_PRINT( "%04X~%04X -> %s save end.\r\n", startAdrs, endAdrs, path );
  else
    _MSG_PRINT( "%s save failed.\r\n", path );
}


#ifdef _DEBUG
void Machine::debugHelp( void )
{
  _DEBUG_PRINT( "\r\nhelp for debug.\r\n" );
  _DEBUG_PRINT( "b[adrs] : breakpoint setting\r\n" );
  _DEBUG_PRINT( "t       : step excute\r\n" );
  _DEBUG_PRINT( "c       : continue excute\r\n" );
  _DEBUG_PRINT( "r       : dump for CPU register\r\n" );
  _DEBUG_PRINT( "d[adrs] : memory dump for 256 bytes\r\n" );
  _DEBUG_PRINT( "i[adrs] : I/O dump for 1byte input port\r\n" );
  _DEBUG_PRINT( "o[adrs] : I/O dump for 1byte output port\r\n" );
  _DEBUG_PRINT( "mlA,F   : Loads a file to the specified address\r\n" );
  _DEBUG_PRINT( "      A : load a address\r\n" );
  _DEBUG_PRINT( "      F : file name\r\n" );
  _DEBUG_PRINT( "        : ex. ml8000,TEST001.bin   <- %s/TEST001.bin\r\n", PC8001MEDIA );
  _DEBUG_PRINT( "        : ex. mlde00,DE00-DF3B.bin <- %s/DE00-DF3F.bin\r\n", PC8001MEDIA );
  _DEBUG_PRINT( "msS,E   : saves a specified range of memory\r\n" );
  _DEBUG_PRINT( "      S : save a start address\r\n" );
  _DEBUG_PRINT( "      E : save a end address\r\n" );
  _DEBUG_PRINT( "        : ex. ms0000,02ff -> %s/0000-02FF.bin\r\n", PC8001MEDIA );
  _DEBUG_PRINT( "        : ex. msde00,df3f -> %s/DE00-DF3F.bin\r\n", PC8001MEDIA );
  if ( !m_diskEnable ) return;
  _DEBUG_PRINT( "DISK CPU\r\n" );
  _DEBUG_PRINT( "B[adrs] : breakpoint setting\r\n" );
  _DEBUG_PRINT( "T       : step excute\r\n" );
  _DEBUG_PRINT( "C       : continue excute\r\n" );
  _DEBUG_PRINT( "R       : dump for CPU register\r\n" );
  _DEBUG_PRINT( "D[adrs] : memory dump for 256 bytes\r\n" );
  _DEBUG_PRINT( "I[adrs] : I/O dump for 1byte input port\r\n" );
  _DEBUG_PRINT( "O[adrs] : I/O dump for 1byte output port\r\n" );
}


void Machine::debugCommand( void )
{
  char buf[16];
  char * p = NULL;
  int adrs;
  bool any = false;
  p = strchr( m_serialRecvBuf, 0x0D );
  if ( p ) p[0] = 0x00;
  p = strchr( m_serialRecvBuf, 0x0A );
  if ( p ) p[0] = 0x00;
  _DEBUG_PRINT( "\r\n%s\r\n", m_serialRecvBuf );

  switch ( m_serialRecvBuf[0] )
  {
    case 'h':
      debugHelp();
      break;
    case 'b':
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      buf[2] = m_serialRecvBuf[3];
      buf[3] = m_serialRecvBuf[4];
      if ( strlen( buf ) > 0 )
      {
        any = false;
        sscanf( buf, "%x", &adrs );
      }
      else
        any = true;
      debugBreak( any, adrs );
      break;
    case 't':
      debugStep();
      break;
    case 'c':
      debugContinue();
      break;
    case 'r':
      registerDump( this );
      break;
    case 'd':
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      buf[2] = m_serialRecvBuf[3];
      buf[3] = m_serialRecvBuf[4];
      if ( strlen( buf ) > 0 )
        sscanf( buf, "%x", &adrs );
      else
        adrs = m_lastDumpMemoryAdrs;
      memoryDump( adrs, 256 );
      m_lastDumpMemoryAdrs = adrs + 256;
      if ( m_lastDumpMemoryAdrs >= m_RAMSize )
        m_lastDumpMemoryAdrs = 0;
      break;
    case 'i':
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      if ( strlen( buf ) > 0 )
        sscanf( buf, "%x", &adrs );
      else
        adrs = m_lastDumpIoInAdrs;
      ioDump( 0, adrs );
      m_lastDumpIoInAdrs = adrs + 1;
      if ( m_lastDumpIoInAdrs >= m_IOSize )
        m_lastDumpIoInAdrs = 0;
      break;
    case 'o':
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      if ( strlen( buf ) > 0 )
        sscanf( buf, "%x", &adrs );
      else
        adrs = m_lastDumpIoOutAdrs;
      ioDump( 1, adrs );
      m_lastDumpIoOutAdrs = adrs + 1;
      if ( m_lastDumpIoOutAdrs >= m_IOSize )
        m_lastDumpIoOutAdrs = 0;
      break;
    case 'm':
      if ( m_serialRecvBuf[1] == 'l' && m_serialRecvBuf[6] == ',' )
      {
        int loadAdrs = -1;
        char fileName[256];
        memset( buf, 0, sizeof( buf ) );
        buf[0] = m_serialRecvBuf[2];
        buf[1] = m_serialRecvBuf[3];
        buf[2] = m_serialRecvBuf[4];
        buf[3] = m_serialRecvBuf[5];
        if ( strlen( buf ) > 0 )
          sscanf( buf, "%x", &loadAdrs );
        memset( fileName, 0, sizeof( fileName ) );
        strcpy( fileName, &m_serialRecvBuf[7] );
        if ( loadAdrs != -1 && fileName[0] != 0 )
          memoryLoad( loadAdrs, fileName );
      }
      if ( m_serialRecvBuf[1] == 's' && m_serialRecvBuf[6] == ',' )
      {
        int startAdrs = -1;
        int endAdrs = -1;
        memset( buf, 0, sizeof( buf ) );
        buf[0] = m_serialRecvBuf[2];
        buf[1] = m_serialRecvBuf[3];
        buf[2] = m_serialRecvBuf[4];
        buf[3] = m_serialRecvBuf[5];
        if ( strlen( buf ) > 0 )
          sscanf( buf, "%x", &startAdrs );
        memset( buf, 0, sizeof( buf ) );
        buf[0] = m_serialRecvBuf[7];
        buf[1] = m_serialRecvBuf[8];
        buf[2] = m_serialRecvBuf[9];
        buf[3] = m_serialRecvBuf[10];
        if ( strlen( buf ) > 0 )
          sscanf( buf, "%x", &endAdrs );
        if ( startAdrs != -1 && endAdrs != -1 && startAdrs <= endAdrs )
          memorySave( startAdrs, endAdrs );
      }
      break;
    case 'B':
      if ( !m_diskEnable ) break;
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      buf[2] = m_serialRecvBuf[3];
      buf[3] = m_serialRecvBuf[4];
      if ( strlen( buf ) > 0 )
      {
        any = false;
        sscanf( buf, "%x", &adrs );
      }
      else
        any = true;
      m_DISK.debugBreak( any, adrs );
      break;
    case 'T':
      if ( !m_diskEnable ) break;
      m_DISK.debugStep();
      break;
    case 'C':
      if ( !m_diskEnable ) break;
      m_DISK.debugContinue();
      break;
    case 'R':
      if ( !m_diskEnable ) break;
      m_DISK.registerDump( &m_DISK );
      break;
    case 'D':
      if ( !m_diskEnable ) break;
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      buf[2] = m_serialRecvBuf[3];
      buf[3] = m_serialRecvBuf[4];
      if ( strlen( buf ) > 0 )
        sscanf( buf, "%x", &adrs );
      else
        adrs = m_lastDumpDiskMemoryAdrs;
      m_DISK.memoryDump( adrs, 256 );
      m_lastDumpDiskMemoryAdrs = adrs + 256;
      if ( m_lastDumpDiskMemoryAdrs >= m_DISK.getRAMSize() )
        m_lastDumpDiskMemoryAdrs = 0;
      break;
    case 'I':
      if ( !m_diskEnable ) break;
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      if ( strlen( buf ) > 0 )
        sscanf( buf, "%x", &adrs );
      else
        adrs = m_lastDumpDiskIoInAdrs;
      m_DISK.ioDump( 0, adrs );
      m_lastDumpDiskIoInAdrs = adrs + 1;
      if ( m_lastDumpDiskIoInAdrs >= m_DISK.getIOSize() )
        m_lastDumpDiskIoInAdrs = 0;
      break;
    case 'O':
      if ( !m_diskEnable ) break;
      memset( buf, 0, sizeof( buf ) );
      buf[0] = m_serialRecvBuf[1];
      buf[1] = m_serialRecvBuf[2];
      if ( strlen( buf ) > 0 )
        sscanf( buf, "%x", &adrs );
      else
        adrs = m_lastDumpDiskIoOutAdrs;
      m_DISK.ioDump( 1, adrs );
      m_lastDumpDiskIoOutAdrs = adrs + 1;
      if ( m_lastDumpDiskIoOutAdrs >= m_DISK.getIOSize() )
        m_lastDumpDiskIoOutAdrs = 0;
      break;
  }

  m_serialRecvIndex = 0;
  memset( m_serialRecvBuf, 0, sizeof( m_serialRecvBuf ) );
}


void _DEBUG_PRINT( const char * format, ... )
{
  va_list ap;
  va_start( ap, format );
  int size = vsnprintf( NULL, 0, format, ap ) + 1;
  if ( size > 0 )
  {
    va_end( ap );
    va_start( ap, format );
    char buf[size + 1];
    vsnprintf( buf, size, format, ap );
    Serial.printf( "%s", buf );
  }
  va_end( ap );
}
#endif // _DEBUG


#ifdef _UPD765A_DEBUG
void _UPD765A_DEBUG_PRINT( const char * format, ... )
{
  va_list ap;
  va_start( ap, format );
  int size = vsnprintf( NULL, 0, format, ap ) + 1;
  if ( size > 0 )
  {
    va_end( ap );
    va_start( ap, format );
    char buf[size + 1];
    vsnprintf( buf, size, format, ap );
    Serial.printf( "%s", buf );
  }
  va_end( ap );
}
#endif // _UPD765A_DEBUG


void _MSG_PRINT( const char * format, ... )
{
  va_list ap;
  va_start( ap, format );
  int size = vsnprintf( NULL, 0, format, ap ) + 1;
  if ( size > 0 )
  {
    va_end( ap );
    va_start( ap, format );
    char buf[size + 1];
    vsnprintf( buf, size, format, ap );
    Serial.printf( "%s", buf );
  }
  va_end( ap );
}
