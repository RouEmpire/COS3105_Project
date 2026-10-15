CC = clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
ROGUEUTIL_DIR = ../../lib/rogueutil-1.0
TARGET = kanban-tui

.PHONY: all run clean

all: $(TARGET)

$(TARGET): main.c $(ROGUEUTIL_DIR)/rogueutil.h
	$(CC) $(CFLAGS) -I$(ROGUEUTIL_DIR) -o $@ main.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
