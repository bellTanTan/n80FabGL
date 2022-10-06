#!/bin/sh

HOME_FABGL=Arduino/libraries/FabGL
HOME_PATCH=Arduino/FabGL/n80FabGL/patch1.0.9

diff -u $HOME_FABGL/src/fabutils.h $HOME_PATCH/src/fabutils.h > $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/fabutils.cpp $HOME_PATCH/src/fabutils.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/inputbox.cpp $HOME_PATCH/src/inputbox.cpp >> $HOME_PATCH/Narya.patch
diff -u $HOME_FABGL/src/dispdrivers/vgabasecontroller.cpp $HOME_PATCH/src/dispdrivers/vgabasecontroller.cpp >> $HOME_PATCH/Narya.patch
