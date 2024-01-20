INSTALLER_NAME ?= install_blanko
SRC = installer.c
PCH_INC_PATH = ../../../build/blanko/${KERNEL_VER}/
OUT = ../../../build/blanko/$(KERNEL_VER)/$(INSTALLER_NAME)

release:
	gcc -s -Os $(SRC) -static -I ${PCH_INC_PATH} -o $(OUT)

debug:
	gcc -DDEBUG -Og -g -gdwarf-2 $(SRC) -static -I ${PCH_INC_PATH} -o $(OUT)

