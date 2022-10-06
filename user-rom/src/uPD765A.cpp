/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  This program is based on upd765a.cpp in xm8_170.zip
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

#include "uPD765A.h"
#include "sampling_diskdata.h"


uPD765A::uPD765A()
{
  m_bufferSize  = 0;
  m_bufferCount = 0;
  m_bufferPtr   = NULL;
  size_t size   = 4096;
  m_buffer = (uint8_t *)ps_malloc( size );
  if ( m_buffer )
  {
    memset( m_buffer, 0, size );
    m_bufferPtr  = m_buffer;
    m_bufferSize = size;
  }

  m_requestIRQ     = NULL;
  m_requestIRQ     = NULL;
  m_media          = NULL;
  m_soundGenerator = NULL;
  m_bgmSeek        = NULL;
}


uPD765A::~uPD765A()
{
}


bool uPD765A::reset( bool * requestIRQ, media * disk, uint8_t * d88rwBuffer, SoundGenerator * soundGenerator )
{
  if ( !m_buffer ) return false;
  m_requestIRQ      = requestIRQ;
  m_media           = disk;
  m_d88rwBuffer     = d88rwBuffer;
  m_sectorBuffer    = &m_d88rwBuffer[16];
  m_sectorSize      = 256;
  m_command         = 0;
  m_phase           = PHASE_IDLE;
  m_eventPhase      = (PHASE)-1;
  m_eventPhaseTimer = 0;
  m_eventID         = (EVENT)-1;
  m_bgmSeekTimer    = 0;
  m_mainStatus      = S_RQM;
  m_stepRateTime    = 0;
  m_noDmaMode       = true;
  m_forceReady      = false;
  memset( m_fdd, 0, sizeof( m_fdd ) );
  memset( m_eventTimer, 0, sizeof( m_eventTimer ) );
  set_hdu( 0 );

  if ( !m_soundGenerator )
  {
    m_soundGenerator = soundGenerator;
#ifndef DONT_SEEK_SOUND
    m_bgmSeek = m_soundGenerator->playSamples( (int8_t *)&_seekTrackAnyData[0], sizeof( _seekTrackAnyData ), 100, -1 );
    if ( !m_bgmSeek )
      return false;
    m_bgmSeek->enable( false );
#endif // !DONT_SEEK_SOUND
  }

  for ( int i = 0; i < MAX_DRIVE; i++ )
  {
    if ( !m_media[i].setSectorBuffer( m_d88rwBuffer ) )
      return false;
  }

  return true;
}


int uPD765A::readIO( int address )
{
  int A0 = address & 0x01;
  int value = 0xFF;

  if ( A0 == 1 )
  {
    if ( ( m_mainStatus & ( S_RQM | S_DIO ) ) == ( S_RQM | S_DIO ) )
    {
      m_mainStatus &= ~S_RQM;
      switch ( (int)m_phase )
      {
        case PHASE_RESULT:
          value = *m_bufferPtr++;
          //_UPD765A_DEBUG_PRINT( "uPD765A %s(%d):0x%02X PHASE_RESULT(0x%02X)\r\n", __func__, __LINE__, address, value );
          if ( --m_bufferCount )
            m_mainStatus |= S_RQM;
          else
          {
            bool clearIRQ = true;
            if ( ( m_command & 0x1F ) == 0x08 )
            {
              for ( int i = 0; i < ARRAY_SIZE( m_fdd ); i++ )
              {
                if ( m_fdd[i].result )
                {
                  clearIRQ = false;
                  break;
                }
              }
            }
            if ( clearIRQ )
              set_irq( false );
            shift_to_idle();
          }
          break;
        case PHASE_READ:
          value = *m_bufferPtr++;
          //_UPD765A_DEBUG_PRINT( "uPD765A %s(%d):0x%02X PHASE_READ(0x%02X)\r\n", __func__, __LINE__, address, value );
          if ( --m_bufferCount )
          {
            if ( m_noDmaMode )
              m_mainStatus |= S_RQM;
            set_irq( true );
          }
          else
            process_cmd( m_command & 0x1F );
          m_fdd[m_hdu & DRIVE_MASK].access = true;
          break;
      }
    }
  }
  else
  {
    value = m_mainStatus;
    //_UPD765A_DEBUG_PRINT( "uPD765A %s(%d):0x%02X 0x%02X\r\n", __func__, __LINE__, address, value );
  }

  return value;
}


