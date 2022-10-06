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

#include <stdint.h>
#include <stdarg.h>
#include <string.h>


//#define NARYA_2_0
//#define DONT_CMT_SAVE_INDICATOR
//#define DONT_KANA_LOCK_INDICATOR
//#define DONT_IME_LOCK_INDICATOR
//#define DONT_DISK_ACCESS_INDICATOR
//#define DONT_SEEK_SOUND
//#define DONT_DISKCPU_SUSPEND
//#define UPD765A_DONT_WAIT_SEEK

//#define _DEBUG
//#define _UPD765A_DEBUG
//#define _KEYBRD_LAYOUT_ALLCHECK
//#define _ENABLE_USERROMAREA_RAM

#define SD_MOUNT_PATH           "/SD"
#define PC8001ROM               "/PC8001/PC-8001.ROM"
#define PC8001_USEROM           "/PC8001/USER.ROM"
#define PC8801_N80ROM           "/PC8001/8801-N80.ROM"
#define PC80312W_ROM            "/PC8001/PC-8031-2W.ROM"
#define PC80S31_ROM             "/PC8001/PC-80S31.ROM"
#define DISK_ROM                "/PC8001/DISK.ROM"
#define PC8001FONT              "/PC8001/PC-8001.FON"
#define PC8001MEDIA             "/PC8001/MEDIA"
#define PC8001MEDIA_DISK        "/PC8001/MEDIA/DISK"
#define CMT_FILE_EXTENSION      ".cmt"
#define N80_FILE_EXTENSION      ".n80"
#define D88_FILE_EXTENSION      ".d88"
#define VIDEO_MEM_SIZE          (80 * 2 * 25)
#define MAX_DRIVE               (4)

#define ARRAY_SIZE( array )     ( (int)( sizeof( array ) / sizeof( (array)[0] ) ) )
#define HLT                     { while ( 1 ) { delay( 500 ); } }

#ifdef _DEBUG
  extern  void _DEBUG_PRINT( const char * format, ... );
#else
  #define _DEBUG_PRINT( format, ... ) { }
#endif // _DEBUG

#ifdef _UPD765A_DEBUG
  extern  void _UPD765A_DEBUG_PRINT( const char * format, ... );
#else
  #define _UPD765A_DEBUG_PRINT( format, ... ) { }
#endif // _UPD765A_DEBUG

extern  void _MSG_PRINT( const char * format, ... );
