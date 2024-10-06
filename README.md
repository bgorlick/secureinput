# SecureInput

SecureInput: A C and ASM secure password and input library with constant-time, side-channel resistant features supporting arbitrary terminal input handling

## Features

- Constant-time operations to prevent timing attacks
- Hybrid C and x86-64 Assembly implementation for optimal performance
- Secure memory allocation and wiping
- TTY state management for echo disabling/enabling
- Cross-platform support (Linux, potentially extendable to other UNIX-like systems)

## Installation

### Prerequisites

- GCC or Clang compiler
- NASM (Netwide Assembler)
- Make
- (Optional) Meson build system and Ninja

### Building from Source

#### Using Make (Primary method)

1. Clone the repository:
   ```
   git clone https://github.com/bgorlick/secureinput.git
   cd secureinput
   ```

2. Build the library and all components:
   ```
   make
   ```

   This will build the main program, benchmark, and constant-time test.

3. (Optional) Install the library system-wide:
   ```
   sudo make install
   ```

4. Additional build options:
   - Build only the benchmark program:
     ```
     make bench
     ```
   - Build shared library:
     ```
     make lib
     ```
   - Build static library:
     ```
     make static-lib
     ```
   - Enable debug mode:
     ```
     make DEBUG=1
     ```
   - Enable AddressSanitizer:
     ```
     make SANITIZE=1
     ```
   - Enable optimizations:
     ```
     make OPTIMIZE=1
     ```
   - Use a specific compiler:
     ```
     make CC=clang
     ```

   You can combine these options as needed, for example:
   ```
   make bench CC=clang OPTIMIZE=1
   ```

#### Using Meson (Alternative method)

1. Configure the build:
   ```
   meson setup builddir
   ```

2. Build the project:
   ```
   meson compile -C builddir
   ```

3. (Optional) Install the library system-wide:
   ```
   sudo meson install -C builddir
   ```

4. Additional Meson options:
   - Enable debug mode:
     ```
     meson setup builddir -Denable_debug=true
     ```
   - Enable AddressSanitizer:
     ```
     meson setup builddir -Denable_sanitize=address
     ```
   - Enable optimizations:
     ```
     meson setup builddir -Denable_optimize=true
     ```

## API

The main API is defined in `include/secureinput.h`. Key functions include:

- `int init_secureinput(void)`: Initialize the library
- `int get_password_input(pw_state_t *pw_state, const char *prompt)`: Get password input (C version)
- `void getpw_asm()`: Get password input (Assembly version)
- `void *secure_alloc(size_t size)`: Securely allocate memory
- `void secure_free(void *ptr, size_t size)`: Securely free memory
- `int secure_compare(const void *a, const void *b, uint_least32_t length)`: Constant-time memory comparison

For full API details, refer to `include/secureinput.h`.

## Usage

1. Include the header in your C file:
   ```c
   #include <secureinput.h>
   ```

2. Link against the library when compiling:
   ```
   gcc your_program.c -lsecureinput -o your_program
   ```

## Examples

Here's a basic example of using SecureInput to read a password:
```c
#include <stdio.h>
#include <secureinput.h>

int main() {
    if (init_secureinput() != 0) {
        fprintf(stderr, "Failed to initialize SecureInput\n");
        return 1;
    }

    if (get_password_input(&pwstate, "Enter password: ") == 0) {
        printf("Password entered: %s\n", pwstate.password);
    }

    free_pw_state(&pwstate);
    return 0;
}
```
For more examples, check the `examples/` directory in the repository.

## Testing

To build the constant-time test suite:
make test_const_time

To build and run the constant-time test suite:
make run_test_const_time

To run benchmarks:
make bench
./bench_secureinput

## Development

When developing or debugging, you can use the following options:

- Enable debug mode: make DEBUG=1
- Enable AddressSanitizer: make SANITIZE=1
- Enable optimizations: make OPTIMIZE=1

You can combine these options as needed. For example, to build with debug symbols and AddressSanitizer:
make DEBUG=1 SANITIZE=1

For a full list of available make targets and options, run:
make help

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License and Copyright

This project is (c) 2024 Ben Gorlick, and is licensed under the MIT License - see the [LICENSE](LICENSE) file for details. To use, please attribute by including the copyright notice and attribution in any use or distribution.

## Acknowledgments

- This project was inspired by the need for secure input handling in cryptographic applications.
- Special thanks to the open-source community for their invaluable resources on constant-time programming and side-channel attack prevention.

## Disclaimer

This library is provided as-is, without any guarantees or warranties. It is designed for educational purposes and for use in non-critical applications. While efforts have been made to ensure its security and correctness, it has not undergone formal security audits. Use at your own risk.

