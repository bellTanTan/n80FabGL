; PARITY           : E EVEN
;                  : O ODD
;                  : N NON
; DATA BITS        : 8 8BITS
;                  : 7 7BITS
; STOP BITS        : 1 1BIT
;                  : 2 1.5BITS
;                  : 3 2BITS
; BAUD RATE FACTOR : 0 64X
;                  : 1 16X
; ex. E730,O710,N810
; ex. E731,O711,N811
;
; CMD RCV,N811,&HD000[,&HD000]
; CMD RCV,N811,BAS
;
;
; CONSTANT CONTROL CODE
;
SOH:            EQU     01H
STX:            EQU     02H
EOT:            EQU     04H
ACK:            EQU     06H
NAK:            EQU     15H
CAN:            EQU     18H
;
; CONSTANT I/O PORT
;
KEYBRD9:        EQU     09H
USARTDW:        EQU     20H
USARTCW:        EQU     21H             ; MODE SETUP
                                        ; 76543210
                                        ; ||||||++--- BAUD RATE FACTOR
                                        ; ||||||      00 : SYNC MODE
                                        ; ||||||      01 : 1X
                                        ; ||||||      10 : 16X
                                        ; ||||||      11 : 64X
                                        ; ||||++----- CHARACTER LENGTH
                                        ; ||||        00 : 5BITS
                                        ; ||||        01 : 6BITS
                                        ; ||||        10 : 7BITS
                                        ; ||||        11 : 8BITS
                                        ; |||+------- PARITY ENABLE
                                        ; |||         1 : ENABLE
                                        ; |||         0 : DISABLE
                                        ; ||+-------- EVEN PARITY GENARATION/CHECK
                                        ; ||          1 : EVEN
                                        ; ||          0 : ODD
                                        ; ++--------- NUMBER OF STOP BITS
                                        ;             00 : INVALID
                                        ;             01 : 1 BIT
                                        ;             10 : 1.5 BITS
                                        ;             11 : 2 BITS
                                        ;
                                        ; COMMAND SETUP
                                        ; 76543210
                                        ; |||||||+--- TRANSMIT ENABLE
                                        ; |||||||     1 : ENABLE
                                        ; |||||||     0 : DISABLE
                                        ; ||||||+---- DATA TERMINAL READY
                                        ; ||||||      "high" will force ~DTR output zero
                                        ; |||||+----- RECEIVE ENABLE
                                        ; |||||       1 : ENABLE
                                        ; |||||       0 : DISABLE
                                        ; ||||+------ SEND BREAK CHARACTER
                                        ; ||||        1 : forces TxD "low"
                                        ; ||||        0 : normal operation
                                        ; |||+------- ERROR RESET
                                        ; |||         1 : reset all error flags PE,OE,FE
                                        ; ||+-------- REQUEST TO SEND
                                        ; ||          "high" will force ~RTS output zero
                                        ; |+--------- INTERNAL RESET
                                        ; |           "high" returns USART to Mode Instruction Format
                                        ; +---------- ENTER HUNT MODE
                                        ;             1 = enable search for Sync Characters
CONTROL1:       EQU     30H
;
; CONSTANT ROM BASIC ROUTINE
;
ROM_BASPROMPT:  EQU     0081H
ROM_DSPCHR:     EQU     0257H
ROM_BEEP:       EQU     0D43H
ROM_TIMEREAD:   EQU     1602H
ROM_SYNERR:     EQU     3BDFH
ROM_MSGOUT:     EQU     52EDH
ROM_MON_UNEXT:  EQU     5C35H
ROM_DSPHEX4:    EQU     5EC0H
ROM_CMPHLDE:    EQU     5ED3H
ROM_DSPCRLF:    EQU     5FCAH
;
; CONSTANT ROM BASIC WORD AREA
;
PC8001RAM_TOP:  EQU     8000H
ROMBAS_WORKTOP: EQU     0EA00H
CURSOR_YPOS:    EQU     0EA63H
CURSOR_XPOS:    EQU     0EA64H
CTL1_OUTDATA:   EQU     0EA66H
BCD_TIME_SEC:   EQU     0EA76H
OUTPUT_DEVICE:  EQU     0EB49H
BASAREA_TOP:    EQU     0EB54H
VARAREA_TOP:    EQU     0EFA0H
ARRAYAREA_TOP:  EQU     0EFA2H
FREEAREA_TOP:   EQU     0EFA4H
CMD_ENTRY:      EQU     0F0FCH
;
; N-BASIC ROM FREE AREA1 : F216~F2FF
; N-BASIC ROM FREE AREA2 : FF3D~FFFF
; CONSTANT "CMD RCV" WORK AREA
;
MODEWORD:       EQU     0F216H
CTLWORD:        EQU     0F217H
LOADADRS:       EQU     0F218H
EXECADRS:       EQU     0F21AH
FLGLOADBAS:     EQU     0F21CH
TIMEUP:         EQU     0F21DH
CHKBLKNO:       EQU     0F21EH
RCVBUF_TOP:     EQU     0F22FH
RCVBUF_HEADER:  EQU     RCVBUF_TOP
RCVBUF_BLKNO:   EQU     RCVBUF_TOP +1
RCVBUF_CBLKNO:  EQU     RCVBUF_TOP +2
RCVBUF_DATA:    EQU     RCVBUF_TOP +3
RCVBUF_CHKSUM:  EQU     RCVBUF_DATA +128
RCVBUF_BOTTOM:  EQU     RCVBUF_CHKSUM
;
; CONSTANT "CMD RCV" VERSION DATA
;
VER_MAJOR:      EQU     1
VER_MINOR:      EQU     1
VER_REVISION:   EQU     0
;
                ;ORG    0H
