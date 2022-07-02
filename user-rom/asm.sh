#!/bin/sh
z80asm --input=test001.asm --output=test001.bin --list=test001.lst
z80asm --input=cmdrcv_2krom.asm --output=cmdrcv_2krom.bin --list=cmdrcv_2krom.lst
z80asm --input=cmdrcv_4krom.asm --output=cmdrcv_4krom.bin --list=cmdrcv_4krom.lst
z80asm --input=cmdrcv_8krom.asm --output=cmdrcv_8krom.bin --list=cmdrcv_8krom.lst
z80asm --input=cmdrcv_ram.asm --output=cmdrcv_ram.bin --list=cmdrcv_ram.lst
z80asm --input=chksum.asm --output=chksum.bin --list=chksum.lst
