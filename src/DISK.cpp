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


#include "DISK.h"


DISK::DISK()
{
  m_RAM         = NULL;
  m_RAMSize     = 0;
  m_IoIn        = NULL;
  m_IoOut       = NULL;
  m_IOSize      = 0;
  m_diskEnable  = false;
  m_2kRomLoaded = false;

  m_Z80.setCallbacks( this, readByte, writeByte, readWord, writeWord, readIO, writeIO );
}


DISK::~DISK()
{
}


bool DISK::romLoad( const char * fileName )
{
  size_t IOSize = 256;
  if ( !m_IoIn )
  {
    m_IoIn = (uint8_t *)ps_malloc( IOSize );
    if ( !m_IoIn ) return false;
    memset( m_IoIn, 0x00, IOSize );
  }
  if ( !m_IoOut )
  {
    m_IoOut = (uint8_t *)ps_malloc( IOSize );
    if ( !m_IoOut ) return false;
    memset( m_IoOut, 0x00, IOSize );
    m_IOSize = IOSize;
  }

  size_t size = 32768;
  if ( !m_RAM )
  {
    m_RAM = (uint8_t *)ps_malloc( size );
    if ( !m_RAM ) return false;
    memset( m_RAM, 0, size );
    for ( int i = 0x4000; i <= 0x7FFF; i += 128 )
    {
      memset( &m_RAM[i+0],  0xFF, 64 );
      memset( &m_RAM[i+64], 0x00, 64 );
    }
    m_RAMSize = size;
    // used : 16 + 256 bytes
    m_d88ReadWriteBuffer = &m_RAM[0x2000];
  }

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
  if ( fileSize == 2048 || fileSize == 8192 )
  {
    size_t result = fread( &m_RAM[0], 1, fileSize, fp );
    if ( result == 2048 )
      if ( m_RAM[0x07EF] == 0xEF )
      {
        m_2kRomLoaded = true;
        romload = true;
      }
    if ( result == 8192 )
      if ( m_RAM[0x07EE] == 0xFE && m_RAM[0x07EF] == 0xEF )
        romload = true;
  }
  fclose( fp );
  return romload;
}


bool DISK::insert( int drive, bool writeProtected, const char * imgFileName )
{
  if ( !m_diskEnable ) return false;
  if ( drive < 0 || drive >= ARRAY_SIZE( m_media ) ) return false;
  if ( !m_2kRomLoaded && drive >= 2 ) return false;
  bool result = m_media[drive].insert( writeProtected, imgFileName );
  return result;
}


void DISK::eject( int drive )
{
  if ( !m_diskEnable ) return;
  if ( drive < 0 || drive >= ARRAY_SIZE( m_media ) ) return;
  m_media[drive].eject();
}


void DISK::setScreenMode( int value )
{
  switch ( value )
  {
    case 0: // 40x20
    case 1: // 40x25
      m_statusAdrs = 39 * 2;
      break;
    case 2: // 80x20
    case 3: // 80x25
      m_statusAdrs = 79;
      break;
  }
}


bool DISK::reset( uint8_t * vram, bool diskEnable, SoundGenerator * soundGenerator )
{
  m_diskEnable = diskEnable;
  if ( !m_diskEnable ) return true;

  m_disk2Pc8255PortC = 0;
  m_pc2Disk8255PortC = 0;
  m_requestIRQ       = false;
  m_vram             = vram;
  m_mutexIoIn        = portMUX_INITIALIZER_UNLOCKED;
  m_mutexIoOut       = portMUX_INITIALIZER_UNLOCKED;

  m_indicatorSeqNo     = 0;
  m_indicatorNextSeqNo = 0;
  m_requestTaskSuspend = false;
  m_taskSuspended      = false;
  m_timerSuspendReset  = 5 * 1000 * 1000;
  m_timerSuspend       = m_timerSuspendReset;

  setScreenMode( 0 );

  bool result = m_uPD765A.reset( (bool *)&m_requestIRQ,
                                 &m_media[0],
                                 m_d88ReadWriteBuffer,
                                 soundGenerator );
  if ( !result ) return false;

#ifdef _DEBUG
  // Queue
  m_queueHandle = xQueueCreate( 10, sizeof( msgRequestCpuCmd ) );
  //debugBreak( false, 0x010E );
  //debugBreak( false, 0x048F );
#endif // _DEBUG

  m_beforeTickTime = micros();

  UBaseType_t uxPriority   = 1;
  const BaseType_t xCoreID = 1;
  xTaskCreatePinnedToCore( &cpuTask, "diskCpuTask", 4096, this, uxPriority, &m_taskHandle, xCoreID );
  return true;
}


