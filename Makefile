# ====================================
# Matrix Operations Project Makefile
# Real-Time & Embedded Systems
# ====================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -fopenmp -std=gnu11
LDFLAGS = -lm -fopenmp

# Target executable
TARGET = matrix_operations

# Source files
SOURCES = main.c matrix.c worker_pool.c eigen.c config.c file_io.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = matrix.h worker_pool.h eigen.h config.h file_io.h

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "🔗 Linking $@..."
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✅ Build successful: $@"

# Compile source files
%.o: %.c $(HEADERS)
	@echo "🔨 Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	rm -f /tmp/matrix_status_fifo
	@echo "✅ Clean complete"

# Deep clean (including output files)
distclean: clean
	@echo "🧹 Deep cleaning..."
	rm -rf *.txt output_matrices/
	@echo "✅ Deep clean complete"

# Run the program
run: $(TARGET)
	@echo "🚀 Running $(TARGET)..."
	./$(TARGET)

# Run with config file
run-config: $(TARGET)
	@echo "🚀 Running $(TARGET) with config..."
	./$(TARGET) matrix_config.txt

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)
	@echo "🐛 Debug build complete"

# Optimized build
release: CFLAGS += -O3 -DNDEBUG
release: clean $(TARGET)
	@echo "🚀 Release build complete"

# Check for memory leaks with valgrind
valgrind: $(TARGET)
	@echo "🔍 Running valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Generate dependencies
depend: $(SOURCES)
	$(CC) -MM $(SOURCES) > .depend

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  run       - Build and run the program"
	@echo "  run-config - Build and run with config file"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  valgrind  - Run with memory leak detection"
	@echo "  help      - Show this help message"

.PHONY: all clean distclean run run-config debug release valgrind help depend

# Include dependencies if they exist
-include .depend
