#!/usr/bin/env bash

INC="-I../src/\
 -I../imgui/"
WRN="-Wall -Werror"
DBG="-ggdb"
OUT="../bin/GdbVkGui"
SRC="../src/main.c\
 ../src/WindowInterface.c\
 ../src/Vulkan/VulkanLayer.c\
 ../src/ProcessIO.c"
OBJ="main.o\
 WindowInterface.o\
 VulkanLayer.o\
 ProcessIO.o"
SRCPP="../src/Gui/GuiLayer.cpp\
 ../imgui/imgui_impl_vulkan.cpp\
 ../imgui/imgui_widgets.cpp\
 ../imgui/imgui_tables.cpp\
 ../imgui/imgui_draw.cpp\
 ../imgui/imgui_demo.cpp\
 ../imgui/imgui.cpp"
OBJPP="GuiLayer.o\
 imgui_impl_vulkan.o\
 imgui_widgets.o\
 imgui_tables.o\
 imgui_draw.o\
 imgui_demo.o\
 imgui.o"
LIB="-lm -ldl -lX11 -lxcb -lxcb-icccm -lxcb-keysyms -lxcb-xinput"
DEF="-D VK_NO_PROTOTYPES"

#g++ -Isrc/ -Wall -Werror -ggdb src/main.cpp src/ProcessIO.cpp -obin/GuiGdb
mkdir bin 2>/dev/null 1>&2
cd bin
gcc -c ${INC} ${WRN} ${DBG} ${DEF} ${SRC}
g++ -c ${INC} ${WRN} ${DBG} ${DEF} ${SRCPP}
g++ ${OBJ} ${OBJPP} ${LIB} -o${OUT}
cd ..
