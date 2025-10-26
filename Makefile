CC := gcc
CFLAGS := -Wall -Wextra -g
TARGET := solver
SRC := main.c .c .c

$(TARGET): $(SRC)
  $(CC) $(CFLAGS) -o $@ $(SRC)

.PHONY: all clean

all: solver

clean:
  rm -f $(TARGET)