CMD_START:
                CALL    PARSER_CMDPARAM
                JP      C,CMD_ERR_EXIT

                PUSH    HL
                CALL    INIT_SIO
                LD      A,1
                LD      (CHKBLKNO),A
CMD_L010:
                LD      A,NAK
                CALL    OUT_SIO
                JP      C,CMD_CANCEL
CMD_L020:
                CALL    IN_SIO
                JP      C,CMD_CANCEL
                CP      EOT
                JR      Z,CMD_030
                CP      SOH
                JR      Z,CMD_010
                LD      A,(TIMEUP)
                CP      1
                JR      Z,CMD_L010
                JR      CMD_L020
CMD_L030:
                CALL    IN_SIO
                JP      C,CMD_CANCEL
                CP      EOT
                JR      Z,CMD_030
                CP      SOH
                JR      Z,CMD_010
                LD      A,(TIMEUP)
                CP      1
                JP      Z,CMD_CANCEL2
                JR      CMD_L030
CMD_010:
                CALL    CLR_RCVBUF
                LD      (HL),A
                INC     HL
                LD      B,131
CMD_L040:
                CALL    IN_SIO
                JP      C,CMD_CANCEL
                PUSH    AF
                LD      A,(TIMEUP)
                CP      1
                JR      Z,CMD_CANCEL3
                POP     AF
                LD      (HL),A
                INC     HL
                DJNZ    CMD_L040
;
                CALL    DISABLE_RTS
                CALL    CHKDATA
                JR      C,CMD_020
                CALL    CHK_MEMOVER
                JR      C,CMD_CANCEL2
                LD      HL,RCVBUF_DATA
                LD      BC,128
                LDIR
                CALL    DISP_PROGRESS
                CALL    ENABLE_RTS
;
                LD      A,ACK
                CALL    OUT_SIO        
                JR      C,CMD_CANCEL
                LD      HL,CHKBLKNO
                INC     (HL)
                JR      CMD_L030
CMD_020:
                CALL    ENABLE_RTS
                LD      A,NAK
                CALL    OUT_SIO        
                JR      C,CMD_CANCEL
                JR      CMD_L030
CMD_030:
                LD      A,ACK
                CALL    OUT_SIO
                CALL    TERM_SIO
                POP     HL
                LD      A,(FLGLOADBAS)
                CP      1
                JR      Z,CMD_FIXBAS
                PUSH    HL
                LD      HL,0
                LD      DE,(EXECADRS)
                CALL    ROM_CMPHLDE     ;         Z CY
                                        ; HL<DE : 0  1
                                        ; HL=DE : 1  0
                                        ; HL>DE : 0  0
                POP     HL
                JR      Z,CMD_EXIT
                EX      DE,HL
                JP      (HL)
CMD_FIXBAS:
                PUSH    HL
                PUSH    DE
                LD      HL,(LOADADRS)
                EX      DE,HL
                SBC     HL,DE
                PUSH    HL
                POP     BC
                POP     HL
                XOR     A
                CPDR
                INC     HL
                INC     HL
                LD      (VARAREA_TOP),  HL
                LD      (ARRAYAREA_TOP),HL
                LD      (FREEAREA_TOP), HL
                POP     HL
                JR      CMD_EXIT
