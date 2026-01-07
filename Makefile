# Makefile for MLAI - Machine Learning from Scratch
# Security-hardened build configuration

CC = gcc
TARGET = mlai
SOURCES = main.c
HEADERS = base.h arena.h prng.h

# Security-hardened compiler flags
CFLAGS = -std=c11 \
         -Wall -Wextra -Wpedantic \
         -Wformat=2 \
         -Wformat-security \
         -Wnull-dereference \
         -Wstack-protector \
         -Wconversion \
         -Wsign-conversion \
         -D_FORTIFY_SOURCE=2 \
         -fstack-protector-strong \
         -fPIE \
         -fno-common

# Platform-specific security flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CFLAGS += -D__linux__
    LDFLAGS += -pie -Wl,-z,relro,-z,now
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -D__APPLE__
    LDFLAGS += -Wl,-pie
endif

# Math library
LDFLAGS += -lm

# Debug build (default)
debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG -march=native
release: $(TARGET)

# Sanitizer builds for development
asan: CFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g
asan: LDFLAGS += -fsanitize=address -fsanitize=undefined
asan: $(TARGET)

msan: CFLAGS += -fsanitize=memory -fno-omit-frame-pointer -g
msan: LDFLAGS += -fsanitize=memory
msan: $(TARGET)

# Main target
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Data preparation
.PHONY: data
data:
	@echo "Preparing MNIST dataset..."
	python3 mnist.py

# Run the program
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(TARGET) *.o *.out

# Clean everything including data files
.PHONY: cleanall
cleanall: clean
	rm -f *.mat

# Run with sanitizers for testing
.PHONY: test-asan
test-asan: asan
	./$(TARGET)

# Static analysis with cppcheck (if available)
.PHONY: analyze
analyze:
	@which cppcheck > /dev/null && cppcheck --enable=all --suppress=missingIncludeSystem $(SOURCES) || echo "cppcheck not found"

# Security audit
.PHONY: security-check
security-check:
	@echo "=== Security Checks ==="
	@echo "Checking for hardcoded secrets..."
	@grep -rn "password\|secret\|api_key\|token" --include="*.c" --include="*.h" . || echo "No obvious secrets found"
	@echo ""
	@echo "Checking for unsafe functions..."
	@grep -rn "gets\|strcpy\|strcat\|sprintf" --include="*.c" --include="*.h" . || echo "No obviously unsafe functions found"

# Help target
.PHONY: help
help:
	@echo "MLAI Build System"
	@echo ""
	@echo "Targets:"
	@echo "  debug         - Build with debug symbols (default)"
	@echo "  release       - Build optimized release version"
	@echo "  asan          - Build with AddressSanitizer"
	@echo "  msan          - Build with MemorySanitizer"
	@echo "  data          - Prepare MNIST dataset"
	@echo "  run           - Build and run the program"
	@echo "  clean         - Remove build artifacts"
	@echo "  cleanall      - Remove build artifacts and data files"
	@echo "  analyze       - Run static analysis"
	@echo "  security-check - Run security checks"
	@echo "  help          - Show this help message"

.DEFAULT_GOAL := debug
