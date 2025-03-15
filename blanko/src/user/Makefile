USER_EXE_NAME ?= blanko
SRC = blanko.c base64/base64.c
OUT_REL = ../../../_build/blanko/$(KERNEL_VER)/release/$(USER_EXE_NAME)
OUT_DBG = ../../../_build/blanko/$(KERNEL_VER)/debug/$(USER_EXE_NAME)

release:
	gcc -s -Os $(SRC) -static -o $(OUT_REL)

debug:
	gcc -DDEBUG -Og -g -gdwarf-2 $(SRC) -static -o $(OUT_DBG)

