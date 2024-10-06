// File: examples/test.c
#include <stdio.h>
#include <secureinput.h>

extern tty_state_t tty;
extern pw_state_t pwstate;

int main() {
    // Initialize the secure input library
    if (init_secureinput() != 0) {
        fprintf(stderr, "Failed to initialize secure input\n");
        return 1;
    }

    // Test C function
    printf("Enter a password (C function): ");
    if (get_password_input(&pwstate, "Enter password: ") == 0) {
        printf("Password entered: %s\n", pwstate.password);
        printf("Password length: %d\n", pwstate.length);
    } else {
        printf("Failed to get password input\n");
    }

    // Test ASM function
    printf("\nNow testing ASM function:\n");
    getpw_asm();
    printf("Password entered (ASM): %s\n", pwstate.password);
    printf("Password length: %d\n", pwstate.length);

    // Clean up
    free_pw_state(&pwstate);
    return 0;
}