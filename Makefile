CC=cc
CFLAGS=-Wall -ggdb -g3 `sdl2-config --cflags` -Wno-unused-variable 
LDFLAGS=
LIBS=`sdl2-config --libs`  -lSDL2_mixer
OBJ=widget.o json.o var.o image.o 
TARGET=example

.SUFFIXES: .c.o
.c.o:
	$(CC) $(CFLAGS) -c $? -o $@

all: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET) $(LIBS)


clean:
	-rm $(OBJ) $(TARGET)
