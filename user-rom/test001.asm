                ORG     06000H

_USER_ROM_START:
                DB      "AB"
_ENTRY:
                LD      HL,0E000H
                XOR     A
                LD      B,255
LOOP1:
                LD      (HL),A
                INC     HL
                INC     A
                DJNZ    LOOP1
                RET

_USER_ROM_END:
                DS      8192 - ( _USER_ROM_END - _USER_ROM_START ), 0FFH 

  	        END

