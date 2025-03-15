INSTALLER_NAME ?= bang-installer
SRC = installer.c
PCH_INC_REL = ../../../../_build/bang/pam/release/
PCH_INC_DBG = ../../../../_build/bang/pam/debug/
OUT_REL = ../../../../_export/bang/pam/release/$(INSTALLER_NAME)
OUT_DBG = ../../../../_export/bang/pam/debug/$(INSTALLER_NAME)

release:
	gcc -s -Os $(SRC) -static -I ${PCH_INC_REL} -o $(OUT_REL)

debug:
	gcc -DDEBUG -Og -g -gdwarf-2 $(SRC) -static -I ${PCH_INC_DBG} -o $(OUT_DBG)

