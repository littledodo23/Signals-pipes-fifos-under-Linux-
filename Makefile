# Makefile for Matrix Operations with Multi-Processing
# Author: Project 1 - Signals, Pipes, and FIFOs under Linux

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -fopenmp
LDFLAGS = -lm

# Directories
BUILD_DIR = build
SRC_DIR = .

# Target executable
TARGET = matrix_ops

# Source files
SRCS = main.c eigen.c worker_pool.c matrix.c file_io.c config.c

# Object files (placed in build directory)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Header files (for dependency tracking)
HEADERS = matrix.h worker_pool.h eigen.h config.h file_io.h

# Default target - build the executable
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Build successful! Executable: $(TARGET)"

# Compile each .c file to .o file in build directory
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Clean complete!"

# Clean and rebuild
rebuild: clean all

# Run the program
run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET)

# Run with custom config file
run-config: $(TARGET)
	@echo "Running $(TARGET) with custom config..."
	./$(TARGET) config.txt

# Run with custom matrix file
run-matrix: $(TARGET)
	./$(TARGET) config.txt matrix.txt

# Check for memory leaks with valgrind
valgrind: $(TARGET)
	@echo "Running valgrind memory check..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Debug with gdb
debug: $(TARGET)
	@echo "Starting gdb debugger..."
	gdb ./$(TARGET)

# Install (copy to /usr/local/bin - requires sudo)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete!"

# Uninstall
uninstall:
	@echo "Uninstalling $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete!"

# Show build info
info:
	@echo "Build Information:"
	@echo "  Compiler: $(CC)"
	@echo "  Flags: $(CFLAGS)"
	@echo "  Target: $(TARGET)"
	@echo "  Build Dir: $(BUILD_DIR)"
	@echo "  Sources: $(SRCS)"
	@echo "  Objects: $(OBJS)"

# Help target
help:
	@echo "Available targets:"
	@echo "  all         - Build the executable (default)"
	@echo "  clean       - Remove build artifacts"
	@echo "  rebuild     - Clean and rebuild"
	@echo "  run         - Build and run the program"
	@echo "  run-config  - Run with config file"
	@echo "  run-matrix  - Run with config and matrix file"
	@echo "  valgrind    - Run with valgrind memory checker"
	@echo "  debug       - Run with gdb debugger"
	@echo "  install     - Install to system (requires sudo)"
	@echo "  uninstall   - Remove from system (requires sudo)"
	@echo "  info        - Show build configuration"
	@echo "  help        - Show this help message"

# Phony targets (not actual files)
.PHONY: all clean rebuild run run-config run-matrix valgrind debug install uninstall info help