void IRAM_ATTR DISK::cpuTask( void * pvParameters )
{
  auto m = (DISK *)pvParameters;
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

    m->taskCheckSuspend( m );

#ifdef _DEBUG
    msg.cmd = cmdNon;
    xQueueReceive( m->m_queueHandle, &msg, 0 );
    switch ( (int)msg.cmd )
    {
      case cmdReset:
        enableSelBreadAdrs = false;
        m->m_Z80.reset();
        m->m_Z80.setPC( 0 );
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
    }
    if ( !m->m_taskSuspended )
    {
      if ( m->m_Z80.getStatus() == Z80_STATUS_HALT )
      {
        if ( m->m_requestIRQ )
        {
          if ( m->m_Z80.getIM() == Z80_INTERRUPT_MODE_0 && m->m_Z80.getIFF1() )
          {
            // NOP : 00H
            cycles = m->m_Z80.IRQ( 0x00 );
            if ( cycles > 0 )
              m->m_requestIRQ = false;
          }
        }
      }
      else
      {
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
      }
    }
#else
    if ( m->m_Z80.getStatus() == Z80_STATUS_HALT )
    {
      if ( m->m_requestIRQ )
      {
        if ( m->m_Z80.getIM() == Z80_INTERRUPT_MODE_0 && m->m_Z80.getIFF1() )
        {
          // NOP : 00H
          cycles = m->m_Z80.IRQ( 0x00 );
          if ( cycles > 0 )
            m->m_requestIRQ = false;
        }
      }
    }
    else
      cycles = m->m_Z80.step();
#endif // _DEBUG

    m->m_uPD765A.tick();
    m->taskRequestSuspend( m );

    // time in microseconds
    uint32_t t1 = micros();
    int dt = t1 - t0;
    if ( dt < 0 )
      dt = t1 + ( 0xFFFFFFFF - t0 );

    // at 2MHz each cycle last 0.5us, so instruction time is cycles*0.5, that is cycles/2
    // NEC PC-8031-2W/PC-80S31 4MHz
    int t = ( cycles / 4 ) - dt;
    if ( t > 0 )
      delayMicroseconds( t );

    // tick counter up
    m->m_ticksCounter++;
  }
}


int DISK::readByte( void * context, int address )
{
  auto m = (DISK *)context;
  int romLimitAdrs;
  int value = 0xFF;
  if ( m->m_2kRomLoaded )
    romLimitAdrs = 0x07FF;
  else
    romLimitAdrs = 0x1FFF;
  if ( ( address >= 0x0000 && address <= romLimitAdrs )
    || ( address >= 0x4000 && address <= 0x7FFF ) )
    value = m->m_RAM[address];
  return value;
}


void DISK::writeByte( void * context, int address, int value )
{
  auto m = (DISK *)context;
  if ( address >= 0x4000 && address <= 0x7FFF )
    m->m_RAM[address] = value;
}


