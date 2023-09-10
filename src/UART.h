/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2023 tan
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
#include <string.h>
#include <esp32-hal-uart.h>

#include "emu.h"

extern  void  onSerialReceive( void );
extern  void  onSerialReceiveError( hardwareSerial_error_t errorID );

// uPD8251 A0=1 CONTROL & STATUS WORD
// MODE SETUP
// 76543210
// ||||||++--- BAUD RATE FACTOR
// ||||||      00 : SYNC MODE                                       Not implement
// ||||||      01 : 1X                                              Not implement
// ||||||      10 : 16X
// ||||||      11 : 64X
// ||||++----- CHARACTER LENGTH
// ||||        00 : 5BITS
// ||||        01 : 6BITS
// ||||        10 : 7BITS
// ||||        11 : 8BITS
// |||+------- PARITY ENABLE
// |||         1 : ENABLE
// |||         0 : DISABLE
// ||+-------- EVEN PARITY GENARATION/CHECK
// ||          1 : EVEN
// ||          0 : ODD
// ++--------- NUMBER OF STOP BITS
//             00 : INVALID
//             01 : 1 BIT
//             10 : 1.5 BITS                                        Not implement
//             11 : 2 BITS
//
// COMMAND SETUP
// 76543210
// |||||||+--- TRANSMIT ENABLE
// |||||||     1 : ENABLE
// |||||||     0 : DISABLE
// ||||||+---- DATA TERMINAL READY                                  Not implement
// ||||||      "high" will force ~DTR output zero
// |||||+----- RECEIVE ENABLE
// |||||       1 : ENABLE
// |||||       0 : DISABLE
// ||||+------ SEND BREAK CHARACTER
// ||||        1 : forces TxD "low"
// ||||        0 : normal operation
// |||+------- ERROR RESET
// |||         1 : reset all error flags PE,OE,FE
// ||+-------- REQUEST TO SEND                                      Not implement
// ||          "high" will force ~RTS output zero
// |+--------- INTERNAL RESET
// |           "high" returns USART to Mode Instruction Format
// +---------- ENTER HUNT MODE                                      Not implement
//             1 = enable search for Sync Characters
// 
// STATUS READ
// 76543210
// |||||||+--- TxRDY
// ||||||+---- RxRDY
// |||||+----- TxEMPTY
// ||||+------ PE(PARITY ERROR)
// |||+------- OE(OVERRUN ERROR)
// ||+-------- FE(FRAMING ERROR)
// |+--------- SYNDET                                               Not implement
// +---------- DSR                                                  Not implement

#define _STATUS_CLEAR     ((uint8_t)(0x00))
#define _STATUS_TxRDY     ((uint8_t)(0x01))
#define _STATUS_RxRDY     ((uint8_t)(0x02))
#define _STATUS_TxMPTY    ((uint8_t)(0x04))
#define _STATUS_PE        ((uint8_t)(0x08))
#define _STATUS_OE        ((uint8_t)(0x10))
#define _STATUS_FE        ((uint8_t)(0x20))
#define _STATUS_SYNDET    ((uint8_t)(0x40))
#define _STATUS_DSR       ((uint8_t)(0x80))

typedef struct _TAG_SERIAL_COND_TBL {
  int       dataBits;           // 5/6/7/8
  bool      parityEnable;       // false/true
  bool      evenParity;         // false/true
  int       stopBits;           // 1:1BIT/3:2BITS
  uint32_t  serialCondValue;    // esp32-hal-uart.h #define SERIAL_xyz
} _SERIAL_COND_TBL, * _PSERIAL_COND_TBL;

static const _SERIAL_COND_TBL _serialCondTbl[] PROGMEM = {
  5, false, false, 1, SERIAL_5N1,
  6, false, false, 1, SERIAL_6N1,
  7, false, false, 1, SERIAL_7N1,
  8, false, false, 1, SERIAL_8N1,
  5, false, false, 3, SERIAL_5N2,
  6, false, false, 3, SERIAL_6N2,
  7, false, false, 3, SERIAL_7N2,
  8, false, false, 3, SERIAL_8N2,
  5, true,  true,  1, SERIAL_5E1,
  6, true,  true,  1, SERIAL_6E1,
  7, true,  true,  1, SERIAL_7E1,
  8, true,  true,  1, SERIAL_8E1,
  5, true,  true,  3, SERIAL_5E2,
  6, true,  true,  3, SERIAL_6E2,
  7, true,  true,  3, SERIAL_7E2,
  8, true,  true,  3, SERIAL_8E2,
  5, true,  false, 1, SERIAL_5O1,
  6, true,  false, 1, SERIAL_6O1,
  7, true,  false, 1, SERIAL_7O1,
  8, true,  false, 1, SERIAL_8O1,
  5, true,  false, 3, SERIAL_5O2,
  6, true,  false, 3, SERIAL_6O2,
  7, true,  false, 3, SERIAL_7O2,
  8, true,  false, 3, SERIAL_8O2
};

