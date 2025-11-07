# Makefile for Matrix Operations Project
# Real-Time & Embedded Systems - Birzeit University

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fopenmp -g
LDFLAGS = -lm -fopenmp

# Source files
SRCS = main.c matrix.c worker_pool.c eigen.c file_io.c config.c
OBJS = $(SRCS:.c=.o)
TARGET = matrix_ops

# Build target
all: $(TARGET)
	@echo "✅ Build complete!"
	@echo "Run with: ./$(TARGET) [config_file] [matrix_file]"

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
main.o: main.c matrix.h worker_pool.h eigen.h config.h file_io.h
matrix.o: matrix.c matrix.h
worker_pool.o: worker_pool.c worker_pool.h matrix.h
eigen.o: eigen.c eigen.h matrix.h
file_io.o: file_io.c file_io.h matrix.h
config.o: config.c config.h

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: clean all

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f /tmp/matrix_status_fifo
	@echo "✅ Cleaned build artifacts"

# Run with default config
run: $(TARGET)
	./$(TARGET) matrix_config.txt

# Run with valgrind memory check
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Create sample config file
config:
	@echo "4 60" > matrix_config.txt
	@echo "matrices" >> matrix_config.txt
	@echo "✅ Created matrix_config.txt"

# Create sample matrices folder
matrices:
	mkdir -p matrices
	@echo "A 3 3" > matrices/A.txt
	@echo "1.0 2.0 3.0" >> matrices/A.txt
	@echo "4.0 5.0 6.0" >> matrices/A.txt
	@echo "7.0 8.0 9.0" >> matrices/A.txt
	@echo "B 3 3" > matrices/B.txt
	@echo "9.0 8.0 7.0" >> matrices/B.txt
	@echo "6.0 5.0 4.0" >> matrices/B.txt
	@echo "3.0 2.0 1.0" >> matrices/B.txt
	@echo "✅ Created sample matrices in matrices/"

# Setup everything
setup: config matrices
	@echo "✅ Setup complete! Ready to compile and run."

# Help target
help:
	@echo "Matrix Operations Project - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run with default config"
	@echo "  valgrind  - Run with memory leak detection"
	@echo "  config    - Create sample config file"
	@echo "  matrices  - Create sample matrices folder"
	@echo "  setup     - Create config and sample matrices"
	@echo "  help      - Show this help message"

.PHONY: all debug clean run valgrind config matrices setup help
