/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

  Arduino IDE board : Arduino ESP32 v1.0.6 or v2.0.5
  library           : FabGL v1.0.9
  target            : ESP32 DEV Module(ESP32-Wrover)
  flash size        : 4MB(32Mb)
  partition scheme  : Huge APP (3MB No OTA/1MB SPIFFS)
  PSRAM             : Enabled

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

// 問題点
//
// 1. 全般
//    Arduino ESP32 v2.0.5 & FabGL v1.0.9
//      微妙に遅い
//    Arduino ESP32 v1.0.6 & FabGL v1.0.9
//      微妙に遅い
//
// 2. WIDTH 80,20 表示崩れ
//    なして発生するのかわがんね。単体テストはOK。このモードは放置。
//    Arduino ESP32 v2.0.5 だとだいぶましだけど瞬時崩れが随時見える。
//    やっぱ 200LINE をフォント高さ 10 でスキャンライン描画は想定外なのかも知れない。
//    WIDTH 40,20 はいけてるのに。かなしすｗ
//
// 3. beep音、seek音が微妙
//    実機(PC-8001/PC-8033/PC-80S31)から生録したデータを
//    FabGL準拠のサンプリングレート16kHz/mono/8bitデータ化で
//    再生してるけど元がヘタれてる?(ｗ) やっぱコンデンサ全交換要かｗ
//
// microSD
// /
// +--PC8001
//    +-- PC-8001.ROM or 8801-N80.ROM
//    +-- PC-8001.FON
//    +-- USER.ROM       (OPTION) [8192bytes]
//    +-- PC-8031-2W.ROM (OPTION) [2048bytes]
//    +-- PC-80S31.ROM   (OPTION) [2048bytes=PC-8801mkIISR/FR/...]
//    +-- DISK.ROM       (OPTION) [8192bytes=PC-8801MA/MC/MH/...]
//    |
//    +--MEDIA
//       +-- DISK
//       |   +--- *.d88  (OPTION) 5.25 inch 2D Type
//       |
//       +-- *.cmt
//       +-- *.n80

// F9  : open menu
// F10 : PCG-8100 enable/disable (toggle)
// F11 : 専用機化ｗ
// F12 : hard reset(ESP32 restart)
// SHIFT+F12 : soft reset(FDCリセットを除外した形式。DISK Bootを早くしたいから作った。オイオイｗ)

#include <memory>
#include <Preferences.h>

#include "fabgl.h"
#include "fabutils.h"

#include "src/machine.h"

using std::unique_ptr;

Preferences   preferences;
InputBox ibox;
Machine   * m;
bool writeProtected = false;
bool selDiskRomEnable = true;


void n80FabglMenu( void )
{
  m->cga()->enableVideo( false );
  const char * menuItem = NULL;
  char fileName[256];
  char path[256];
  char drvItem[1024];
  char drvName[256];
  int maxDrive;
  memset( fileName, 0, sizeof( fileName ) );
  strcpy( path, SD_MOUNT_PATH );
  strcat( path, PC8001MEDIA );
  if ( m->getDiskRomEnable() )
  {
    if ( selDiskRomEnable )
      menuItem = "Select Files;Browse Files;Hard Reset(ESP32 Restart);Soft Reset;Continue;Disk Rom Disable;Mount Disk;Unmount Disk";
    else
      menuItem = "Select Files;Browse Files;Hard Reset(ESP32 Restart);Soft Reset;Continue;Disk Rom Enable";
  }
  else
    menuItem = "Select Files;Browse Files;Hard Reset(ESP32 Restart);Soft Reset;Continue";
  if ( m->get2kRomLoaded() )
    maxDrive = 4;
  else
    maxDrive = 2;
  memset( drvItem, 0, sizeof( drvItem ) );
  for ( int drv = 0; drv < maxDrive; drv++ )
  {
    sprintf( drvName, "fd%d:%s;", drv, m->getDiskImageFileName( drv ) );
    strcat( drvItem, drvName );
  }
  int len = strlen( drvItem );
  drvItem[len-1] = 0;
  ibox.begin( VGA_640x480_60Hz, 500, 400, 4 );
  ibox.setBackgroundColor( RGB888( 0, 0, 0 ) );
  char verData[256];
  char menuTitle[512];
  memset( verData, 0, sizeof( verData ) );
  strncpy( verData, m->getVersionString(), sizeof( verData ) - 1 );
  sprintf( menuTitle, "menu %s", verData );
  int selCmd = ibox.menu( menuTitle, "Select a command", menuItem );
  int drv;

  switch ( selCmd )
  {
    // Select Files
    case 0:
      ibox.fileSelector( "Select file to load", "Filename: ", path, sizeof( path ) - 1, fileName, sizeof( fileName ) - 1 );
      break;
    // Browse Files
    case 1:
      strcpy( path, SD_MOUNT_PATH );
      strcat( path, PC8001MEDIA );
      ibox.folderBrowser( "Browse Files", path );
      break;
    // Hard Reset
    case 2:
      m->menuCmdEspRestart();
      break;
    // Soft Reset
    case 3:
      m->menuCmdReset();
      break;
    // Disk Rom Enable/Disable
    case 5:
      selDiskRomEnable ^= 1;
      preferences.putBool( "diskRomEnable", selDiskRomEnable );
      esp_restart();
      break;
    // Mount Disk
    case 6:
      drv = ibox.menu( "Mount Disk", "Select d88 Image(2D Type)", drvItem );
      if ( drv > -1 )
      {
        constexpr int MAXNAMELEN = 256;
        unique_ptr<char[]> dir( new char[MAXNAMELEN + 1] { 0 } );
        unique_ptr<char[]> filename( new char[MAXNAMELEN + 1] { 0 } );
        strcpy( path, SD_MOUNT_PATH );
        strcat( path, PC8001MEDIA_DISK );
        strcpy( dir.get(), path );
        if ( m->getDiskImageFileName( drv ) )
          strcpy( filename.get(), m->getDiskImageFileName( drv ) );
        if ( ibox.fileSelector( "Select d88 Image", "Image Filename", dir.get(), MAXNAMELEN, filename.get(), MAXNAMELEN ) == InputResult::Enter )
        {
          char key[16];
          sprintf( key, "fd%d", drv );
          char * p = filename.get();
          if ( m->setDiskImage( drv, writeProtected, p ) )
            preferences.putString( key, p );
        }
      }
      break;
    // Unmount Disk
    case 7:
      drv = ibox.menu( "Unmount Disk", "Select Floppy Drive", drvItem );
      if ( drv > -1 )
      {
        char key[16];
        sprintf( key, "fd%d", drv );
        m->setDiskImage( drv, writeProtected, "" );
        preferences.putString( key, "" );
      }
      break;
    // Continue
    default:
      break;
  }

  ibox.end();
  m->menu2FileName( fileName );
  m->cga()->enableVideo( true );
}

