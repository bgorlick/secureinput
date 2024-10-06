#ifndef SECUREINPUT_H
#define SECUREINPUT_H

#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

/* 
Use secure_compare for raw data comparisons
Use secure_strcmp for null-terminated string comparisons
*/

#define DEBUG 0
#define PASSWORD_MAX_LENGTH 64

typedef struct {
    struct termios original_termios;
    struct termios current_termios;
} tty_state_t;

typedef struct {
    char *current;
    char *old;
    char *password;
    char *buffer;
    size_t capacity;
    uint8_t length;
    uint8_t padding[7];  // ensure 8-byte alignment
} pw_state_t;

extern tty_state_t tty;
extern pw_state_t pwstate;

// init lib
int init_secureinput(void);

// C functions
void tty_save_state(tty_state_t *state);
void tty_restore_state(const tty_state_t *state);
void tty_disable_echo(tty_state_t *state);
void tty_enable_echo(tty_state_t *state);
int get_password_input(pw_state_t *pw_state, const char *prompt);

// secure memory functions (C)
void *secure_alloc(size_t size);
void secure_free(void *ptr, size_t size);
void secure_wipe(void *ptr, size_t size);
int secure_compare(const void *a, const void *b, uint_least32_t length);

static inline int secure_strcmp(const char *a, const char *b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    if (len_a != len_b) {
        return 0;  // not equal if lengths differ
    }
    return secure_compare(a, b, (uint_least32_t)len_a);
}

// password state management
int init_pw_state(pw_state_t *state);
void free_pw_state(pw_state_t *state);

// assembly functions
extern void getpw_asm();
extern int disable_echo_asm();
extern int enable_echo_asm();
extern void *secure_alloc_asm(size_t size);
extern void secure_free_asm(void *ptr, size_t size);
extern void secure_wipe_asm(void *ptr, size_t size);

#endif // SECUREINPUT_H