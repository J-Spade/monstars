KERNEL_VER ?= `uname -r`
INSTALLER ?= go-monstars
SRC = installer.c
OUT = ../build/$(KERNEL_VER)/$(INSTALLER)

KO_PATH = ../build/$(KERNEL_VER)/ko_precompiled.h
USR_PATH = ../build/$(KERNEL_VER)/user_precompiled.h

release:
	gcc -s -Os $(SRC) -static -include ${KO_PATH} -include ${USR_PATH} -o $(OUT)

debug:
	gcc -DDEBUG -Og -g -gdwarf-2 $(SRC) -static -include ${KO_PATH} -include ${USR_PATH} -o $(OUT)

