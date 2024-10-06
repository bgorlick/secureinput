#include "secureinput.h"
#include <stdio.h>
#include <unistd.h>

tty_state_t tty;


//pw_state_t pwstate;

void print_memory(const char *label, const void *ptr, size_t size) {
    printf("%s:\n", label);
    const unsigned char *p = (const unsigned char *)ptr;
    for (size_t i = 0; i < size; i++) {
        printf("%02x ", p[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int main() {
    tty_save_state(&tty);
    
    if (init_pw_state(&pwstate) != 0) {
        fprintf(stderr, "Failed to initialize password state\n");
        return 1;
    }
    
    printf("Address of pwstate: %p\n", (void*)&pwstate);
    printf("Size of pwstate: %zu\n", sizeof(pwstate));
    
    // Test C functions
    printf("\n--- Testing C functions ---\n");
    print_memory("Content of password buffer before input (C)", pwstate.password, pwstate.capacity);
    
    if (get_password_input(&pwstate, "Enter password (C): ") == 0) {
        printf("Password entered (C): %s\n", pwstate.password);
        print_memory("Content of password buffer after input (C)", pwstate.password, pwstate.capacity);
    }

    secure_wipe(pwstate.password, pwstate.capacity);
    print_memory("Content of password buffer after wiping (C)", pwstate.password, pwstate.capacity);

    // Test ASM functions
    printf("\n--- Testing ASM functions ---\n");
    
    printf("pwstate address: %p\n", (void*)&pwstate);
    printf("Buffer address: %p\n", (void*)pwstate.password);
    printf("Buffer capacity: %zu\n", pwstate.capacity);

    print_memory("Content of ASM buffer before input", pwstate.password, pwstate.capacity);

    // Use Assembly function for password input
    printf("Calling getpw_asm() ASM and flushing stdout\n");

    fflush(stdout);


    getpw_asm();
    printf("Returned from getpw_asm()\n");

    if (pwstate.length > 0) {
        printf("Password entered (ASM): %s\n", pwstate.password);
        print_memory("Content of ASM buffer after input", pwstate.password, pwstate.capacity);
    } else {
        printf("No password entered or error occurred\n");
    }

    // Wipe ASM buffer
    secure_wipe_asm(pwstate.password, pwstate.capacity);
    print_memory("Content of ASM buffer after wiping", pwstate.password, pwstate.capacity);

    printf("Restoring state: tty_restore_state\n");
    tty_restore_state(&tty);

    free_pw_state(&pwstate);
    return 0;
}

/* Compiling works like this:
nasm -f elf64 secureinput.asm -o secureinput_asm.o
gcc -c secureinput.c -o secureinput_c.o
gcc -c main.c -o main.o
gcc main.o secureinput_asm.o secureinput_c.o -o secureinput -no-pie -g
./secureinput
*/