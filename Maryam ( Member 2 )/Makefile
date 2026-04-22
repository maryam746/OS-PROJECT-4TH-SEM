# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -I/usr/local/include -D_DEFAULT_SOURCE

# Libraries
LIBS = -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# All C source files
SRC = $(wildcard *.c)

# Executable name
TARGET = main

# Default target
all: $(TARGET)

# Compile
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Clean up
clean:
	rm -f $(TARGET) railway_log.txt

# Run
run: $(TARGET)
	./$(TARGET)
