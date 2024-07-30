#
# Makefile
#

ifneq ($(OS),Windows_NT)

all: cflashy converttool

cflashy: cflashy.c
	@echo "  TOOL  $@"
	@gcc -o cflashy cflashy.c

converttool: converttool.c
	@echo "  TOOL  $@"
	@gcc -o converttool converttool.c

clean:
	@echo "  CLEAN " `pwd`
	@rm -rf converttool cflashy

cflashy.win: cflashy.c
	x86_64-w64-mingw32-gcc -o cflashy.win cflashy.c
	chmod -x cflashy.win

converttool.win: converttool.c
	x86_64-w64-mingw32-gcc -o converttool.win converttool.c
	chmod -x converttool.win

else

all: cflashy.exe converttool.exe

cflashy.exe: cflashy.win
	@echo "  COPY  $@"
	@cp cflashy.win cflashy.exe

converttool.exe: converttool.win
	@echo "  COPY  $@"
	@cp converttool.win converttool.exe

clean:
	@echo "  CLEAN " `pwd`
	@rm -rf cflashy.exe converttool.exe

endif