int DISK::readIO( void * context, int address )
{
  auto m = (DISK *)context;
#ifdef _DEBUG
  int pc = m->m_Z80.getPC() - 1;
#endif // _DEBUG
  int value = 0xFF;

  switch ( address )
  {
    case 0xF8:
      // FDC TC signal
      m->m_uPD765A.write_signal( SIG_UPD765A_TC );
      break;
    case 0xFA:
    case 0xFB:
      // FDC
      value = m->m_uPD765A.readIO( address );
      m->m_IoIn[address] = value;
      break;
    case 0xFC:
      // 8255 Port A
      portENTER_CRITICAL( &m->m_mutexIoIn );
      value = m->m_IoIn[address];
      portEXIT_CRITICAL( &m->m_mutexIoIn );
      break;
    case 0xFE:
      // 8255 Port C
      portENTER_CRITICAL( &m->m_mutexIoIn );
      value = m->m_IoIn[address];
      portEXIT_CRITICAL( &m->m_mutexIoIn );
      break;
    case 0xFF:
      // 8255 Control Port
      portENTER_CRITICAL( &m->m_mutexIoIn );
      m->m_IoIn[address] = m->m_IoOut[address];
      value = m->m_IoIn[address];
      portEXIT_CRITICAL( &m->m_mutexIoIn );
      break;
    default:
      // not handlded!
      _DEBUG_PRINT( "DISK %s(%d):0x%02X [PC 0x%04X] not handlded!\r\n", __func__, __LINE__, address, pc );
      break;
  }

  return value;
}


