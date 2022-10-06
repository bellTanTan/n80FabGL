                ORG     0E400H
_PROG_TOP:
_MON_ENTRY:
                CALL    _ENTRY
                JP      ROM_BASPROMPT
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
                LD      HL,NEWJPTBL
                LD      DE,CMD_ENTRY
                LD      BC,3
                LDIR
                RET
NEWJPTBL:
                JP      NEW_CMD
NEW_CMD:
                INCLUDE "cmdrcv.asm"
                END

