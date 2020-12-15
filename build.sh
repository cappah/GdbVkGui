#!/usr/bin/env bash

INC="-Isrc/"
WRN="-Wall -Werror"
DBG="-ggdb"
OUT="bin/GdbVkGui"
SRC="src/main.c\
 src/WindowInterface.c\
 src/ProcessIO.c"
LIB="-lm -ldl -lX11 -lxcb -lxcb-icccm -lxcb-keysyms -lxcb-xinput"

#g++ -Isrc/ -Wall -Werror -ggdb src/main.cpp src/ProcessIO.cpp -obin/GuiGdb
gcc ${INC} ${WRN} ${DBG} ${SRC} ${LIB} -o${OUT}
