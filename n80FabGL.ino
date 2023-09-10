/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022-2023 tan
  All rights reserved.

  Arduino IDE board : Arduino ESP32 v2.0.12
  library           : FabGL v1.0.9
  target            : ESP32 DEV Module(ESP32-Wrover)
  flash size        : 4MB(32Mb)
  partition scheme  : Huge APP (3MB No OTA/1MB SPIFFS)
  PSRAM             : Enabled
  Arduino Run On    : Core 1
  Event Run On      : Core 1

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
//    Arduino ESP32 v2.0.12 & FabGL v1.0.9
//      微妙に遅い
//
// 2. WIDTH 80,20 表示崩れ
//    なして発生するのかわがんね。単体テストはOK。このモードは放置。
//    Arduino ESP32 v2.0.5(以降)だとだいぶましだけど瞬時崩れが随時見える。
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
//       |   +-- *.d88   (OPTION) 5.25 inch 2D Type
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
StringList slist;
Machine   * m;
bool writeProtected = false;
bool selDiskRomEnable = true;
int8_t selEsp32UartType = 0;          // 0 : PRINTER(115200bps 8N1, _DEBUG true 併用)
                                      // 1 : PC-8011 USART channel1
                                      // 2 : PC-8011 USART channel2
int8_t selEsp32UartBpsType = 0;       // 0 : 16x 9600bps
                                      // 1 : 16x 4800bps
                                      // 2 : 16x 2400bps
                                      // 3 : 16x 1200bps
                                      // 4 : 16x  600bps
                                      // 5 : 16x  300bps
                                      // 6 : 64x 1200bps
                                      // 7 : 64x  600bps
                                      // 8 : 64x  300bps
long selEsp32UartBps = 115200;
int8_t selNARYA20GroveUartType = 2;   // 0 : 8N1 115200bps( selEsp32UartType != 0 && _DEBUG true 併用)
                                      // 1 : 8N1 9600bps
                                      // 2 : 8N1 4800bps

