INSTALLER_NAME ?= install_blanko
SRC = installer.c
PCH_INC_REL = ../../../_build/blanko/${KERNEL_VER}/release/
PCH_INC_DBG = ../../../_build/blanko/${KERNEL_VER}/debug/
OUT_REL = ../../../_export/blanko/$(KERNEL_VER)/release/$(INSTALLER_NAME)
OUT_DBG = ../../../_export/blanko/$(KERNEL_VER)/debug/$(INSTALLER_NAME)

release:
	gcc -s -Os $(SRC) -static -I ${PCH_INC_REL} -o $(OUT_REL)

debug:
	gcc -DDEBUG -Og -g -gdwarf-2 $(SRC) -static -I ${PCH_INC_DBG} -o $(OUT_DBG)

