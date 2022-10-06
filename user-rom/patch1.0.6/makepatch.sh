#!/bin/sh

HOME_FABGL=Arduino/libraries/FabGL
HOME_PATCH=Arduino/FabGL/n80FabGL/patch1.0.6

diff -u $HOME_FABGL/src/emudevs/Z80.h $HOME_PATCH/src/emudevs/Z80.h > $HOME_PATCH/Z80.patch
diff -u $HOME_FABGL/src/emudevs/Z80.cpp $HOME_PATCH/src/emudevs/Z80.cpp >> $HOME_PATCH/Z80.patch

diff -u $HOME_FABGL/src/fabutils.h $HOME_PATCH/src/fabutils.h.kbdjp > $HOME_PATCH/kbdlayouts.patch
diff -u $HOME_FABGL/src/devdrivers/kbdlayouts.h $HOME_PATCH/src/devdrivers/kbdlayouts.h >> $HOME_PATCH/kbdlayouts.patch
diff -u $HOME_FABGL/src/devdrivers/kbdlayouts.cpp $HOME_PATCH/src/devdrivers/kbdlayouts.cpp >> $HOME_PATCH/kbdlayouts.patch

diff -u $HOME_FABGL/src/emudevs/Z80.h $HOME_PATCH/src/emudevs/Z80.h > $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/emudevs/Z80.cpp $HOME_PATCH/src/emudevs/Z80.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/devdrivers/kbdlayouts.h $HOME_PATCH/src/devdrivers/kbdlayouts.h >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/devdrivers/kbdlayouts.cpp $HOME_PATCH/src/devdrivers/kbdlayouts.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/fabutils.h $HOME_PATCH/src/fabutils.h.sd >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/fabutils.cpp $HOME_PATCH/src/fabutils.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/inputbox.cpp $HOME_PATCH/src/inputbox.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/dispdrivers/vgabasecontroller.cpp $HOME_PATCH/src/dispdrivers/vgabasecontroller.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/emudevs/i8042.cpp $HOME_PATCH/src/emudevs/i8042.cpp >> $HOME_PATCH/Narya.patch
