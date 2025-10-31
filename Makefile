CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -g `pkg-config --cflags --libs gtk+-3.0` -lm
TARGET := detection image nn solver 
SRC := src/detection.c  src/image.c src/nn.c src/solver.c

.PHONY: all clean 

all: $(TARGET)

clean:
	rm -f $(TARGET)
detection: src/detection.c
	$(CC) $(CFLAGS) -o $@ src/detection.c
image: src/image.c
	$(CC) $(CFLAGS) -o $@ src/image.c
solver: src/solver.c
	$(CC) $(CFLAGS) -o $@ src/solver.c
nn: src/nn.c
	$(CC) $(CFLAGS) -o $@ src/nn.c
