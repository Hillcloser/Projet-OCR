CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -g `pkg-config --cflags --libs gtk+-3.0` -lm
TARGET := build/ocr build/test
SRC := src/detect_cut.c src/ src/image.h src/solver.c src/detection.c src/image.c src/nn.c

.PHONY: all clean ocr test
	
ocr: $(SRC) src/graphic_interface.c
	mkdir build/
	$(CC) $(CFLAGS) -o build/$@ $(SRC) src/graphic_interface.c
	./build/ocr
	
test: $(SRC) src/test.c
	mkdir build/
	$(CC) $(CFLAGS) -o build/$@ $(SRC) src/test.c
	./build/test

all: 
	mkdir build/
	$(CC) $(CFLAGS) -o $@ $(SRC) graphic_interface.c
	$(CC) $(CFLAGS) -o $@ $(SRC) test.c
	echo $(TARGET)
	
clean:
	rm -rf build/