void uPD765A::writeIO( int address, int value )
{
  int A0 = address & 0x01;
  if ( A0 == 0 ) return;

  if ( ( m_mainStatus & ( S_RQM | S_DIO) ) == S_RQM )
  {
    m_mainStatus &= ~S_RQM;
    switch ( (int)m_phase )
    {
      case PHASE_IDLE:
        m_command = value;
        process_cmd( m_command & 0x1F );
        break;
      case PHASE_CMD:
        *m_bufferPtr++ = value;
        if ( --m_bufferCount )
          m_mainStatus |= S_RQM;
        else
          process_cmd( m_command & 0x1F );
        break;
      case PHASE_WRITE:
        //_UPD765A_DEBUG_PRINT( "uPD765A %s(%d):0x%02X PHASE_WRITE(0x%02X)\r\n", __func__, __LINE__, address, value );
        *m_bufferPtr++ = value;
        if ( --m_bufferCount )
        {
          if ( m_noDmaMode )
          {
            m_mainStatus |= S_RQM;
            set_irq( true );
          }
        }
        else
          process_cmd( m_command & 0x1F );
        m_fdd[m_hdu & DRIVE_MASK].access = true;
        break;
      // Not implement
      //case PHASE_SCAN:
      //  break;
    }
  }
}


void uPD765A::write_signal( int id )
{
  switch ( id )
  {
    case SIG_UPD765A_TC:
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) phase(%d)\r\n", __func__, __LINE__, (int)m_phase );
      if ( m_phase == PHASE_READ
        || m_phase == PHASE_WRITE
        || m_phase == PHASE_SCAN
        || ( m_phase == PHASE_RESULT && m_bufferCount == 7 ) )
      {
        m_prevphase = m_phase;
				m_phase     = PHASE_TC;
        process_cmd( m_command & 0x1F );
      }
      break;
  }
}


void uPD765A::tick( void )
{
  switch ( (int)m_eventPhase )
  {
    case PHASE_EXEC:
    case PHASE_TIMER:
      if ( checkTimer( m_eventPhaseTimer ) )
      {
        m_phase      = m_eventPhase;
        m_eventPhase = (PHASE)-1;
        m_eventPhaseTimer = 0;
        process_cmd( m_command & 0x1F );
      }
      break;
  }
  int eventID = (int)m_eventID;
  switch ( eventID )
  {
    case EVENT_SEEK_DRV0:
    case EVENT_SEEK_DRV1:
    case EVENT_SEEK_DRV2:
    case EVENT_SEEK_DRV3:
      if ( checkTimer( m_eventTimer[eventID] ) )
      {
        int drv = eventID - (int)EVENT_SEEK_DRV0;
        m_eventID = (EVENT)-1;
        m_eventTimer[eventID] = 0;
    		seek_event( drv );
      }
      break;
  }
#ifndef DONT_SEEK_SOUND
  if ( checkTimer( m_bgmSeekTimer ) )
    bgmSeekEnd();
#endif // !DONT_SEEK_SOUND
}


void uPD765A::set_irq( bool enableIRQ )
{
  if ( m_noDmaMode )
    *m_requestIRQ = enableIRQ;
}


