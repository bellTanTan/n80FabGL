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

#include "media.h"


media::media()
{
  m_d88Type        = false;
  m_writeProtected = false;
  m_inserted       = false;
  m_mediaType      = MEDIA_TYPE_UNKNOWN;
  m_driveMfm       = false;
  m_fileSize       = 0;
  m_sectorSize     = 0;
  m_deleted        = false;
  m_crcError       = false;
  m_driveMfm       = false;

  m_d88FileHeader = NULL;
  m_d88rwBuffer   = NULL;
  m_sectorBuffer  = NULL;
  m_formatfp      = NULL;

  memset( m_imgFileName,  0, sizeof( m_imgFileName ) );
}


media::~media()
{
}


bool media::setSectorBuffer( uint8_t * d88rwBuffer )
{
  m_d88rwBuffer  = d88rwBuffer;
  m_sectorBuffer = &m_d88rwBuffer[16];
  size_t size = sizeof( *m_d88FileHeader );
  m_d88FileHeader = (PD88_FILE_HEADER)ps_malloc( size );
  if ( !m_d88FileHeader ) return false;
  memset( m_d88FileHeader, 0, size );
  return true;
}


bool media::insert( bool writeProtected, const char * imgFileName )
{
  m_d88Type        = false;
  m_writeProtected = false;
  m_inserted       = false;
  m_mediaType      = MEDIA_TYPE_UNKNOWN;
  m_driveMfm       = false;
  m_fileSize       = 0;
  m_sectorSize     = 0;
  m_deleted        = false;
  m_crcError       = false;
  m_driveMfm       = false;
  if ( !imgFileName ) return false;
  memset( m_d88FileHeader, 0, sizeof( *m_d88FileHeader ) );
  char * p = strrchr( imgFileName, '.' );
  if ( !p ) return false;
  if ( strcasecmp( p, D88_FILE_EXTENSION ) != 0 ) return false;
  auto fp = fopen( imgFileName, "r" );
  if ( !fp ) return false;
  fseek( fp, 0, SEEK_END );
  size_t fileSize = ftell( fp );
  fseek( fp, 0, SEEK_SET );
  size_t result = fread( m_d88FileHeader, 1, sizeof( *m_d88FileHeader ), fp );
  fclose( fp );
  if ( result != sizeof( *m_d88FileHeader ) ) return false;
  if ( m_d88FileHeader->diskType != MEDIA_TYPE_2D ) return false;
  if ( m_d88FileHeader->diskSize != 348848 ) return false;
  if ( fileSize != 348848 ) return false;
  memset( m_imgFileName, 0, sizeof( m_imgFileName ) );
  strncpy( m_imgFileName, imgFileName, sizeof( m_imgFileName ) );
  if ( m_d88FileHeader->writeProtect )
    m_writeProtected = true;
  else
    m_writeProtected = writeProtected;
  m_d88Type    = true;
  m_inserted   = true;
  m_ejected    = false;
  m_mediaType  = MEDIA_TYPE_2D;
  m_driveMfm   = true;
  m_fileSize   = fileSize;
  m_sectorSize = 256;
  m_deleted    = false;
  m_crcError   = false;
  return true;
}


void media::eject( void )
{
  m_d88Type        = false;
  m_inserted       = false;
  m_ejected        = true;
  m_writeProtected = false;
  m_mediaType      = MEDIA_TYPE_UNKNOWN;
  m_driveMfm       = false;
  m_fileSize       = 0;
  m_sectorSize     = 0;
  m_deleted        = false;
  m_crcError       = false;
  memset( m_imgFileName, 0, sizeof( m_imgFileName ) );
}


bool media::readSector( int cylinder, int head, int sectorNo, int sectorLen )
{
  if ( !( m_d88Type && m_inserted && m_mediaType == MEDIA_TYPE_2D && sectorLen == 1 ) ) return false;
  if ( !( cylinder >= 0 && cylinder <= 39 ) ) return false;
  if ( !( head >= 0 && head <= 1 ) )          return false;
  if ( !( sectorNo >= 1 && sectorNo <= 16 ) ) return false;
  int d88SectorSize = 16 + m_sectorSize;
  int logicalTrack = ( cylinder * 2 ) + head;
  int seekPos = sizeof( D88_FILE_HEADER )
              + ( d88SectorSize * 16 * logicalTrack )
              + ( d88SectorSize * ( sectorNo - 1 ) );
  int sectorPos = m_d88FileHeader->trackTable[cylinder]
                + ( d88SectorSize * 16 * cylinder )
                + ( d88SectorSize * 16 * head )
                + ( d88SectorSize * ( sectorNo - 1 ) );
  if ( seekPos != sectorPos ) return false;
  if ( seekPos > m_fileSize ) return false;
  auto fp = fopen( m_imgFileName, "r" );
  if ( !fp ) return false;
  bool sectorRead = false;
  fseek( fp, seekPos, SEEK_SET );
  size_t result = fread( &m_d88rwBuffer[0], 1, d88SectorSize, fp );
  if ( result == d88SectorSize )
    sectorRead = true;
  else
    m_crcError = true;
  fclose( fp );
  return sectorRead;
}