CMD_CANCEL3:
                POP     AF
CMD_CANCEL2:
                CALL    ROM_DSPCRLF
                XOR     A
                LD      (OUTPUT_DEVICE),A
                INC     A
                LD      (CURSOR_XPOS),A
                LD      HL,ERR_MSG
                CALL    ROM_MSGOUT
                CALL    ROM_BEEP
CMD_CANCEL:
                LD      A,CAN
                CALL    OUT_SIO
                CALL    TERM_SIO
                POP     HL
CMD_EXIT:
                RET
CMD_ERR_EXIT:
                POP     DE
                JP      ROM_SYNERR
;
;
;
PARSER_CMDPARAM:
                PUSH    HL
                LD      HL,0
                LD      (LOADADRS),HL
                LD      (EXECADRS),HL
                XOR     A
                LD      (FLGLOADBAS),A
                POP     HL
                LD      A,(HL)
                CP      'R'
                JP      NZ,SETCFRET
                INC     HL
                LD      A,(HL)
                CP      'C'
                JP      NZ,SETCFRET
                INC     HL
                LD      A,(HL)
                CP      'V'
                JP      NZ,SETCFRET
                CALL    PARSER_SKIP
                LD      DE,PARSER_CMDPARAM_010
                PUSH    DE
                CP      'E'
                JP      Z,PARSER_CMDPARAM_PE
                CP      'O'
                JP      Z,PARSER_CMDPARAM_PO
                CP      'N'
                JP      Z,PARSER_CMDPARAM_PN
                POP     DE
                JP      SETCFRET
PARSER_CMDPARAM_010:
                INC     HL
                LD      A,(HL)
                LD      DE,PARSER_CMDPARAM_020
                PUSH    DE
                CP      '8'
                JP      Z,PARSER_CMDPARAM_8BIT
                CP      '7'
                JP      Z,PARSER_CMDPARAM_7BIT
                POP     DE
                JP      SETCFRET
PARSER_CMDPARAM_020:
                INC     HL
                LD      A,(HL)
                LD      DE,PARSER_CMDPARAM_030
                PUSH    DE
                CP      '1'
                JP      Z,PARSER_CMDPARAM_SBIT1
                CP      '2'
                JP      Z,PARSER_CMDPARAM_SBIT1_5
                CP      '3'
                JP      Z,PARSER_CMDPARAM_SBIT2
                POP     DE
                JP      SETCFRET
PARSER_CMDPARAM_030:
                INC     HL
                LD      A,(HL)
                LD      DE,PARSER_CMDPARAM_040
                PUSH    DE
                CP      '0'
                JP      Z,PARSER_CMDPARAM_BRATEFAC64
                CP      '1'
                JP      Z,PARSER_CMDPARAM_BRATEFAC16
                POP     DE
                JP      SETCFRET
PARSER_CMDPARAM_040:
                CALL    PARSER_SKIP
                CP      0CH             ; ID CODE "&H"
                JP      Z,PARSER_CMDPARAM_LOADADRS
                CP      'B'
                JP      NZ,SETCFRET
                INC     HL
                LD      A,(HL)
                CP      'A'
                JP      NZ,SETCFRET
                INC     HL
                LD      A,(HL)
                CP      'S'
                JP      NZ,SETCFRET
                INC     HL
                LD      A,1
                LD      (FLGLOADBAS),A
                LD      DE,(BASAREA_TOP)
                LD      (LOADADRS),DE
                JR      PARSER_CMDPARAM_EXIT
PARSER_CMDPARAM_LOADADRS:
                INC     HL
                LD      E,(HL)
                INC     HL
                LD      D,(HL)
                INC     HL
                LD      (LOADADRS),DE
                LD      A,(HL)
                CP      0
                JR      Z,PARSER_CMDPARAM_EXIT
                CP      ':'
                JR      Z,PARSER_CMDPARAM_EXIT
                DEC     HL
                CALL    PARSER_SKIP
                CP      0CH             ; ID CODE "&H"
                JP      NZ,SETCFRET
PARSER_CMDPARAM_EXECADRS:
                INC     HL
                LD      E,(HL)
                INC     HL
                LD      D,(HL)
                INC     HL
                LD      (EXECADRS),DE
PARSER_CMDPARAM_EXIT:
                LD      DE,(LOADADRS)
                JR      CLRCFRET
