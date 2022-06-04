#!/bin/sh
z80asm --input=test001.asm --output=test001.bin --list=test001.lst
z80asm --input=chksum.asm --output=chksum.bin --list=chksum.lst
