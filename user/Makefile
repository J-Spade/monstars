KERNEL_VER ?= `uname -r`
EXE ?= monstars
SRC = monstars.c
OUT = ../build/$(EXE)-$(KERNEL_VER)

release:
	gcc -s -Os $(SRC) -static -o $(OUT)

debug:
	gcc -DDEBUG -Og -g $(SRC) -static -o $(OUT)