void uPD765A::process_cmd( int cmd )
{
  switch ( cmd )
  {
    // Not implement
    //case 0x02:  // COMMAND : Read Diagnostic
    //  cmd_read_diagnostic();
    //  break;
    case 0x03:  // COMMAND : Specify
      cmd_specify();
      break;
    case 0x04:  // COMMAND : Sense Drive Status
      cmd_sense_drive_status();
      break;
    case 0x05:  // COMMAND : Write Data
    case 0x09:  // COMMAND : Write Deleted Data
      cmd_write_data();
      break;
    case 0x06:  // COMMAND : Read Data
    case 0x0C:  // COMMAND : Read Deleted Data
      cmd_read_data();
      break;
    case 0x07:  // COMMAND : Recalibrate
      cmd_recalibrate();
      break;
    case 0x08:  // COMMAND : Sense Interrupt Status
      cmd_sense_interrupt_status();
      break;
    // Not implement
    //case 0x0A:  // COMMAND : Read ID
    //  cmd_read_id();
    //  break;
    case 0x0D:  // COMMAND : Write ID [Format Write]
      cmd_write_id();
      break;
    case 0x0F:  // COMMAND : Seek
      cmd_seek();
      break;
    // Not implement
    //case 0x11:  // COMMAND : Scan Equal
    //case 0x19:  // COMMAND : Scan Low or Equal
    //case 0x1D:  // COMMAND : Scan High or Equal
    //  cmd_scan();
    //  break;
    default:
      // not handlded!
      cmd_invalid();
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) command(0x%02X)\r\n", __func__, __LINE__, cmd );
      break;
  }
}


void uPD765A::cmd_read_diagnostic( void )
{
  // Not implement
}


void uPD765A::cmd_specify( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 2 );
      break;
    case PHASE_CMD:
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) 0x%02X 0x%02X\r\n", __func__, __LINE__, m_buffer[0], m_buffer[1] );
      m_stepRateTime = ( m_buffer[0] & 0xF0 ) >> 4;
      m_noDmaMode = ( ( m_buffer[1] & 0x01 ) != 0);
      shift_to_idle();
      break;
  }
}


void uPD765A::cmd_sense_drive_status( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 1 );
      break;
    case PHASE_CMD:
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) 0x%02X\r\n", __func__, __LINE__, m_buffer[0] );
      set_hdu( m_buffer[0] );
      m_buffer[0] = get_devstat( m_buffer[0] & DRIVE_MASK );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) 0x%02X\r\n", __func__, __LINE__, m_buffer[0] );
      shift_to_result( 1 );
      set_irq( true );
      break;
  }
}


void uPD765A::cmd_write_data( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 8 );
      break;
    case PHASE_CMD:
      get_sector_params();
      REGISTER_PHASE_EVENT_NEW( PHASE_EXEC, get_usec_to_exec_phase() );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) hdu            (%d)\r\n", __func__, __LINE__, m_hdu );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) hdue           (%d)\r\n", __func__, __LINE__, m_hdue );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[0] cylinder (%d)\r\n", __func__, __LINE__, m_id[0] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[1] head     (%d)\r\n", __func__, __LINE__, m_id[1] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[2] record   (%d)\r\n", __func__, __LINE__, m_id[2] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[3] number   (%d)\r\n", __func__, __LINE__, m_id[3] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) eot            (%d)\r\n", __func__, __LINE__, m_eot );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) gpl            (%d)\r\n", __func__, __LINE__, m_gpl );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) dtl            (%d)\r\n", __func__, __LINE__, m_dtl );
      break;
    case PHASE_EXEC:
      m_result = check_cond( true );
      if ( m_result & ST1_MA )
      {
        // retry
        REGISTER_PHASE_EVENT( PHASE_EXEC, 1000000 );
        break;
      }
      if ( !m_result )
        m_result = find_id();
      if ( m_result )
        shift_to_result7();
      else
      {
        int length = 0x80 << ( m_id[3] & 0x07 );
        if ( !( m_id[3] & 0x07 ) )
        {
          length = _min( m_dtl, 0x80 );
          memset( m_buffer + length, 0, 0x80 - length );
        }
        _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) length(%d)\r\n", __func__, __LINE__, length );
        shift_to_write( length );
      }
      break;
    case PHASE_WRITE:
      write_data( ( m_command & 0x1F ) == 0x09 );
      if ( m_result )
      {
        shift_to_result7();
        break;
      }
      if ( !id_incr() )
      {
        REGISTER_PHASE_EVENT( PHASE_TIMER, 2000 );
        break;
      }
      REGISTER_PHASE_EVENT_NEW( PHASE_EXEC, get_usec_to_exec_phase() );
      break;
    case PHASE_TC:
      CANCEL_EVENT();
      shift_to_result7();
      break;
    case PHASE_TIMER:
      m_result = ST1_EN;
      shift_to_result7();
      break;
  }
}


