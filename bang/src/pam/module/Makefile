SO_NAME ?= pam_bang.so
SRC = pam_mod.c
OUT_REL = ../../../../_export/bang/pam/release
OUT_DBG = ../../../../_export/bang/pam/debug

release:
	mkdir -p $(OUT_REL)
	gcc -s -Os -fPIC -fno-stack-protector --shared $(SRC) -o $(OUT_REL)/$(SO_NAME)

debug:
	mkdir -p $(OUT_DBG)
	gcc -DDEBUG -Og -g -gdwarf-2 -fPIC -fno-stack-protector --shared $(SRC) -o $(OUT_DBG)/$(SO_NAME)
