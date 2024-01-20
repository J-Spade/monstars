USER_EXE_NAME ?= blanko
SRC = blanko.c base64.c
OUT = ../../../build/blanko/$(KERNEL_VER)/$(USER_EXE_NAME)

release:
	gcc -s -Os $(SRC) -static -o $(OUT)

debug:
	gcc -DDEBUG -Og -g -gdwarf-2 $(SRC) -static -o $(OUT)

