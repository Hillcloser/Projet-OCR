CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -g `pkg-config --cflags --libs gtk+-3.0` -lm
TARGET := build/detection build/image build/nn build/solver 
SRC := src/detection.c  src/image.c src/nn.c src/solver.c

.PHONY: all clean 

all: $(TARGET)

build/clean:
	rm -f $(TARGET)
build/detection: src/detection.c
	$(CC) $(CFLAGS) -o $@ src/detection.c
build/image: src/image.c
	$(CC) $(CFLAGS) -o $@ src/image.c
build/solver: src/solver.c
	$(CC) $(CFLAGS) -o $@ src/solver.c
build/nn: src/nn.c
	$(CC) $(CFLAGS) -o $@ src/nn.c