void uPD765A::cmd_read_data( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 8 );
      break;
    case PHASE_CMD:
      get_sector_params();
      REGISTER_PHASE_EVENT_NEW( PHASE_EXEC, get_usec_to_exec_phase() );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) hdu            (%d)\r\n", __func__, __LINE__, m_hdu );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) hdue           (%d)\r\n", __func__, __LINE__, m_hdue );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[0] cylinder (%d)\r\n", __func__, __LINE__, m_id[0] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[1] head     (%d)\r\n", __func__, __LINE__, m_id[1] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[2] record   (%d)\r\n", __func__, __LINE__, m_id[2] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[3] number   (%d)\r\n", __func__, __LINE__, m_id[3] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) eot            (%d)\r\n", __func__, __LINE__, m_eot );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) gpl            (%d)\r\n", __func__, __LINE__, m_gpl );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) dtl            (%d)\r\n", __func__, __LINE__, m_dtl );
      break;
    case PHASE_EXEC:
      read_data( ( m_command & 0x1F ) == 0x0C, false );
      break;
    case PHASE_READ:
      if ( m_result )
      {
        shift_to_result7();
        break;
      }
      if ( !id_incr() )
      {
        REGISTER_PHASE_EVENT( PHASE_TIMER, 2000 );
        break;
      }
      REGISTER_PHASE_EVENT_NEW( PHASE_EXEC, get_usec_to_exec_phase() );
      break;
    case PHASE_TC:
      CANCEL_EVENT();
      shift_to_result7();
      break;
    case PHASE_TIMER:
      m_result = ST1_EN;
      shift_to_result7();
      break;
  }
}


void uPD765A::cmd_recalibrate( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 1 );
      break;
    case PHASE_CMD:
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) 0x%02X\r\n", __func__, __LINE__, m_buffer[0] );
  		seek( m_buffer[0] & DRIVE_MASK, 0 );
      shift_to_idle();
      break;
  }
}


void uPD765A::cmd_sense_interrupt_status( void )
{
  for ( int i = 0; i < ARRAY_SIZE( m_fdd ); i++ )
  {
    if ( m_fdd[i].result )
    {
      m_buffer[0] = m_fdd[i].result;
      m_buffer[1] = m_fdd[i].track;
      m_fdd[i].result = 0;
      shift_to_result( 2 );
      set_irq( true );
      return;
    }
  }
  m_buffer[0] = ST0_IC;
  shift_to_result( 1 );
  set_irq( true );
}


void uPD765A::cmd_read_id( void )
{
  // Not implement
}


void uPD765A::cmd_write_id( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 5 );
      break;
    case PHASE_CMD:
      set_hdu( m_buffer[0] );
      m_hdue  = m_buffer[0];
      m_id[3] = m_buffer[1];
      m_sc    = m_buffer[2];
      m_gpl   = m_buffer[3];
      m_fill  = m_buffer[4];
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) hdu             (%d)\r\n", __func__, __LINE__, m_hdu );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) hdue            (%d)\r\n", __func__, __LINE__, m_hdue );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) id[3] number    (%d)\r\n", __func__, __LINE__, m_id[3] );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) sc sector/track (%d)\r\n", __func__, __LINE__, m_sc );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) gpl             (%d)\r\n", __func__, __LINE__, m_gpl );
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) d fill byte     (%d)\r\n", __func__, __LINE__, m_fill );
      if ( !m_sc )
      {
        REGISTER_PHASE_EVENT( PHASE_TIMER, 1000000 );
        break;
      }
      shift_to_write( 4 * m_sc );
      break;
    case PHASE_WRITE:
      m_result = write_id();
      shift_to_result7();
      break;
    case PHASE_TC:
      CANCEL_EVENT();
      shift_to_result7();
      break;
    case PHASE_TIMER:
      m_result = ST1_EN;
      shift_to_result7();
      break;
  }
}