class UART
{
public:
  // callbacks
  typedef void (* intReqCallback)( void * context, int value );

  void setCallbacks( void * context, intReqCallback intReq )
  {
    m_context = context;
    m_intReq  = intReq;
  }

  bool reset( int selEsp32UartType, int selEsp32UartBpsType, int selEsp32UartBps )
  {
    m_txEnable            = false;
    m_rxEnable            = false;
    m_commandSetup        = true;
    m_rxEmpty             = true;
    m_receiveData         = 0x00;
    m_statusData          = _STATUS_CLEAR;
    m_m8214CurrentStatus  = 0xFF;
    m_selEsp32UartType    = selEsp32UartType;
    m_selEsp32UartBpsType = selEsp32UartBpsType;
    m_selEsp32UartBps     = selEsp32UartBps;
    m_mutexUART           = portMUX_INITIALIZER_UNLOCKED;
    Serial.onReceive( NULL );
    Serial.onReceiveError( NULL );
    return true;
  }

  bool readIO( int address, int * result )
  {
    bool rdyAddress = false;
    bool statusRead = false;
    if ( m_selEsp32UartType == 1 )
    {
      if ( address == 0xC0 )
      {
        rdyAddress = true;
        statusRead = false;
      }
      if ( address == 0xC1 )
      {
        rdyAddress = true;
        statusRead = true;
      }
    }
    if ( m_selEsp32UartType == 2 )
    {
      if ( address == 0xC2 )
      {
        rdyAddress = true;
        statusRead = false;
      }
      if ( address == 0xC3 )
      {
        rdyAddress = true;
        statusRead = true;
      }
    }
    if ( rdyAddress )
    {
      portENTER_CRITICAL( &m_mutexUART );
      if ( statusRead )
        *result = m_statusData;
      else
      {
        *result = 0x00;
        if ( m_rxEnable && !m_rxEmpty )
        {
          m_rxEmpty = true;
          // RxRDY : false
          m_statusData &= ~( _STATUS_RxRDY );
          *result = m_receiveData;
        }
      }
      portEXIT_CRITICAL( &m_mutexUART );
      return true;
    }
    return false;
  }