PARSER_SKIP:
                INC     HL
                LD      A,(HL)
                CP      ' '
                JR      Z,PARSER_SKIP
                CP      ','
                JR      Z,PARSER_SKIP_010
                POP     DE
                JR      SETCFRET
PARSER_SKIP_010:
                INC     HL
                LD      A,(HL)
                CP      ' '
                JR      Z,PARSER_SKIP_010
                RET
PARSER_CMDPARAM_PE:
                LD      B,1
                LD      A,00110000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_PO:
                LD      B,1
                LD      A,00010000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_PN:
                LD      B,0
                LD      A,00000000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_8BIT:
                LD      A,B
                CP      1
                JP      Z,SETCFRET
                LD      A,(MODEWORD)
                OR      00001100B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_7BIT:
                LD      A,B
                CP      0
                JP      Z,SETCFRET
                LD      A,(MODEWORD)
                OR      00001000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_SBIT1:
                LD      A,(MODEWORD)
                OR      01000000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_SBIT1_5:
                LD      A,(MODEWORD)
                OR      10000000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_SBIT2:
                LD      A,(MODEWORD)
                OR      11000000B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_BRATEFAC64:
                LD      A,(MODEWORD)
                OR      00000011B
                LD      (MODEWORD),A
                RET
PARSER_CMDPARAM_BRATEFAC16:
                LD      A,(MODEWORD)
                OR      00000010B
                LD      (MODEWORD),A
                RET
CLRCFRET:
                SCF
                CCF
                RET
SETCFRET:
                SCF
                RET
;
;
;
CLR_RCVBUF:
                LD      HL,RCVBUF_TOP
                PUSH    AF
                PUSH    HL
                LD      B,RCVBUF_BOTTOM - RCVBUF_TOP + 1
                XOR     A
CLR_RCVBUF_L010:
                LD      (HL),A
                INC     HL
                DJNZ    CLR_RCVBUF_L010
                POP     HL
                POP     AF
                RET
;
;
;
CHK_MEMOVER:
                LD      BC,128
                PUSH    DE
                LD      HL,_PROG_TOP
                LD      DE,PC8001RAM_TOP
                CALL    ROM_CMPHLDE     ;         Z CY
                                        ; HL<DE : 0  1
                                        ; HL=DE : 1  0
                                        ; HL>DE : 0  0
                POP     DE
                JR      C,CHK_MEMOVER_010
                PUSH    DE
                EX      DE,HL
                ADD     HL,BC
                EX      DE,HL
                LD      HL,_PROG_TOP
                CALL    ROM_CMPHLDE     ;         Z CY
                                        ; HL<DE : 0  1
                                        ; HL=DE : 1  0
                                        ; HL>DE : 0  0
                POP     DE
                RET     C
CHK_MEMOVER_010:
                PUSH    DE
                EX      DE,HL
                ADD     HL,BC
                EX      DE,HL
                LD      HL,ROMBAS_WORKTOP
                CALL    ROM_CMPHLDE     ;         Z CY
                                        ; HL<DE : 0  1
                                        ; HL=DE : 1  0
                                        ; HL>DE : 0  0
                POP     DE
                RET
;
;
;
CHKDATA:
                LD      HL,RCVBUF_TOP
                LD      A,(HL)
                CP      SOH
                JR      NZ,SETCFRET
                LD      C,A
                INC     HL
                LD      A,(CHKBLKNO)
                CP      (HL)
                JR      NZ,SETCFRET
                ADD     A,C
                LD      C,A
                INC     HL
                LD      A,(CHKBLKNO)
                CPL
                CP      (HL)
                JR      NZ,SETCFRET
                ADD     A,C
                LD      C,A
                INC     HL
                LD      B,128
CHKDATA_L010:
                LD      A,(HL)
                ADD     A,C
                LD      C,A
                INC     HL
                DJNZ    CHKDATA_L010
                LD      A,C
                CP      (HL)
                JP      NZ,SETCFRET
                JP      CLRCFRET
;
;
;
DISP_PROGRESS:
                PUSH    DE
                PUSH    HL
                LD      A,1
                LD      (CURSOR_XPOS),A
                LD      A,'['
                CALL    ROM_DSPCHR
                LD      HL,(LOADADRS)
                CALL    ROM_DSPHEX4
                LD      A,'-'
                CALL    ROM_DSPCHR
                EX      DE,HL
                DEC     HL
                CALL    ROM_DSPHEX4
                LD      A,']'
                CALL    ROM_DSPCHR
                POP     HL
                POP     DE
                RET
