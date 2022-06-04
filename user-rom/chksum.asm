                ORG	06000H

ROM_DSPCHR:             EQU   00257H
ROM_BEEP:               EQU   00350H
ROM_WIDTH:              EQU   00843H
ROM_CONSOLE:            EQU   00884H
ROM_SCR2PRN:            EQU   0124AH
ROM_DSPMSG:             EQU   052EDH
ROM_MONERR:             EQU   05C5EH
ROM_HEX4IN:             EQU   05E21H
ROM_DSPHEX4:            EQU   05EC0H
ROM_DSPHEX2:            EQU   05EC5H
ROM_HLCPDE:             EQU   05ED3H
ROM_DSPCRLF:            EQU   05FCAH
ROM_DSPSPC:             EQU   05FD4H

ROW_CHKSUM:             EQU   0EE00H
TOTAL_CHKSUM:           EQU   0EE10H

_USER_ROM_START:
_ENTRY:
                LD      HL,T_WIDTH
                CALL    ROM_WIDTH
                LD      HL,T_CONSOLE
                CALL    ROM_CONSOLE
INPUT_ADD:
                LD      HL,T_INPUTMSG
                CALL    ROM_DSPMSG
                LD      HL,0
                LD      DE,0
                CALL    ROM_HEX4IN
                JR      Z,INPUT_ADD
                EX      DE,HL
                CALL    ROM_HEX4IN
                JR      NZ,INPUT_ADD
                CALL    ROM_HLCPDE
                JR      C,INPUT_ADD
                CALL    ROM_DSPCRLF
                EX      DE,HL
                LD      A,L
                AND     0F0H
                LD      L,A
                CALL    ROM_DSPSPC
                CALL    ROM_DSPCRLF
                PUSH    DE
                POP     IY
L_NEXTBLK:
                PUSH    HL
                LD      HL,ROW_CHKSUM
                XOR     A
                LD      B,17
L_CLRSUM:
                LD      (HL),A
                INC     HL
                DJNZ    L_CLRSUM
                LD      IX,TOTAL_CHKSUM
                CALL    ROM_DSPCRLF
                LD      HL,T_HEAD
                CALL    ROM_DSPMSG
                CALL    ROM_DSPCRLF
                LD      B,16
L_NEXTROW:
                POP     HL
                CALL    ROM_DSPHEX4
                LD      DE,ROW_CHKSUM
                CALL    ROM_DSPSPC
                PUSH    BC
                LD      B,16
L_DSPROW:
                LD      A,(HL)
                PUSH    AF
                CALL    ROM_DSPHEX2
                CALL    ROM_DSPSPC
                POP     AF
                PUSH    AF
                ADD     A,(IX+0)
                LD      (IX+0),A
                POP     AF
                EX      DE,HL
                ADD     A,(HL)
                LD      (HL),A
                EX      DE,HL
                INC     HL
                INC     E
                DJNZ    L_DSPROW
                PUSH    HL
                PUSH    BC
                LD      HL,T_SUM
                CALL    ROM_DSPMSG
                POP     BC
                LD      A,(IX+0)
                CALL    ROM_DSPHEX2
                CALL    ROM_DSPCRLF
                LD      (IX+0),0
                POP     HL
                PUSH    HL
                PUSH    IY
                POP     DE
                CALL    ROM_HLCPDE
                JR      NC,D_BTMSUM
                POP     HL
                POP     BC
                PUSH    HL
                DJNZ    L_NEXTROW
                POP     HL
                PUSH    DE
                PUSH    HL
D_BTMSUM:
                LD      B,56
L_SEPDSP:
                LD      HL,T_SEPARATE
                PUSH    BC
                CALL    ROM_DSPMSG
                POP     BC
                DJNZ    L_SEPDSP
                CALL    ROM_DSPCRLF
                LD      HL,T_BOTTOM
                CALL    ROM_DSPMSG
                LD      DE,ROW_CHKSUM
                LD      B,16
                LD      C,0
L_BTMSUM:
                LD      A,(DE)
                PUSH    AF
                CALL    ROM_DSPHEX2
                CALL    ROM_DSPSPC
                POP     AF
                ADD     A,C
                LD      C,A
                INC     E
                DJNZ    L_BTMSUM
                PUSH    BC
                LD      HL,T_SUM
                CALL    ROM_DSPMSG
                POP     BC
                LD      A,C
                CALL    ROM_DSPHEX2
                CALL    ROM_DSPCRLF
                CALL    ROM_SCR2PRN
                LD      A,12
                CALL    ROM_DSPCHR
                POP     HL
                POP     DE
                PUSH    IY
                POP     DE
                CALL    ROM_HLCPDE
                JP      C,L_NEXTBLK
                CALL    ROM_BEEP
                JP      _ENTRY

T_WIDTH:        DB      "80,25", 0
T_CONSOLE:      DB      "0,25,0,0", 0
T_INPUTMSG:     DB      "Start,End Add ? ", 0
T_HEAD:         DB      "Add  +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F Sum", 0
T_SUM:          DB      ":", 0
T_SEPARATE:     DB      "-", 0
T_BOTTOM:       DB      "Sum  ", 0

_USER_ROM_END:  DS      8192 - ( _USER_ROM_END - _USER_ROM_START ), 0FFH

  	        END

