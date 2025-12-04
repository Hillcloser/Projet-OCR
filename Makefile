CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -g `pkg-config --cflags --libs gtk+-3.0` -lm
TARGET := build/ocr build/test
SRC := src/detect_cut.c src/solver.c src/image.c src/nn.c

.PHONY: all clean ocr test
	
ocr: $(SRC) src/graphic_interface.c build
	$(CC) -o build/ocr $(SRC) src/graphic_interface.c $(CFLAGS)
	./build/ocr
	
test: $(SRC) src/test.c build
	$(CC) -o build/test $(SRC) src/test.c $(CFLAGS)
	./build/test

build:
	mkdir build/
	
all: $(SRC) src/test.c src/graphic_interface.c build
	$(CC) -o ocr $(SRC) graphic_interface.c $(CFLAGS)
	$(CC) -o test $(SRC) test.c $(CFLAGS)
	echo $(TARGET)
	
clean:
	rm -rf build/