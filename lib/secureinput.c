#include "secureinput.h"

pw_state_t pwstate;

void tty_save_state(tty_state_t *state) {
    tcgetattr(STDIN_FILENO, &state->original_termios);
    state->current_termios = state->original_termios;
}

void tty_restore_state(const tty_state_t *state) {
    tcsetattr(STDIN_FILENO, TCSANOW, &state->original_termios);
}

void tty_disable_echo(tty_state_t *state) {
    state->current_termios.c_lflag &= ~(ECHO | ICANON);
    state->current_termios.c_cc[VMIN] = 1;
    state->current_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &state->current_termios);
}

void tty_enable_echo(tty_state_t *state) {
    state->current_termios.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &state->current_termios);
}

void *secure_alloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return NULL;
    
    if (mlock(ptr, size) != 0) {
        munmap(ptr, size);
        return NULL;
    }
    memset(ptr, 0, size);  // Initialize to zero for security
    return ptr;
}

void secure_wipe(void *ptr, size_t size) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (size--) {
        *p++ = 0;
    }
}

void secure_free(void *ptr, size_t size) {
    if (ptr == NULL) return;
    secure_wipe(ptr, size);
    munlock(ptr, size);
    munmap(ptr, size);
}

int init_pw_state(pw_state_t *state) {
    state->capacity = PASSWORD_MAX_LENGTH;
    state->current = secure_alloc(state->capacity);
    state->old = secure_alloc(state->capacity);
    state->password = secure_alloc(state->capacity);
    state->buffer = secure_alloc(state->capacity);
    
    if (!state->current || !state->old || !state->password || !state->buffer) {
        free_pw_state(state);
        return -1;
    }
    
    state->length = 0;
    return 0;
}

int init_secureinput(void) {
    return init_pw_state(&pwstate);
}

void free_pw_state(pw_state_t *state) {
    if (state->current) secure_free(state->current, state->capacity);
    if (state->old) secure_free(state->old, state->capacity);
    if (state->password) secure_free(state->password, state->capacity);
    if (state->buffer) secure_free(state->buffer, state->capacity);
    memset(state, 0, sizeof(pw_state_t));
}

int get_password_input(pw_state_t *pw_state, const char *prompt) {
    tty_state_t temp_state;
    tty_save_state(&temp_state);
    tty_disable_echo(&temp_state);

    printf("%s", prompt);
    fflush(stdout);

    size_t i = 0;
    char c;
    while (i < pw_state->capacity - 1) {
        if (read(STDIN_FILENO, &c, 1) != 1) {
            break;
        }
        if (c == '\n' || c == '\r') {
            break;
        }
        if (i < pw_state->capacity - 1) {
            pw_state->password[i] = c;
        }
        i++;
    }

    // Ensure we always process up to capacity - 1 characters
    for (; i < pw_state->capacity - 1; i++) {
        pw_state->password[i] = '\0';
    }
    pw_state->password[pw_state->capacity - 1] = '\0';
    pw_state->length = (uint8_t)strlen(pw_state->password);

    printf("\n");
    tty_restore_state(&temp_state);
    return 0;
}


int secure_compare(const void *a, const void *b, uint_least32_t length) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    uint_least32_t result = 0;
    uint_least32_t i;

    for (i = 0; i < length; i++) {
        result |= pa[i] ^ pb[i];
    }

    return (result == 0) ? 1 : 0;
}