bool media::writeSector( int cylinder, int head, int sectorNo, int sectorLen )
{
  if ( !( m_d88Type && m_inserted && m_mediaType == MEDIA_TYPE_2D && sectorLen == 1 ) ) return false;
  if ( !( cylinder >= 0 && cylinder <= 39 ) ) return false;
  if ( !( head >= 0 && head <= 1 ) )          return false;
  if ( !( sectorNo >= 1 && sectorNo <= 16 ) ) return false;
  int d88SectorSize = 16 + m_sectorSize;
  int logicalTrack = ( cylinder * 2 ) + head;
  int seekPos = sizeof( D88_FILE_HEADER )
              + ( d88SectorSize * 16 * logicalTrack )
              + ( d88SectorSize * ( sectorNo - 1 ) );
  int sectorPos = m_d88FileHeader->trackTable[cylinder]
                + ( d88SectorSize * 16 * cylinder )
                + ( d88SectorSize * 16 * head )
                + ( d88SectorSize * ( sectorNo - 1 ) );
  if ( seekPos != sectorPos ) return false;
  if ( seekPos > m_fileSize ) return false;
  auto fp = fopen( m_imgFileName, "r+" );
  if ( !fp ) return false;
  bool sectorWrite = false;
  fseek( fp, seekPos, SEEK_SET );
  size_t result = fwrite( &m_d88rwBuffer[0], 1, d88SectorSize, fp );
  if ( result == d88SectorSize )
    sectorWrite = true;
  else
    m_crcError = true;
  fclose( fp );
  return sectorWrite;
}


bool media::formatTrackBegin( int cylinder, int head, int sectorNoMax )
{
  if ( !( m_d88Type && m_inserted && m_mediaType == MEDIA_TYPE_2D && sectorNoMax == 16 ) ) return false;
  if ( !( cylinder >= 0 && cylinder <= 39 ) ) return false;
  if ( !( head >= 0 && head <= 1 ) )          return false;
  m_formatfp = NULL;
  auto fp = fopen( m_imgFileName, "r+" );
  if ( !fp ) return false;
  m_formatfp = fp;
  return true;
}


void media::formatTrackEnd( void )
{
  if ( !m_formatfp ) return;
  fclose( m_formatfp );
  m_formatfp = NULL;  
}


bool media::formatSector( int cylinder, int head, int sectorNo, int sectorLen, uint8_t fillData, int length )
{
  if ( !( m_d88Type && m_inserted && m_mediaType == MEDIA_TYPE_2D && sectorLen == 1 ) ) return false;
  if ( !( cylinder >= 0 && cylinder <= 39 ) ) return false;
  if ( !( head >= 0 && head <= 1 ) )          return false;
  if ( !( sectorNo >= 1 && sectorNo <= 16 ) ) return false;
  if ( !( length == m_sectorSize ) )          return false;
  int d88SectorSize = 16 + m_sectorSize;
  int logicalTrack = ( cylinder * 2 ) + head;
  size_t result = 0;
  bool format = false;
  int seekPos = sizeof( D88_FILE_HEADER )
              + ( d88SectorSize * 16 * logicalTrack )
              + ( d88SectorSize * ( sectorNo - 1 ) );
  int sectorPos = m_d88FileHeader->trackTable[cylinder]
                + ( d88SectorSize * 16 * cylinder )
                + ( d88SectorSize * 16 * head )
                + ( d88SectorSize * ( sectorNo - 1 ) );
  if ( seekPos != sectorPos ) return false;
  if ( seekPos > m_fileSize ) return false;
  if ( !m_formatfp ) return false;
  fseek( m_formatfp, seekPos, SEEK_SET );
  result = fread( &m_d88rwBuffer[0], 1, d88SectorSize, m_formatfp );
  if ( result == d88SectorSize )
  {
    memset( &m_d88rwBuffer[16], fillData, m_sectorSize );
    fseek( m_formatfp, seekPos, SEEK_SET );
    result = fwrite( &m_d88rwBuffer[0], 1, d88SectorSize, m_formatfp );
    if ( result == d88SectorSize )
      format = true;
  }
  return format;
}


void media::setDeleted( bool value )
{
  if ( m_d88rwBuffer != NULL )
  {
    PD88_FILE_SECTOR d88FileSector = (PD88_FILE_SECTOR)m_d88rwBuffer;
    d88FileSector->deletedMark = value ? 0x10 : 0x00;
    if ( ( d88FileSector->status & 0xF0 ) == 0x00
      || ( d88FileSector->status & 0xF0 ) == 0x10 )
      d88FileSector->status = ( d88FileSector->status & 0x0F ) | d88FileSector->deletedMark;
  }
  m_deleted = value;
}

