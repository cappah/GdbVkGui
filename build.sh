#!/usr/bin/env bash

# get the current directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/"

INC="-I${DIR}src/\
 -I${DIR}lua-5.4.2/src/\
 -I${DIR}imgui/"
WRN="-Wall -Werror"
DBG="-ggdb"
OUT="${DIR}bin/GdbVkGui"

SRC="${DIR}src/main.c\
 ${DIR}src/WindowInterface.c\
 ${DIR}src/Vulkan/VulkanLayer.c\
 ${DIR}src/LuaLayer.c\
 ${DIR}src/tlsf.c\
 ${DIR}src/ProcessIO.c"
OBJ="${DIR}bin/main.o\
 ${DIR}bin/WindowInterface.o\
 ${DIR}bin/VulkanLayer.o\
 ${DIR}bin/ProcessIO.o\
 ${DIR}bin/LuaLayer.o\
 ${DIR}bin/tlsf.o"

SRCPP="${DIR}src/Gui/GuiLayer.cpp\
 ${DIR}src/Gui/ImguiToLua.cpp\
 ${DIR}src/Frontend/GdbFE.cpp"
SRCPP2="${DIR}imgui/imgui_impl_vulkan.cpp\
 ${DIR}imgui/imgui_widgets.cpp\
 ${DIR}imgui/imgui_tables.cpp\
 ${DIR}imgui/imgui_draw.cpp\
 ${DIR}imgui/imgui_demo.cpp\
 ${DIR}imgui/imgui.cpp\
 ${DIR}src/Frontend/ImGuiFileBrowser.cpp\
 ${DIR}src/Frontend/TextEditor.cpp"
OBJPP="${DIR}bin/GuiLayer.o\
 ${DIR}bin/ImguiToLua.o\
 ${DIR}bin/TextEditor.o\
 ${DIR}bin/ImGuiFileBrowser.o\
 ${DIR}bin/GdbFE.o\
 ${DIR}bin/imgui_impl_vulkan.o\
 ${DIR}bin/imgui_widgets.o\
 ${DIR}bin/imgui_tables.o\
 ${DIR}bin/imgui_draw.o\
 ${DIR}bin/imgui_demo.o\
 ${DIR}bin/imgui.o"

ILIB="-L${DIR}lua-5.4.2/src/"
LIB="-lm -ldl -lX11 -lxcb -lxcb-icccm -lxcb-keysyms -lxcb-xinput -llua"
DEF="-DVK_NO_PROTOTYPES -DROOT_DIR=\"${DIR}\""

# build lua if necessary
if [[ ! -f "lua-5.4.2/src/liblua.a" ]]; then
	cd ${DIR}lua-5.4.2
	make linux
	cd ${DIR}
fi

mkdir bin 2>/dev/null 1>&2
cd ${DIR}bin 
echo "gcc -c ${INC} ${WRN} ${DBG} ${DEF} ${SRC}"
gcc -c ${INC} ${WRN} ${DBG} ${DEF} ${SRC}
echo "g++ -c ${INC} ${WRN} ${DBG} ${DEF} ${SRCPP}"
g++ -c ${INC} ${WRN} ${DBG} ${DEF} ${SRCPP}

# restrict because compiling these can be unreasonably slow
if [[ ! -f "${DIR}/bin/imgui.o" ]]; then
	echo "g++ -c ${INC} ${DBG} ${DEF} ${SRCPP2}"
	g++ -c ${INC} ${DBG} ${DEF} ${SRCPP2}
fi
echo "g++ ${OBJ} ${OBJPP} ${LIB} -o${OUT}"
g++ ${OBJ} ${OBJPP} ${ILIB} ${LIB} -o${OUT}
cd ${DIR}

echo 
echo Done.
