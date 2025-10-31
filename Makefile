CC := gcc
CFLAGS := -Wall -Wextra -g `pkg-config --cflags --libs gtk+-3.0` -lm
TARGET := detection image nn solver 
SRC := detection.c  image.c nn.c solver.c

.PHONY: all clean 

all: $(TARGET)

clean:
	rm -f $(TARGET)
detection: detection.c
	$(CC) $(CFLAGS) -o $@ detection.c
image: image.c
	$(CC) $(CFLAGS) -o $@ image.c
solver: solver.c
	$(CC) $(CFLAGS) -o $@ solver.c
nn: nn.c
	$(CC) $(CFLAGS) -o $@ nn.c