void DISK::writeIO( void * context, int address, int value )
{
  auto m = (DISK *)context;
#ifdef _DEBUG
  int pc = m->m_Z80.getPC() - 1;
#endif // _DEBUG

  switch ( address )
  {
    case 0xF4:
      // ignore : drive mode
      m->m_IoOut[address] = value;
      _DEBUG_PRINT( "DISK %s(%d):0x%02X 0x%02X [PC 0x%04X] drive mode\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xF7:
      // ignore : write pre compensation
      m->m_IoOut[address] = value;
      _DEBUG_PRINT( "DISK %s(%d):0x%02X 0x%02X [PC 0x%04X] write pre compensation\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xF8:
      // ignore : drive motor
      m->m_IoOut[address] = value;
      _DEBUG_PRINT( "DISK %s(%d):0x%02X 0x%02X [PC 0x%04X] drive motor\r\n", __func__, __LINE__, address, value, pc );
      break;
    case 0xFB:
      // FDC
      m->m_IoOut[address] = value;
      m->m_uPD765A.writeIO( address, value );
      break;
    case 0xFD:
      // 8255 Port B
      portENTER_CRITICAL( &m->m_mutexIoOut );
      m->m_IoOut[address] = value;
      portEXIT_CRITICAL( &m->m_mutexIoOut );
      break;
    case 0xFF:
      // 8255 Control Port
      m->m_IoOut[address] = value;
      if ( !( value & 0x80) )
      {
        // BIT SET/RESET
        int data = m->m_disk2Pc8255PortC;
        int bitcount = ( value >> 1 ) & 0x07;
        int bitdata  = 1 << bitcount;
        if ( value & 0x01 )
          data |= bitdata;
        else
          data &= ~bitdata;
        m->m_disk2Pc8255PortC = data;
        data &= 0xF0;
        data >>= 4;
        portENTER_CRITICAL( &m->m_mutexIoOut );
        m->m_IoOut[0xFE] = data;
        portEXIT_CRITICAL( &m->m_mutexIoOut );
      }
      else
        m->m_disk2Pc8255PortC = 0;
      break;
    default:
      // not handlded!
      _DEBUG_PRINT( "DISK %s(%d):0x%02X 0x%02X [PC 0x%04X] not handlded!\r\n", __func__, __LINE__, address, value, pc );
      break;
  }
}


bool DISK::pcReadIO( int address, int * result, int pc )
{
  if ( address == 0xFC )
  {
    // 8255 Port A : I/O read DISK 8255 Port B
    portENTER_CRITICAL( &m_mutexIoOut );
    *result = m_IoOut[0xFD];
    portEXIT_CRITICAL( &m_mutexIoOut );
    updateDiskAccessIndicator();
    return true;
  }
  if ( address == 0xFE )
  {
    // 8255 Port C : I/O read DISK 8255 Port C
    portENTER_CRITICAL( &m_mutexIoOut );
    *result = m_IoOut[0xFE];
    portEXIT_CRITICAL( &m_mutexIoOut );
    updateDiskAccessIndicator();
    return true;
  }
  return false;
}


bool DISK::pcWriteIO( int address, int value, int pc )
{
  if ( address == 0xFD )
  {
    // 8255 Port B : I/O write DISK 8255 Port A
    portENTER_CRITICAL( &m_mutexIoIn );
    m_IoIn[0xFC] = value;
    portEXIT_CRITICAL( &m_mutexIoIn );
    updateDiskAccessIndicator();
    return true;
  }
  if ( address == 0xFE )
  {
    // 8255 Port C : I/O write DISK 8255 Port C
    int data = value;
    data &= 0xF0;
    m_pc2Disk8255PortC = data;
    data >>= 4;
    portENTER_CRITICAL( &m_mutexIoIn );
    m_IoIn[0xFE] = data;
    portEXIT_CRITICAL( &m_mutexIoIn );
    taskCheckResume( m_pc2Disk8255PortC );
    updateDiskAccessIndicator();
    return true;
  }
  if ( address == 0xFF )
  {
    // 8255 Control Port
    if ( !( value & 0x80) )
    {
      // BIT SET/RESET
      int data = m_pc2Disk8255PortC;
      int bitcount = ( value >> 1 ) & 0x07;
      int bitdata  = 1 << bitcount;
      if ( value & 0x01 )
        data |= bitdata;
      else
        data &= ~bitdata;
      m_pc2Disk8255PortC = data;
      data &= 0xF0;
      data >>= 4;
      // 8255 Port C : I/O write DISK 8255 Port C
      portENTER_CRITICAL( &m_mutexIoIn );
      m_IoIn[0xFE] = data;
      portEXIT_CRITICAL( &m_mutexIoIn );
      taskCheckResume( m_pc2Disk8255PortC );
    }
    else
      m_pc2Disk8255PortC = 0;
    updateDiskAccessIndicator();
    return true;
  }
  return false;
}


void DISK::taskCheckSuspend( DISK * m )
{
  if ( m->m_requestTaskSuspend || m->m_taskSuspended )
  {
#ifndef DONT_DISKCPU_SUSPEND
    m->m_requestTaskSuspend = false;
    m->m_taskSuspended      = true;
    m->updateDiskAccessIndicator( true );
  #ifdef _DEBUG
    vTaskDelay( 100 / portTICK_PERIOD_MS );
  #else
    vTaskSuspend( m->m_taskHandle );
  #endif // _DEBUG
#else
    m->m_requestTaskSuspend = false;
    m->updateDiskAccessIndicator( true );
#endif // !DONT_DISKCPU_SUSPEND
  }
}


void DISK::taskRequestSuspend( DISK * m )
{
  uint32_t nowTickTime = micros();
  int dt = nowTickTime - m->m_beforeTickTime;
  if ( dt < 0 )
    dt = nowTickTime + ( 0xFFFFFFFF - m->m_beforeTickTime );
  m->m_beforeTickTime = nowTickTime;

  m->m_timerSuspend -= dt;
  if ( m->m_timerSuspend < 0 )
  {
    m->m_timerSuspend = m->m_timerSuspendReset;
    m->m_requestTaskSuspend = true;
  }
}


void DISK::taskCheckResume( int value )
{
#ifndef DONT_DISKCPU_SUSPEND
  if ( ( value & 0x80 ) && m_taskSuspended )
  {
    m_beforeTickTime = micros();
    m_taskSuspended = false;
  #ifndef _DEBUG
    vTaskResume( m_taskHandle );
  #endif // !_DEBUG
  }
#endif  // !DONT_DISKCPU_SUSPEND
}


void DISK::updateDiskAccessIndicator( bool clear )
{
#ifndef DONT_DISK_ACCESS_INDICATOR
  switch ( m_indicatorSeqNo )
  {
    case 0:
      if ( m_IoOut[0xF8] != 0x00 )
      {
        //                      012345678901234567
        m_indicatorMessage   = "fdd motor spin up ";
        m_indicatorNextSeqNo = 10;
        m_indicatorSeqNo     = 110;
      }
      break;
    case 10:
      if ( m_IoOut[0xF8] == 0xFF )
      {
        //                      012345678901234567
        m_indicatorMessage   = "fdd motor rdy wait";
        m_indicatorNextSeqNo = 20;
        m_indicatorSeqNo     = 110;
      }
      break;
    case 20:
      if ( m_IoOut[0xF8] != 0xFF || m_RAM[0x7F15] == 0x0F )
      {
        //                      012345678901234567
        m_indicatorMessage   = "                  ";
        m_indicatorNextSeqNo = 100;
        m_indicatorSeqNo     = 110;
      }
      break;
    case 100:
      // non
      break;
    case 110:
      {
        int adrs = m_statusAdrs / 2;
        int len = strlen( m_indicatorMessage );
        adrs -= len;
        adrs *= 2;
        for ( int i = 0; i < len; i++ )
          m_vram[ adrs + ( 2 * i ) ] = m_indicatorMessage[i];
      }
      m_indicatorSeqNo = m_indicatorNextSeqNo;
      break;
  }
  uint8_t code = m_vram[ m_statusAdrs ];
  switch ( code )
  {
    case 0x95:
      code = 0xEF;
      break;
    case 0xEF:
      code = 0x96;
      break;
    case 0x96:
      code = 0xEE;
      break;
    case 0xEE:
      code = 0x95;
      break;
    default:
      code = 0x95;
      break;
  }
  if ( clear )
    code = 0;
  m_vram[ m_statusAdrs ] = code;
#endif // !DONT_DISK_ACCESS_INDICATOR

  m_timerSuspend = m_timerSuspendReset;
}


#ifdef _DEBUG
void DISK::setRequestCpuCmd( requestCpuCmd cmd, uint16_t BP )
{
  msgRequestCpuCmd msg;
  msg.cmd = cmd;
  msg.BP  = BP;
  xQueueSend( m_queueHandle, &msg, 0 );
}


void DISK::debugBreak( bool any, int address )
{
  if ( any )
    setRequestCpuCmd( cmdAnyBreak );
  else
    setRequestCpuCmd( cmdSelBreak, address );
}


void DISK::debugContinue( void )
{
  setRequestCpuCmd( cmdContinue );
}


void DISK::debugStep( void )
{
  setRequestCpuCmd( cmdStep );
}
#endif // _DEBUG


void DISK::registerDump( void * context, const char * msg )
{
  auto m = (DISK *)context;
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
  _MSG_PRINT( "DISK CPU:Suspend(%d) IM(%d) IFF1(%d) IFF2(%d)\r\n",
              m->m_taskSuspended,
              m->m_Z80.getIM(),
              m->m_Z80.getIFF1(),
              m->m_Z80.getIFF2() );
  _MSG_PRINT( "AF:%04X BC:%04X DE:%04X HL:%04X  S Z Y H X PV N C\r\n", reg[3], reg[0], reg[1], reg[2] );
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


void DISK::memoryDump( int address, int size )
{
  if ( address < 0 || address >= m_RAMSize || ( address + size ) > m_RAMSize ) return;
  _MSG_PRINT( "DISK CPU\r\nADRS +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F 0123456789ABCDEF\r\n" );
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
      if ( adrs >= 0 && adrs < m_RAMSize )
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


void DISK::ioDump( int type, int address )
{
  uint8_t * ioBuf = ( type == 0 ) ? &m_IoIn[0] : &m_IoOut[0];
  if ( address < 0 || address >= m_IOSize ) return;
  _MSG_PRINT( "DISK CPU\r\n%s Adrs %02X : %02X\r\n", ( type == 0 ) ? "IN" : "OUT", address, ioBuf[address] );
}