void uPD765A::cmd_seek( void )
{
  switch ( (int)m_phase )
  {
    case PHASE_IDLE:
      shift_to_cmd( 2 );
      break;
    case PHASE_CMD:
      _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) 0x%02X 0x%02X\r\n", __func__, __LINE__, m_buffer[0], m_buffer[1] );
  		seek( m_buffer[0] & DRIVE_MASK, m_buffer[1] );
      shift_to_idle();
      break;
  }
}


void uPD765A::cmd_scan( void )
{
  // Not implement
}


void uPD765A::cmd_invalid( void )
{
  m_buffer[0] = (uint8_t)ST0_IC;
	shift_to_result( 1 );
  set_irq( true );
}


void uPD765A::seek( int drv, int trk )
{
  // get distance
  int seektime = 32 - 2 * m_stepRateTime;

  if ( drv >= MAX_DRIVE )
  {
    // invalid drive number
    m_fdd[drv].result = ( drv & DRIVE_MASK ) | ST0_SE | ST0_NR | ST0_AT;
    seek_event( drv );
  }
  else
  {
    // usec
    seektime = ( trk == m_fdd[drv].track ) ? 120 : seektime * abs( trk - m_fdd[drv].track ) + 500;
    m_fdd[drv].track = trk;
    _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) drv(%d) trk(%d) seektime(%d)\r\n", __func__, __LINE__, drv, trk, seektime );
#ifndef DONT_SEEK_SOUND
    bgmSeekBegin();
#endif // !DONT_SEEK_SOUND
#ifdef UPD765A_DONT_WAIT_SEEK
    seek_event( drv );
#else
    EVENT id = (EVENT)( EVENT_SEEK_DRV0 + drv );
    REGISTER_EVENT( id, seektime );
    m_mainStatus |= ( 1 << drv );
#endif // UPD765A_DONT_WAIT_SEEK
  }
}


void uPD765A::seek_event( int drv )
{
  if ( drv >= MAX_DRIVE )
  {
    m_fdd[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
    _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) drv(%d) seek result(0x%02X)\r\n", __func__, __LINE__, drv, m_fdd[drv].result );
  }
  else if ( m_forceReady || m_media[drv].getInserted() )
  {
    m_fdd[drv].result = (drv & DRIVE_MASK) | ST0_SE;
    _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) drv(%d) seek result(0x%02X)\r\n", __func__, __LINE__, drv, m_fdd[drv].result );
  }
  else
  {
    m_fdd[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
    _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) drv(%d) seek result(0x%02X)\r\n", __func__, __LINE__, drv, m_fdd[drv].result );
  }
  m_mainStatus &= ~( 1 << drv );
  set_irq( true );
}


void uPD765A::read_data( bool deleted, bool scan )
{
  m_result = check_cond( false );
  if ( m_result & ST1_MA )
  {
    REGISTER_PHASE_EVENT( PHASE_EXEC, 10000 );
    return;
  }
  if ( m_result )
  {
    shift_to_result7();
    return;
  }
  m_result = read_sector();
  if ( deleted )
    m_result ^= ST2_CM;
  if ( ( m_result & ~ST2_CM ) && !( m_result & ST2_DD ) )
  {
    shift_to_result7();
    return;
  }
  if ( ( m_result & ST2_CM ) && ( m_command & 0x20 ) )
  {
    REGISTER_PHASE_EVENT( PHASE_TIMER, 100000 );
    return;
  }
  int length = ( m_id[3] & 0x07 ) ? ( 0x80 << ( m_id[3] & 0x07 ) ) : ( _min( m_dtl, 0x80 ) );
  if ( !scan )
    shift_to_read( length );
  else
    shift_to_scan( length );
}


void uPD765A::write_data( bool deleted )
{
  if ( ( m_result = check_cond( true ) ) != 0 )
  {
    shift_to_result7();
    return;
	}
  m_result = write_sector( deleted );
}