void n80FabglMenu( void )
{
  m->cga()->enableVideo( false );
  char menuItem[512];
  char fileName[256];
  char path[256];
  char drvItem[1024];
  char drvName[256];
  char msg[128];
  int facter = 0;
  int bps = 0;
  int maxDrive;
  memset( fileName, 0, sizeof( fileName ) );
  strcpy( path, SD_MOUNT_PATH );
  strcat( path, PC8001MEDIA );
  strcpy( menuItem, "Select Files;Browse Files;Hard Reset(ESP32 Restart);Soft Reset;Continue" );
  if ( selEsp32UartType == 0 )
    strcat( menuItem, ";ESP32 UART [PRINTER 8N1 115200bps]" );
  if ( selEsp32UartType == 1 || selEsp32UartType == 2 )
  {
    strcat( menuItem, ";ESP32 UART [PC-8011 USART channel#" );
    sprintf( msg, "%d", selEsp32UartType );
    strcat( menuItem, msg );
    switch ( selEsp32UartBpsType )
    {
      case 0:
        facter = 16;
        bps = 9600;
        break;
      case 1:
        facter = 16;
        bps = 4800;
        break;
      case 2:
        facter = 16;
        bps = 2400;
        break;
      case 3:
        facter = 16;
        bps = 1200;
        break;
      case 4:
        facter = 16;
        bps = 600;
        break;
      case 5:
        facter = 16;
        bps = 300;
        break;
      case 6:
        facter = 64;
        bps = 1200;
        break;
      case 7:
        facter = 64;
        bps = 600;
        break;
      case 8:
        facter = 64;
        bps = 300;
        break;
    }
    sprintf( msg, " %dx %dbps]", facter, bps );
    strcat( menuItem, msg );
  }
  switch ( selNARYA20GroveUartType )
  {
    case 0:
      bps = 115200;
      break;
    case 1:
      bps = 9600;
      break;
    case 2:
      bps = 4800;
      break;
  }
  sprintf( msg, ";NARYA2.0 GROVE UART(CMT PORT) [8N1 %dbps]", bps );
  strcat( menuItem, msg );
  if ( m->getDiskRomEnable() )
  {
    if ( selDiskRomEnable )
      strcat( menuItem, ";Disk Rom Disable;Mount Disk;Unmount Disk" );
    else
      strcat( menuItem, ";Disk Rom Enable" );
  }
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
  int selDat;

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
    // Continue
    case 4:
    default:
      break;
    // ESP32 Uart
    case 5:
      slist.clear();
      slist.append( "PRINTER 8N1 115200bps" );
      slist.append( "PC-8011 channel#1 16x 9600bps" );
      slist.append( "PC-8011 channel#1 16x 4800bps" );
      slist.append( "PC-8011 channel#1 16x 2400bps" );
      slist.append( "PC-8011 channel#1 16x 1200bps" );
      slist.append( "PC-8011 channel#1 16x 600bps" );
      slist.append( "PC-8011 channel#1 16x 300bps" );
      slist.append( "PC-8011 channel#1 64x 1200bps" );
      slist.append( "PC-8011 channel#1 64x 600bps" );
      slist.append( "PC-8011 channel#1 64x 300bps" );
      slist.append( "PC-8011 channel#2 16x 9600bps" );
      slist.append( "PC-8011 channel#2 16x 4800bps" );
      slist.append( "PC-8011 channel#2 16x 2400bps" );
      slist.append( "PC-8011 channel#2 16x 1200bps" );
      slist.append( "PC-8011 channel#2 16x 600bps" );
      slist.append( "PC-8011 channel#2 16x 300bps" );
      slist.append( "PC-8011 channel#2 64x 1200bps" );
      slist.append( "PC-8011 channel#2 64x 600bps" );
      slist.append( "PC-8011 channel#2 64x 300bps" );
      if ( selEsp32UartType == 0 )
        selDat = 0;
      else
        selDat = 1 + ( ( selEsp32UartType - 1 ) * 9 ) + selEsp32UartBpsType;
      slist.select( selDat, true );
      selDat = ibox.menu( "ESP32 UART", "Select ESP32 UART Type", &slist );
      if ( selDat > -1 )
      {
        if ( selDat == 0 )
        {
          selEsp32UartType = 0;
          selEsp32UartBpsType = 0;
          bps = 115200;
        }
        else
        {
          selDat--;
          if ( selDat >= 0 && selDat <= 8 )
          {
            selEsp32UartType = 1;
            selEsp32UartBpsType = selDat;
          }
          else
          {
            selEsp32UartType = 2;
            selEsp32UartBpsType = selDat - 9;
          }
        }
        preferences.begin( "n80FabGL", false );
        preferences.putChar( "uartType", selEsp32UartType );
        preferences.putChar( "uartBpsType", selEsp32UartBpsType );
        preferences.end();
        esp_restart();
      }
      break;
    // NARYA2.0 Grove Uart
    case 6:
      slist.clear();
      slist.append( "8N1 115200bps" );
      slist.append( "8N1 9600bps" );
      slist.append( "8N1 4800bps" );
      slist.select( selNARYA20GroveUartType, true );
      selDat = ibox.menu( "NARYA2.0 GROVE UART(CMT PORT)", "Select NARYA2.0 GROVE UART Type", &slist );
      if ( selDat > -1 )
      {
        selNARYA20GroveUartType = selDat;
        preferences.begin( "n80FabGL", false );
        preferences.putChar( "uartType2", selNARYA20GroveUartType );
        preferences.end();
        esp_restart();
      }
      break;
    // Disk Rom Enable/Disable
    case 7:
      selDiskRomEnable ^= 1;
      preferences.begin( "n80FabGL", false );
      preferences.putBool( "diskRomEnable", selDiskRomEnable );
      preferences.end();
      esp_restart();
      break;
    // Mount Disk
    case 8:
      selDat = ibox.menu( "Mount Disk", "Select d88 Image(2D Type)", drvItem );
      if ( selDat > -1 )
      {
        constexpr int MAXNAMELEN = 256;
        unique_ptr<char[]> dir( new char[MAXNAMELEN + 1] { 0 } );
        unique_ptr<char[]> filename( new char[MAXNAMELEN + 1] { 0 } );
        strcpy( path, SD_MOUNT_PATH );
        strcat( path, PC8001MEDIA_DISK );
        strcpy( dir.get(), path );
        if ( m->getDiskImageFileName( selDat ) )
          strcpy( filename.get(), m->getDiskImageFileName( selDat ) );
        if ( ibox.fileSelector( "Select d88 Image", "Image Filename", dir.get(), MAXNAMELEN, filename.get(), MAXNAMELEN ) == InputResult::Enter )
        {
          char key[16];
          sprintf( key, "fd%d", selDat );
          char * p = filename.get();
          if ( m->setDiskImage( selDat, writeProtected, p ) )
          {
            preferences.begin( "n80FabGL", false );
            preferences.putString( key, p );
            preferences.end();
          }
        }
      }
      break;
    // Unmount Disk
    case 9:
      selDat = ibox.menu( "Unmount Disk", "Select Floppy Drive", drvItem );
      if ( selDat > -1 )
      {
        char key[16];
        sprintf( key, "fd%d", selDat );
        m->setDiskImage( selDat, writeProtected, "" );
        preferences.begin( "n80FabGL", false );
        preferences.putString( key, "" );
        preferences.end();
      }
      break;
  }

  ibox.end();
  m->menu2FileName( fileName );
  m->cga()->enableVideo( true );
}

