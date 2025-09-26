# Makefile for wayrandr Qt/QML project

# Directories
BUILD_DIR = build
SOURCE_DIR = .
QML_DIR = qml
SRC_DIR = src

# Build type (Debug or Release)
BUILD_TYPE ?= Debug

# Default target
all: build

# Create build directory and generate build files with CMake
setup-build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_DEBUG_LOGS=ON ..

# Determine the number of CPU cores
NPROC = $(shell nproc)

# Build the project
build: setup-build
	cd $(BUILD_DIR) && cmake --build . --config $(BUILD_TYPE) -j$(NPROC)

# Build with debug logs enabled
build-debug: 
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_DEBUG_LOGS=ON ..
	cd $(BUILD_DIR) && cmake --build . --config $(BUILD_TYPE) -j$(NPROC)

# Build without debug logs
build-nodebug: 
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_DEBUG_LOGS=OFF ..
	cd $(BUILD_DIR) && cmake --build . --config $(BUILD_TYPE) -j$(NPROC)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run the application (uses current build)
run: build
	cd $(BUILD_DIR) && ./wayrandr

# Run with debug logs enabled
run-debug: build-debug
	cd $(BUILD_DIR) && ./wayrandr

# Run without debug logs
run-nodebug: build-nodebug
	cd $(BUILD_DIR) && ./wayrandr

# Install the application
install: build
	cd $(BUILD_DIR) && cmake --install . --config $(BUILD_TYPE)

# Clean and rebuild
rebuild: clean build

# Generate compile_commands.json for code completion and linting tools
compile_commands:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	ln -sf $(BUILD_DIR)/compile_commands.json .

# Help target
help:
	@echo "Available targets:"
	@echo "  build         - Build the project with debug logs enabled (default)"
	@echo "  build-debug   - Build with debug logs enabled (same as build)"
	@echo "  build-nodebug - Build with debug logs disabled"
	@echo "  run           - Run the application with debug logs enabled (default)"
	@echo "  run-debug     - Build and run with debug logs enabled (same as run)"
	@echo "  run-nodebug   - Build and run without debug logs"
	@echo "  clean         - Remove build directory and artifacts"
	@echo "  rebuild       - Clean and rebuild the project"
	@echo "  install       - Install the application"
	@echo "  compile_commands - Generate compile_commands.json for tooling"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=Debug|Release - Set build type (default: Debug)"

# Phony targets (targets that don't represent files)
.PHONY: all build clean run install rebuild help compile_commands setup-build
