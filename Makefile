# Interactive Tester Makefile

BUILD_DIR ?= build
DEBUG_DIR ?= build-debug
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic

.PHONY: all compile debug format format-check test-fixtures test test-timeout install uninstall clean help

all: compile

## Build targets
compile:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) --parallel

debug:
	cmake -S . -B $(DEBUG_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(DEBUG_DIR) --parallel

## Code formatting
format:
	@echo "Formatting C++ files with clang-format..."
	clang-format --style=file -i src/*.cpp include/*.hpp

format-check:
	@echo "Checking formatting with clang-format..."
	clang-format --style=file --dry-run --Werror src/*.cpp include/*.hpp

## Testing targets
test-fixtures: $(DEBUG_DIR)
	@echo "Compiling test fixtures into $(DEBUG_DIR)..."
	$(CXX) $(CXXFLAGS) test/echo_interactor.cpp -o $(DEBUG_DIR)/echo_interactor
	$(CXX) $(CXXFLAGS) test/guess_solution.cpp -o $(DEBUG_DIR)/guess_solution
	$(CXX) $(CXXFLAGS) test/hang_solution.cpp -o $(DEBUG_DIR)/hang_solution

test: debug test-fixtures
	@echo "Executing interaction test..."
	$(DEBUG_DIR)/interactive_tester $(DEBUG_DIR)/guess_solution $(DEBUG_DIR)/echo_interactor -v

test-timeout: debug test-fixtures
	@echo "Executing timeout test (expected TLE exit code 124)..."
	-$(DEBUG_DIR)/interactive_tester $(DEBUG_DIR)/hang_solution $(DEBUG_DIR)/echo_interactor -t 100 -v

## Installation targets
install: compile
	@echo "Installing interactive_tester using CMake..."
	cmake --install $(BUILD_DIR) --prefix $(PREFIX)

uninstall:
	@echo "Uninstalling interactive_tester from $(DESTDIR)$(BINDIR)..."
	rm -f $(DESTDIR)$(BINDIR)/interactive_tester

## Cleaning
clean:
	@echo "Cleaning build directories and logs..."
	rm -rf $(BUILD_DIR) $(DEBUG_DIR) *.log

## Help
help:
	@echo "Interactive Tester Makefile targets:"
	@echo "  make compile       - Build Release configuration in $(BUILD_DIR)"
	@echo "  make debug         - Build Debug configuration in $(DEBUG_DIR)"
	@echo "  make format        - In-place format with clang-format"
	@echo "  make format-check  - Verify formatting without modifying files"
	@echo "  make test-fixtures - Compile test programs in test/ directory"
	@echo "  make test          - Build and run standard interactor integration test"
	@echo "  make test-timeout  - Build and run timeout test (-t 100)"
	@echo "  make install       - Install binary using CMake (supports PREFIX & DESTDIR)"
	@echo "  make uninstall     - Remove binary from $(PREFIX)/bin"
	@echo "  make clean         - Remove build directories and logs"
	@echo "  make help          - Show this help message"
