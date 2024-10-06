# Compiler and flags
CC ?= gcc
ASM = nasm
CFLAGS = -Wall -Wextra -Wpedantic -g -I./include -I./tests -fPIC
ASMFLAGS = -f elf64 -DPIC -g
LDFLAGS = -shared -fPIC
EXEC_LDFLAGS = -no-pie -lm
PREFIX ?= /usr/local

# Source files
MAIN_SRC = examples/main.c
BENCH_SRC = tests/bench_secureinput.c
ASM_SRC = lib/secureinput.asm
C_SRC = lib/secureinput.c
TEST_CONST_TIME_SRC = tests/test_const_time.c

# Object files
MAIN_OBJ = $(MAIN_SRC:.c=.o)
BENCH_OBJ = $(BENCH_SRC:.c=.o)
ASM_OBJ = $(ASM_SRC:.asm=_asm.o)
C_OBJ = $(C_SRC:.c=.o)
TEST_CONST_TIME_OBJ = $(TEST_CONST_TIME_SRC:.c=.o)

# Executables
MAIN_EXEC = secureinput
BENCH_EXEC = bench_secureinput
TEST_CONST_TIME_EXEC = test_const_time

# Library
LIB_NAME = libsecureinput.so
LIB_STATIC = libsecureinput.a

# Default target
all: $(MAIN_EXEC) $(BENCH_EXEC) $(TEST_CONST_TIME_EXEC)

# Main program
$(MAIN_EXEC): $(C_OBJ) $(ASM_OBJ) $(MAIN_OBJ)
	$(CC) $^ -o $@ $(EXEC_LDFLAGS)

# Benchmark program
$(BENCH_EXEC): $(C_OBJ) $(ASM_OBJ) $(BENCH_OBJ)
	$(CC) $^ -o $@ $(EXEC_LDFLAGS)

# Constant-time test program
$(TEST_CONST_TIME_EXEC): $(C_OBJ) $(ASM_OBJ) $(TEST_CONST_TIME_OBJ)
	$(CC) $^ -o $@ $(EXEC_LDFLAGS) -lm

# Help target
.PHONY: help
help:
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all         : Build main program, benchmark, and constant-time test (default)"
	@echo "  bench       : Build benchmark program"
	@echo "  lib         : Build shared library"
	@echo "  static-lib  : Build static library"
	@echo "  install     : Install shared library and header"
	@echo "  clean       : Remove object files and executables"
	@echo "  help        : Display this help message"
	@echo "  test_const_time : Build constant-time test"
	@echo "  run_test_const_time : Build and run constant-time test"
	@echo ""
	@echo "Options:"
	@echo "  CC=compiler : Choose compiler (gcc or clang)"
	@echo "  DEBUG=1     : Enable debug mode"
	@echo "  SANITIZE=1  : Enable AddressSanitizer"
	@echo "  OPTIMIZE=1  : Enable optimizations"
	@echo ""
	@echo "Example build commands:"
	@echo "  1. Build main program, benchmark, and constant-time test with default settings:"
	@echo "     make"
	@echo ""
	@echo "  2. Build benchmark program with clang and optimizations:"
	@echo "     make bench CC=clang OPTIMIZE=1"
	@echo ""
	@echo "  3. Build shared library with debug symbols and AddressSanitizer:"
	@echo "     make lib DEBUG=1 SANITIZE=1"
	@echo ""
	@echo "  4. Build static library with all options enabled:"
	@echo "     make static-lib CC=clang DEBUG=1 SANITIZE=1 OPTIMIZE=1"
	@echo ""
	@echo "  5. Clean and rebuild main program and benchmark with gcc and optimizations:"
	@echo "     make clean && make OPTIMIZE=1"
	@echo ""
	@echo "  6. Install the shared library:"
	@echo "     sudo make install"
	@echo ""
	@echo "  7. Run constant-time test:"
	@echo "     make run_test_const_time"
	@echo ""
	@echo "Note: You can combine options as needed for your specific build requirements."


# Shared library
$(LIB_NAME): $(ASM_OBJ) $(C_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Static library
$(LIB_STATIC): $(ASM_OBJ) $(C_OBJ)
	ar rcs $@ $^

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM source files
%_asm.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Debug mode
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
ASMFLAGS += -DDEBUG
endif

# Sanitizer
ifeq ($(SANITIZE),1)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

# Optimization
ifeq ($(OPTIMIZE),1)
CFLAGS += -O3 -march=native
endif

# Targets
.PHONY: bench lib static-lib install clean run_test_const_time

bench: $(BENCH_EXEC)

lib: $(LIB_NAME)

static-lib: $(LIB_STATIC)

install: $(LIB_NAME)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 755 $(LIB_NAME) $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 644 include/secureinput.h $(DESTDIR)$(PREFIX)/include
	ldconfig

clean:
	rm -f $(MAIN_EXEC) $(BENCH_EXEC) $(TEST_CONST_TIME_EXEC) $(LIB_NAME) $(LIB_STATIC) **/*.o

run_test_const_time: $(TEST_CONST_TIME_EXEC)
	./$(TEST_CONST_TIME_EXEC)

# Prevent make from deleting intermediate files
.SECONDARY:
