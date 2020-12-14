#!/usr/bin/env bash

INC="-Isrc/"
WRN="-Wall -Werror"
DBG="-ggdb"
OUT="bin/GdbVkGui"
SRC="src/main.cpp\
 src/ProcessIO.cpp"

#g++ -Isrc/ -Wall -Werror -ggdb src/main.cpp src/ProcessIO.cpp -obin/GuiGdb
g++ ${INC} ${WRN} ${DBG} ${SRC} -o${OUT}
