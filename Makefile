all:
	gcc main.c -lSDL2 -lGLESv2 -lm -o circles -I ./cglm/include/
