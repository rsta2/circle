#
# Makefile
#

ifneq ($(OS),Windows_NT)

converttool: converttool.c
	@echo "  TOOL  $@"
	@gcc -o converttool converttool.c

clean:
	@echo "  CLEAN " `pwd`
	@rm -rf converttool

converttool.win: converttool.c
	x86_64-w64-mingw32-gcc -o converttool.win converttool.c
	chmod -x converttool.win

else

converttool.exe: converttool.win
	@echo "  COPY  $@"
	@cp converttool.win converttool.exe

clean:
	@echo "  CLEAN " `pwd`
	@rm -rf converttool.exe

endif
