OBJS = main.o  trial.o  trials.o
EYELINK_LIB=-L../../../lib -L/usr/lib
EYELINK_INCLUDE=-I../../../include -I/usr/include
LINKLIBS=$(EYELINK_LIB) -leyelink_core_graphics  -leyelink_core -lsdl_util  -lSDL_ttf -lSDL_image -lSDL_gfx -lSDL_mixer -lSDL  -lrt
CFLAGS=$(EYELINK_INCLUDE) `sdl-config --cflags`
gcwindow: $(OBJS)
	$(CC) -no-pie -o gcwindow $(OBJS) $(LINKLIBS)
clean:
	-rm -rf $(OBJS) gcwindow
