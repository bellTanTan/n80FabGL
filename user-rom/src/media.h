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

#include "fabgl.h"
#include "fabutils.h"

using namespace fabgl;

#include "emu.h"

// d88 media type
#define MEDIA_TYPE_2D       0x00
#define MEDIA_TYPE_UNKNOWN  0xFF


class media
{
public:
  // https://github.com/jpzm/wii88/blob/master/document/FORMAT.TXT
  typedef struct _D88_FILE_HEADER {
    char      name[16];         // disk name 
    uint8_t   reserve[10];      // reserve
    uint8_t   writeProtect;     // write protect false(0x00) true(0x10)
    uint8_t   diskType;         // disk type
                                // 0x00 : 2D
                                // 0x10 : 2DD
                                // 0x20 : 2HD
    uint32_t  diskSize;         // disk size
    uint32_t  trackTable[164];  // track table
  } __attribute__((packed)) D88_FILE_HEADER, * PD88_FILE_HEADER;

  typedef struct _D88_FILE_SECTOR {
    uint8_t   cylinder;         // C : cylinder
    uint8_t   head;             // H : head(side)
    uint8_t   sector;           // R : sector
    uint8_t   sectorSize;       // N : sector size 128byte(0x00) 256byte(0x01) 512byte(0x02) ... 0x80 << n
    uint16_t  numberOfSector;   // number of sector
    uint8_t   density;          // 0x00 : double density
                                // 0x40 : single density
                                // 0x01 : High density
    uint8_t   deletedMark;      // 0x00 : normal
                                // 0x10 : deleted
    uint8_t   status;           // 0x00 : normal
                                // 0x10 : normal(deleted data)
                                // 0xA0 : id crc error
                                // 0xB0 : data crc error
                                // 0xE0 : non address mark  
                                // 0xF0 : non data mark
    uint8_t   reserve[5];       // reserve
    uint16_t  sizeOfData;       // size of data size
  } __attribute__((packed)) D88_FILE_SECTOR, * PD88_FILE_SECTOR;

  media();
  ~media();

  bool setSectorBuffer( uint8_t * d88rwBuffer );

  bool insert( bool writeProtected, const char * imgFileName );

  void eject( void );

  bool readSector( int cylinder, int head, int sectorNo, int sectorLen );

  bool writeSector( int cylinder, int head, int sectorNo, int sectorLen );

  bool formatTrackBegin( int cylinder, int head, int sectorNoMax );
  void formatTrackEnd( void );
  bool formatSector( int cylinder, int head, int sectorNo, int sectorLen, uint8_t fillData, int length );

  void setDeleted( bool value );

  bool getInserted( void )              { return m_inserted; }
  bool getWriteProtected( void )        { return m_writeProtected; }
  bool getCrcError( void )              { return m_crcError; }
  bool getDeleted( void )               { return m_deleted; }
  bool getDriveMfm( void )              { return m_driveMfm; }

private:
  PD88_FILE_HEADER  m_d88FileHeader;    // d88 File Header
  char              m_imgFileName[256]; // Image file name
  size_t            m_fileSize;         // Image file size
  bool              m_d88Type;          // flag : d88 type
  bool              m_inserted;         // flag : media inserted
  bool              m_ejected;          // flag : media ejected
  bool              m_writeProtected;   // flag : media write protected
  uint8_t           m_mediaType;        // d88 media type
  uint8_t *         m_d88rwBuffer;      // d88 sector read write buffer (16 + 256 bytes)
  uint8_t *         m_sectorBuffer;     // d88 sector read write buffer (&m_d88rwBuffer[16])
  size_t            m_sectorSize;       // d88 sector size (16 + 256 bytes)
  bool              m_deleted;          // flag : deleted
  bool              m_crcError;         // flag : crc error
  bool              m_driveMfm;         // flag : driver type Modified Frequency Modulation
  FILE *            m_formatfp;         // format track
};

