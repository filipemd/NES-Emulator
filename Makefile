# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2 -std=c11 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lm -O2

# Directories
SRC_DIR = src
OBJ_DIR = build
RES_DIR = resources

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/mappers/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Output executable
TARGET = nes

# Create necessary subdirectories in the build directory
$(shell mkdir -p $(OBJ_DIR)/mappers)

# Default target
all: $(TARGET)

# Link the object files into the executable
$(TARGET): $(OBJ_FILES)
	$(CC) -o $(TARGET) $(OBJ_FILES) $(LDFLAGS)

# Compile the source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(SRC_DIR)/mappers -o $@ -c $<

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Install target (optional, if you want to install the executable)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall target to remove the executable
uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