uint32_t uPD765A::read_sector( void )
{
  int drv = m_hdu & DRIVE_MASK;
  int cylinder = m_fdd[drv].track;
  int head = ( m_hdu >> 2 ) & 1;
  int sectorNo = m_id[2];
  int sectorLen = m_id[3];
  if ( !m_media[drv].readSector( cylinder, head, sectorNo, sectorLen ) )
    return ST0_AT | ST1_MA;
  if ( m_media[drv].getCrcError() )
    return ST0_AT | ST1_DE | ST2_DD;
  if ( m_media[drv].getDeleted() )
    return ST2_CM;
  uint8_t clr   = ( m_media[drv].getDriveMfm() ? 0x4E : 0xFF );
  size_t size   = m_sectorSize;
  uint8_t * src = m_sectorBuffer;
  uint8_t * des = m_buffer;
  memset( des, clr, size );
  memcpy( des, src, size );
  return 0;
}


uint32_t uPD765A::write_sector( bool deleted )
{
  int drv = m_hdu & DRIVE_MASK;
  int cylinder = m_fdd[drv].track;
  int head = ( m_hdu >> 2 ) & 1;
  int sectorNo = m_id[2];
  int sectorLen = m_id[3];
  if ( m_media[drv].getWriteProtected() )
    return ST0_AT | ST1_NW;
  if ( !m_media[drv].readSector( cylinder, head, sectorNo, sectorLen ) )
    return ST0_AT | ST1_MA;
  size_t size = 0x80 << ( m_id[3] & 0x07 );
  size = _min( size, m_sectorSize );
  uint8_t * src = m_buffer;
  uint8_t * des = m_sectorBuffer;
  memcpy( des, src, size );
  m_media[drv].setDeleted( deleted );
  if ( !m_media[drv].writeSector( cylinder, head, sectorNo, sectorLen ) )
    return ST0_AT | ST1_MA;
  return 0;
}


uint8_t uPD765A::get_devstat( int drv )
{
  uint8_t result = drv;
  if ( drv >= MAX_DRIVE )
    result |= ST3_FT;
  else
    if ( m_media[drv].getInserted() )
      result |= ST3_RY
             | ST3_TS
             | ( m_fdd[drv].track                 ? 0 : ST3_T0 )
             | ( ( m_fdd[drv].track & 1 )         ? ST3_HD : 0 )
             | ( m_media[drv].getWriteProtected() ? ST3_WP : 0 );
  return result;
}


void uPD765A::set_hdu( uint8_t value )
{
  m_hdu = value;
}


uint32_t uPD765A::check_cond( bool write )
{
  int drv = m_hdu & DRIVE_MASK;
  m_hdue = m_hdu;
  if ( drv >= MAX_DRIVE )
    return ST0_AT | ST0_NR;
  if ( !m_media[drv].getInserted() )
    return ST0_AT | ST1_MA;
  return 0;
}


uint32_t uPD765A::find_id( void )
{
  return 0;
}


uint32_t uPD765A::write_id( void )
{
  int drv = m_hdu & DRIVE_MASK;
  int cylinder = m_fdd[drv].track;
  int head = ( m_hdu >> 2 ) & 1;
  int length = 0x80 << ( m_id[3] & 0x07 );
  uint32_t result = 0;
  if ( ( m_result = check_cond( true ) ) != 0 )
    return m_result;
  if ( m_media[drv].getWriteProtected() )
    return ST0_AT | ST1_NW;
  if ( !m_media[drv].formatTrackBegin( cylinder, head, m_sc ) )
    return ST0_AT | ST1_MA;
  for ( int i = 0; i < m_eot && i < 256; i++ )
  {
    for ( int j = 0; j < 4; j++ )
      m_id[j] = m_buffer[ 4 * i + j ];
    _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) c(%d) h(%d) r(%2d) n(%d) fill(0x%02X) length(%d)\r\n",
                          __func__, __LINE__, m_id[0], m_id[1], m_id[2], m_id[3], m_fill, length );
    if ( !m_media[drv].formatSector( m_id[0], m_id[1], m_id[2], m_id[3], m_fill, length ) )
    {
      result = ST0_AT | ST1_MA;
      break;
    }
  }
  m_media[drv].formatTrackEnd();
  _UPD765A_DEBUG_PRINT( "uPD765A %s(%d) result(0x%08X)\r\n", __func__, __LINE__, result );
  return result;
}


