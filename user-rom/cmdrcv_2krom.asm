                ORG     07800H
                ;ORG     06000H
                ;DS      8192 - 2048, 0FFH
_USER_ROM_START:
_PROG_TOP:
                CALL    _ENTRY
                JP      ROM_MON_UNEXT
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                DB      VER_MAJOR
                DB      VER_MINOR
                DB      VER_REVISION
_ENTRY:
                PUSH    BC
                PUSH    DE
                PUSH    HL
                LD      HL,NEWJPTBL
                LD      DE,CMD_ENTRY
                LD      BC,3
                LDIR
                POP     HL
                POP     DE
                POP     BC
                RET
NEWJPTBL:
                JP      NEW_CMD
NEW_CMD:
                INCLUDE "cmdrcv.asm"
;
;
;
_USER_ROM_END:
                DS      2048 - 4 - ( _USER_ROM_END - _USER_ROM_START ), 0FFH
;
                JP      _USER_ROM_START
                DB      "U"
                END

