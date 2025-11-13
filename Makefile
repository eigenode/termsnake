CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = tsnake
SRC = termsnake.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