void uPD765A::get_sector_params( void )
{
  set_hdu( m_buffer[0] );
  m_hdue  = m_buffer[0];
  m_id[0] = m_buffer[1];
  m_id[1] = m_buffer[2];
  m_id[2] = m_buffer[3];
  m_id[3] = m_buffer[4];
  m_eot   = m_buffer[5];
  m_gpl   = m_buffer[6];
  m_dtl   = m_buffer[7];
}


void uPD765A::shift_to_idle( void )
{
  m_phase      = PHASE_IDLE;
  m_mainStatus = S_RQM;
}


void uPD765A::shift_to_cmd( int bufferCount )
{
  m_phase       = PHASE_CMD;
  m_mainStatus  = S_RQM | S_CB;
  m_bufferPtr   = m_buffer;
  m_bufferCount = bufferCount;
}


void uPD765A::shift_to_read( int bufferCount )
{
  m_phase       = PHASE_READ;
  m_mainStatus  = S_RQM | S_DIO | S_NDM | S_CB;
  m_bufferPtr   = m_buffer;
  m_bufferCount = bufferCount;
  set_irq( true );
}


void uPD765A::shift_to_write( int bufferCount )
{
  m_phase       = PHASE_WRITE;
  m_mainStatus  = S_RQM | S_NDM | S_CB;
  m_bufferPtr   = m_buffer;
  m_bufferCount = bufferCount;
  set_irq( true );
}


void uPD765A::shift_to_scan( int bufferCount )
{
  m_phase       = PHASE_SCAN;
  m_mainStatus  = S_RQM | S_NDM | S_CB;
  m_result      = ST2_SH;
  m_bufferPtr   = m_buffer;
  m_bufferCount = bufferCount;
  set_irq( true );
}


void uPD765A::shift_to_result( int bufferCount )
{
  m_phase       = PHASE_RESULT;
  m_mainStatus  = S_RQM | S_CB | S_DIO;
  m_bufferPtr   = m_buffer;
  m_bufferCount = bufferCount;
}


void uPD765A::shift_to_result7( void )
{
  m_buffer[0] = ( m_result & 0xF8 ) | ( m_hdue & 0x07 );
  m_buffer[1] = ( m_result >>  8 ) & 0xFF;
  m_buffer[2] = ( m_result >> 16 ) & 0xFF;
  m_buffer[3] = m_id[0];
  m_buffer[4] = m_id[1];
  m_buffer[5] = m_id[2];
  m_buffer[6] = m_id[3];
	shift_to_result( 7 );
  set_irq( true );
#ifndef DONT_SEEK_SOUND
  bgmSeekEnd();
#endif // !DONT_SEEK_SOUND
}


bool uPD765A::id_incr( void )
{
  if ( ( m_command & 0x13 ) == 0x11 )
  {
    // scan equal
    if ( ( m_dtl & 0xFF ) == 0x02 )
    {
      m_id[2]++;
    }
  }
  if ( m_id[2]++ != m_eot )
  {
    return true;
  }
  m_id[2] = 1;
  if ( m_command & 0x80 )
  {
    set_hdu( m_hdu ^ 4 );
    m_id[1] ^= 1;
    if ( m_id[1] & 1 )
    {
      return true;
    }
  }
  m_id[0]++;
  return false;
}


int uPD765A::get_usec_to_exec_phase( void )
{
  return 500;
}


void uPD765A::bgmSeekBegin( void )
{
  m_bgmSeek->enable( true );
  m_bgmSeekTimer = micros() + 100 * 1000;
}


void uPD765A::bgmSeekEnd( void )
{
  m_bgmSeekTimer = 0;
  m_bgmSeek->enable( false );
}