void onSerialReceive( void )
{
  m->onSerialReceive();
}

void onSerialReceiveError( hardwareSerial_error_t errorID )
{
  m->onSerialReceiveError( errorID );
}

void setup()
{
  disableCore0WDT();
  // experienced crashes without this delay!
  delay( 100 );
  disableCore1WDT();

  preferences.begin( "n80FabGL", false );

  // uncomment to clear preferences
  //preferences.clear();

  // save some space reducing UI queue
  fabgl::BitmappedDisplayController::queueSize = 128;

  selEsp32UartType    = preferences.getChar( "uartType", 0 );
  selEsp32UartBpsType = preferences.getChar( "uartBpsType", 0 );
  if ( selEsp32UartType == 0 )
    selEsp32UartBps = 115200;
  else
  {
    switch ( selEsp32UartBpsType )
    {
      case 0:
      default:
        selEsp32UartBpsType = 0;
        selEsp32UartBps = 9600;
        break;
      case 1:
        selEsp32UartBps = 4800;
        break;
      case 2:
        selEsp32UartBps = 2400;
        break;
      case 3:
      case 6:
        selEsp32UartBps = 1200;
        break;
      case 4:
      case 7:
        selEsp32UartBps = 600;
        break;
      case 5:
      case 8:
        selEsp32UartBps = 300;
        break;
    }
  }
  Serial.end();
  Serial.begin( selEsp32UartBps, SERIAL_8N1 );
  while ( !Serial && !Serial.available() );

  selNARYA20GroveUartType = preferences.getChar( "uartType2", 2 );
#ifdef NARYA_2_0
  long bps;
  switch ( selNARYA20GroveUartType )
  {
    case 0:
      bps = 115200;
      break;
    case 1:
      bps = 9600;
      break;
    case 2:
    default:
      selNARYA20GroveUartType = 2;
      bps = 4800;
      break;
  }
  // RXD(RD) : GPIO34
  // TXD(SD) : GPIO26
  Serial2.end();
  Serial2.begin( bps, SERIAL_8N1, 34, 26 );
  while ( !Serial2 && !Serial2.available() );
#endif

  selDiskRomEnable = preferences.getBool( "diskRomEnable", true );
  char fdImgFileName[4][256];
  memset( fdImgFileName, 0, sizeof( fdImgFileName ) );
  preferences.getString( "fd0", fdImgFileName[0], sizeof( fdImgFileName[0] ) );
  preferences.getString( "fd1", fdImgFileName[1], sizeof( fdImgFileName[1] ) );
  preferences.getString( "fd2", fdImgFileName[2], sizeof( fdImgFileName[2] ) );
  preferences.getString( "fd3", fdImgFileName[3], sizeof( fdImgFileName[3] ) );
  preferences.end();

  m = new Machine;
  m->outputEsp32Spec();

  int result;
  if ( (result = m->init( selDiskRomEnable, selEsp32UartType, selEsp32UartBpsType, selNARYA20GroveUartType )) != 0 )
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