  bool writeIO( int address, int value )
  {
    if ( address == 0xE4 )
    {
      portENTER_CRITICAL( &m_mutexUART );
      // Not implement : MASTER uPD8214 INTERRURT PRIORITY CONTROL
      m_m8214CurrentStatus = value;
      portEXIT_CRITICAL( &m_mutexUART );
      return true;
    }
    bool rdyAddress  = false;
    bool modeCommand = false;
    if ( m_selEsp32UartType == 1 )
    {
      if ( address == 0xC0 )
      {
        rdyAddress  = true;
        modeCommand = false;
      }
      if ( address == 0xC1 )
      {
        rdyAddress  = true;
        modeCommand = true;
      }
    }
    if ( m_selEsp32UartType == 2 )
    {
      if ( address == 0xC2 )
      {
        rdyAddress  = true;
        modeCommand = false;
      }
      if ( address == 0xC3 )
      {
        rdyAddress  = true;
        modeCommand = true;
      }
    }
    if ( rdyAddress )
    {
      if ( modeCommand )
      {
        if ( m_commandSetup )
        {
          m_txEnable         = ( ( value & 0x01 ) ? true : false );
          bool dtrEnable     = ( ( value & 0x02 ) ? true : false );
          m_rxEnable         = ( ( value & 0x04 ) ? true : false );
          bool sendBreak     = ( ( value & 0x08 ) ? true : false );
          bool errReset      = ( ( value & 0x10 ) ? true : false );
          bool rtsEnable     = ( ( value & 0x20 ) ? true : false );
          bool internalReset = ( ( value & 0x40 ) ? true : false );
          bool huntSyncMode  = ( ( value & 0x80 ) ? true : false );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:HSY(%d) IRT(%d) RTS(%d) ERT(%d) SBK(%d) RXE(%d) DTR(%d) TXE(%d) 0x%02X\r\n", 
                        __func__, __LINE__,
                        m_selEsp32UartType,
                        huntSyncMode,
                        internalReset,
                        rtsEnable,
                        errReset,
                        sendBreak,
                        m_rxEnable,
                        dtrEnable,
                        m_txEnable,
                        value );
          if ( internalReset )
          {
            portENTER_CRITICAL( &m_mutexUART );
            m_statusData = _STATUS_CLEAR;
            m_rxEmpty    = false;
            portEXIT_CRITICAL( &m_mutexUART );
            Serial.onReceive( NULL );
            Serial.onReceiveError( NULL );
            m_commandSetup = false;
          }
          else
          {
            // Not implement : DTR
            if ( dtrEnable )
              dtrEnable = false;
            // Not implement : RTS
            if ( rtsEnable )
              rtsEnable = false;
            // Not implement : ENTER HUNT MODE
            if ( huntSyncMode )
              huntSyncMode = false;
            if ( sendBreak )
              uart_send_break( 0 );
            if ( errReset )
            {
              portENTER_CRITICAL( &m_mutexUART );
              // PE(PARITY ERROR)  : false
              // OE(OVERRUN ERROR) : false
              // FE(FRAMING ERROR) : false
              m_statusData &= ~( _STATUS_FE | _STATUS_OE | _STATUS_PE );
              portEXIT_CRITICAL( &m_mutexUART );
            }
            if ( m_txEnable )
            {
              portENTER_CRITICAL( &m_mutexUART );
              // TxRDY   : true (CTS ignore)
              // TxEMPTY : true
              m_statusData |= ( _STATUS_TxMPTY | _STATUS_TxRDY );
              portEXIT_CRITICAL( &m_mutexUART );
            }
            Serial.onReceive( NULL );
            Serial.onReceiveError( NULL );
            if ( m_rxEnable )
            {
              m_rxEmpty = true;
              portENTER_CRITICAL( &m_mutexUART );
              // RxRDY : false
              m_statusData &= ~( _STATUS_RxRDY );
              portEXIT_CRITICAL( &m_mutexUART );
              Serial.onReceive( &onSerialReceive );
              Serial.onReceiveError( &onSerialReceiveError );
            }
          }
        }
        else
        {
          int baudRateFactor = ( value & 0x03 );
          int dataBits       = 5 + ( ( value & 0x0C ) >> 2 );
          bool parityEnable  = ( ( value & 0x10 ) ? true : false );
          bool evenParity    = ( ( value & 0x20 ) ? true : false );
          int stopBits       = ( value & 0xC0 ) >> 6;
          uint32_t serialCondValue = 0;
          for ( int i = 0; i < ARRAY_SIZE( _serialCondTbl ); i++ )
          {
            if ( _serialCondTbl[i].dataBits     == dataBits
              && _serialCondTbl[i].parityEnable == parityEnable
              && _serialCondTbl[i].evenParity   == evenParity
              && _serialCondTbl[i].stopBits     == stopBits )
            {
              serialCondValue = _serialCondTbl[i].serialCondValue;
              break;
            }
          }
          _DEBUG_PRINT( "%s(%d) UART ch#%d:uPD8251 MODE(0x%02X)\r\n",    __func__, __LINE__, m_selEsp32UartType, value );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:baudRateFactor(%d)\r\n",      __func__, __LINE__, m_selEsp32UartType, baudRateFactor );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:dataBits(%d)\r\n",            __func__, __LINE__, m_selEsp32UartType, dataBits );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:parityEnable(%d)\r\n",        __func__, __LINE__, m_selEsp32UartType, parityEnable );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:evenParity(%d)\r\n",          __func__, __LINE__, m_selEsp32UartType, evenParity );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:stopBits(%d)\r\n",            __func__, __LINE__, m_selEsp32UartType, stopBits );
          _DEBUG_PRINT( "%s(%d) UART ch#%d:serialCondValue(0x%04X)\r\n", __func__, __LINE__, m_selEsp32UartType, serialCondValue );
          // Not implement : SYNC MODE and Baud rate Factor 1x
          if ( serialCondValue && ( baudRateFactor == 2 || baudRateFactor == 3 ) )
          {
            Serial.end();
            Serial.begin( m_selEsp32UartBps, serialCondValue );
            while ( !Serial && !Serial.available() );
            Serial.onReceive( NULL );
            Serial.onReceiveError( NULL );
            if ( m_rxEnable )
            {
              portENTER_CRITICAL( &m_mutexUART );
              m_rxEmpty = true;
              // RxRDY             : false
              // PE(PARITY ERROR)  : false
              // OE(OVERRUN ERROR) : false
              // FE(FRAMING ERROR) : false
              m_statusData &= ~( _STATUS_FE | _STATUS_OE | _STATUS_PE | _STATUS_RxRDY );
              portEXIT_CRITICAL( &m_mutexUART );
              Serial.onReceive( &onSerialReceive );
              Serial.onReceiveError( &onSerialReceiveError );
            }
          }
          m_commandSetup = true;
        }
      }
      else if ( m_txEnable )
        Serial.write( value );
      return true;
    }
    return false;
  }

  static void onReceive( void * context )
  {
    auto * m = (UART *)context;
    int recvData = Serial.read();
    portENTER_CRITICAL( &m->m_mutexUART );
    if ( m->m_rxEmpty && ( m->m_statusData & ( _STATUS_FE | _STATUS_OE | _STATUS_PE) ) == 0 )
    {
      m->m_receiveData = recvData;
      m->m_rxEmpty = false;
      // RxRDY : true
      m->m_statusData |= _STATUS_RxRDY;
      int vectorLoAdrs = 0;
      // Not implement : MASTER uPD8214 INTERRURT PRIORITY CONTROL
      if ( m->m_selEsp32UartType == 1 && m->m_m8214CurrentStatus == 0xFF )
        vectorLoAdrs = 0x08;
      // Not implement : MASTER uPD8214 INTERRURT PRIORITY CONTROL
      if ( m->m_selEsp32UartType == 2 && m->m_m8214CurrentStatus == 0xFF )
        vectorLoAdrs = 0x0A;
      if ( vectorLoAdrs )
        m->m_intReq( m->m_context, vectorLoAdrs );
    }
    else
    {
      // OE(OVERRUN ERROR) : true
      m->m_statusData |= _STATUS_OE;
    }
    portEXIT_CRITICAL( &m->m_mutexUART );
  }

  static void onReceiveError( void * context, hardwareSerial_error_t errorID )
  {
    auto * m = (UART *)context;
    switch ( (int)errorID )
    {
      case UART_BUFFER_FULL_ERROR:
      case UART_FIFO_OVF_ERROR:
        portENTER_CRITICAL( &m->m_mutexUART );
        // OE(OVERRUN ERROR) : true
        m->m_statusData |= _STATUS_OE;
        portEXIT_CRITICAL( &m->m_mutexUART );
        break;
      case UART_FRAME_ERROR:
        portENTER_CRITICAL( &m->m_mutexUART );
        // FE(FRAMING ERROR) : true
        m->m_statusData |= _STATUS_FE;
        portEXIT_CRITICAL( &m->m_mutexUART );
        break;
      case UART_PARITY_ERROR:
        portENTER_CRITICAL( &m->m_mutexUART );
        // PE(PARITY ERROR) : true
        m->m_statusData |= _STATUS_PE;
        portEXIT_CRITICAL( &m->m_mutexUART );
        break;
    }
  }

