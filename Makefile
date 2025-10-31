CC := gcc
CFLAGS := -Wall -Wextra -g -lm
TARGET := solver nn image
SRC := main.c solver.c image.c tibo.c 

$(TARGET): $(SRC)
  $(CC) $(CFLAGS) -o $@ $(SRC)

.PHONY: all clean 

all: $(TARGET)

clean:
  rm -f $(TARGET)