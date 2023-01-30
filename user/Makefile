KERNEL_VER ?= `uname -r`
USER_EXE ?= monstars
SRC = monstars.c base64.c
OUT = ../build/$(KERNEL_VER)/$(USER_EXE)

release:
	gcc -s -Os $(SRC) -static -o $(OUT)

debug:
	gcc -DDEBUG -Og -g $(SRC) -static -o $(OUT)

