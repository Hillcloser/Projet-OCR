CC := gcc
CFLAGS := -Wall -Wextra -g `pkg-config --cflags --libs gtk+-3.0` -lm
TARGET := solver nn image
SRC := solver.c image.c tibo.c 

.PHONY: all clean 

all: $(TARGET)

clean:
	rm -f $(TARGET)

solver: solver.c
	$(CC) $(CFLAGS) -o $@ solver.c
image: image.c
	$(CC) $(CFLAGS) -o $@ image.c
nn: tibo.c
	$(CC) $(CFLAGS) -o $@ tibo.c
