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
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

# Determine the number of CPU cores
NPROC = $(shell nproc)

# Build the project
build: setup-build
	cd $(BUILD_DIR) && cmake --build . --config $(BUILD_TYPE) -j$(NPROC)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run the application
run: build
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
	@echo "  make          - Build the project (default)"
	@echo "  make setup-build - Generate build files without building"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make rebuild  - Clean and rebuild"
	@echo "  make run      - Build and run the application"
	@echo "  make install  - Install the application"
	@echo "  make compile_commands - Generate compile_commands.json for IDE integration"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=Debug|Release - Set build type (default: Debug)"

# Phony targets (targets that don't represent files)
.PHONY: all build clean run install rebuild help compile_commands setup-build