void setup()
{
  Serial.begin( 115200 );
  while ( !Serial && !Serial.available() );
#ifdef NARYA_2_0
  // RXD(RD) : GPIO34
  // TXD(SD) : GPIO26
  //Serial2.begin( 4800, SERIAL_8N1, 34, 26 );
  Serial2.begin( 9600, SERIAL_8N1, 34, 26 );
  while ( !Serial2 && !Serial2.available() );
#endif

  disableCore0WDT();
  // experienced crashes without this delay!
  delay( 100 );
  disableCore1WDT();

  preferences.begin( "n80FabGL", false );

  // uncomment to clear preferences
  //preferences.clear();

  // save some space reducing UI queue
  fabgl::BitmappedDisplayController::queueSize = 128;

  selDiskRomEnable = preferences.getBool( "diskRomEnable", true );
  char fdImgFileName[4][256];
  memset( fdImgFileName, 0, sizeof( fdImgFileName ) );
  preferences.getString( "fd0", fdImgFileName[0], sizeof( fdImgFileName[0] ) );
  preferences.getString( "fd1", fdImgFileName[1], sizeof( fdImgFileName[1] ) );
  preferences.getString( "fd2", fdImgFileName[2], sizeof( fdImgFileName[2] ) );
  preferences.getString( "fd3", fdImgFileName[3], sizeof( fdImgFileName[3] ) );

  m = new Machine;
  m->outputEsp32Spec();

  int result;
  if ( (result = m->init( selDiskRomEnable )) != 0 )
  {
    ibox.begin( VGA_640x480_60Hz, 500, 400, 4 );
    ibox.setBackgroundColor( RGB888( 0, 0, 0 ) );
    const char * errMsg = "?";
    switch ( result )
    {
      case -1:
        errMsg = "Out of memory (VGA frame buffer)";
        break;
      case -2:
        errMsg = "SD mount failed";
        break;
      case -3:
        errMsg = "JP keyboard layout non";
        break;
      case -4:
        errMsg = "Out of memory (I/O)";
        break;
      case -5:
        errMsg = "Out of memory (RAM/extRAM)";
        break;
      case -6:
        errMsg = "Out of memory (CMT)";
        break;
      case -7:
        errMsg = "ROM file load failed";
        break;
      case -8:
        errMsg = "FONT file load failed";
        break;
      case -9:
        errMsg = "Out of memory (BUZZER SoundGenerator)";
        break;
      case -10:
        errMsg = "Out of memory (DISK::uPD765A SoundGenerator)";
        break;
      case -11:
        errMsg = "Out of memory (PCG SquareWaveformGenerator)";
        break;
    }
    ibox.message( "Error!", errMsg, nullptr, nullptr);
  }

  m->outputEsp32FreeHeap();
  m->setMenuCallback( n80FabglMenu );

  if ( m->getDiskRomEnable() && selDiskRomEnable )
  {
    for ( int drv = 0; drv < 4; drv++ )
      result = m->setDiskImage( drv, writeProtected, fdImgFileName[drv] );
  }

  // CPU execute
  m->run();
}

void loop()
{
  m->backGround();
}