;
;
;
INIT_SIO:
                XOR     A               ; DUMMY OUT
                OUT     (USARTCW),A
                OUT     (USARTCW),A
                OUT     (USARTCW),A
                LD      A,01000000B     ; SOFT RESET
                OUT     (USARTCW),A
                LD      A,(MODEWORD)    ; MODE SETUP
                OUT     (USARTCW),A
                LD      A,00010101B     ; COMMAND SETUP  TRANSMIT ENABLE
                                        ;                DATA TERMINAL READY : FALSE
                                        ;                RECEIVE ENABLE
                                        ;                ERROR RESET
                                        ;                REQUEST TO SEND : FALSE
                OUT     (USARTCW),A
                LD      (CTLWORD),A
                CALL    ENABLE_SIO
                CALL    ENABLE_DTR
                CALL    ENABLE_RTS
                RET
;
;
;
TERM_SIO:
                PUSH    DE
                CALL    ROM_TIMEREAD
                LD      A,(BCD_TIME_SEC)
                LD      D,A
TERM_SIO_010:
                PUSH    DE
                CALL    ROM_TIMEREAD
                POP     DE
                LD      A,(BCD_TIME_SEC)
                CP      D
                JR      Z,TERM_SIO_010
                CALL    DISABLE_RTS
                CALL    DISABLE_DTR
                CALL    DISABLE_SIO
                POP     DE
                RET
;
;
;
ENABLE_SIO:
                LD      A,(CTL1_OUTDATA)
                AND     11001111B       ; BS2,BS1 CLR
                OR      00100000B       ; BS2 ON
                OUT     (CONTROL1),A
                LD      (CTL1_OUTDATA),A
                RET
;
;
;
DISABLE_SIO:
                LD      A,(CTL1_OUTDATA)
                AND     11001111B       ; BS2,BS1 CLR
                OUT     (CONTROL1),A
                LD      (CTL1_OUTDATA),A
                RET
;
;
;
ENABLE_DTR:
                LD      A,(CTLWORD)
                OR      00000010B       ; DATA TERMINAL READY : TRUE
                OUT     (USARTCW),A
                RET
;
;
;
DISABLE_DTR:
                LD      A,(CTLWORD)
                AND     11111101B       ; DATA TERMINAL READY : FALSE
                OUT     (USARTCW),A
                RET
;
;
;
ENABLE_RTS:
                LD      A,(CTLWORD)
                OR      00100000B       ; REQUEST TO SEND : TRUE
                OUT     (USARTCW),A
                RET
;
;
;
DISABLE_RTS:
                LD      A,(CTLWORD)
                AND     11011111B       ; REQUEST TO SEND : FALSE
                OUT     (USARTCW),A
                RET
;
;
;
IN_SIO:
                XOR     A
                LD      (TIMEUP),A
                PUSH    BC
                PUSH    HL
                LD      HL,3            ; 5194msec Timer
                LD      BC,8000H
IN_SIO_010:
                IN      A,(KEYBRD9)
                AND     00000001B
                JP      Z,IN_SIO_030
                IN      A,(USARTCW)
                AND     00000010B
                JR      Z,IN_SIO_040
                IN      A,(USARTDW)
IN_SIO_020:
                POP     HL
                POP     BC
                JP      CLRCFRET
IN_SIO_030:
                POP     HL
                POP     BC
                JP      SETCFRET
IN_SIO_040:
                DEC     BC
                LD      A,C
                CP      0
                JR      NZ,IN_SIO_010
                LD      A,B
                CP      0
                JR      NZ,IN_SIO_010
                DEC     HL
                LD      A,L
                CP      0
                JR      NZ,IN_SIO_010
                LD      A,H
                CP      0
                JR      NZ,IN_SIO_010
                LD      A,1
                LD      (TIMEUP),A
                LD      A,0
                JR      IN_SIO_020
;
;
;
OUT_SIO:
                PUSH    AF
OUT_SIO1:
                IN      A,(KEYBRD9)
                AND     00000001B
                JP      Z,OUT_SIO2
                IN      A,(USARTCW)
                AND     00000001B
                JR      Z,OUT_SIO1
                POP     AF
                OUT     (USARTDW),A
                JP      CLRCFRET
OUT_SIO2:
                POP     AF
                JP      SETCFRET
;
ERR_MSG:        DB      "Abort Receive",0
                ;END

