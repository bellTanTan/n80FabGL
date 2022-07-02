                ORG     06000H
_USER_ROM_START:
_PROG_TOP:
                DB      "AB"
                JP      _ENTRY
_MON_ENTRY:
                CALL    _ENTRY
                JP      ROM_BASPROMPT
                NOP
                NOP
                DB      VER_MAJOR
                DB      VER_MINOR
                DB      VER_REVISION
_ENTRY:
                LD      HL,NEWJPTBL
                LD      DE,CMD_ENTRY
                LD      BC,3
                LDIR
                RET
NEWJPTBL:
                JP      NEW_CMD
NEW_CMD:
                INCLUDE "cmdrcv.asm"
;
;
;
_USER_ROM_END:
                DS      4096 - ( _USER_ROM_END - _USER_ROM_START ), 0FFH
                END