private:
  bool            m_txEnable;             // flag : uPD8251 TRANSMIT ENABLE
  bool            m_rxEnable;             // flag : uPD8251 RECEIVE ENABLE
  bool            m_commandSetup;         // flag : uPD8251 COMMAND SETUP
  bool            m_rxEmpty;              // flag : uPD8251 EMPTY RECEIVE DATA
  uint8_t         m_receiveData;          // uPD8251 RECEIVE DATA
  uint8_t         m_statusData;           // uPD8251 STATUS
  uint8_t         m_m8214CurrentStatus;   // MASTER uPD8214 CURRENT STATUS
                                          //   0xFF : NON INTERRUPT
                                          //   0    : INT15 GP-IB          Not implement
                                          //   1    : INT14 GP-IB          Not implement
                                          //   2    : INT13 RTC            Not implement 
                                          //   3    : INT12 RESERVE
                                          //   4    : INT11 RS-232C ch#1   Select ESP32 UART
                                          //   5    : INT10 RS-232C ch#2   Select ESP32 UART
                                          //   6    : INT9  PARALLEL I/O   Not implement 
                                          //   7    : INT8  PARALLEL I/O   Not implement 
  int             m_selEsp32UartType;     // ESP32 UART TYPE
                                          //   0 : PRINTER(115200bps 8N1, _DEBUG true 併用)
                                          //   1 : PC-8011 USART channel1
                                          //   2 : PC-8011 USART channel2
  int             m_selEsp32UartBpsType;  // PC-8011 USART factor and bps type
                                          //   0 : 16x 9600bps
                                          //   1 : 16x 4800bps
                                          //   2 : 16x 2400bps
                                          //   3 : 16x 1200bps
                                          //   4 : 16x  600bps
                                          //   5 : 16x  300bps
                                          //   6 : 64x 1200bps
                                          //   7 : 64x  600bps
                                          //   8 : 64x  300bps
  long            m_selEsp32UartBps;      // PC-8011 USART bps

  void *          m_context;              // callbacks
  intReqCallback  m_intReq;               // callback : intReq()
  portMUX_TYPE    m_mutexUART;            // mutex UART Status and UART Receive Data
};
