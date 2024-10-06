; secureinput.asm
; (c) 2024 by Ben Gorlick @bgorlick | LICENSE: MIT
; Modular functions for disabling and enabling terminal echo
; and secure memory handling for confidential / password input
; This is a work in progress, and is for educational purposes only - use at your own risk
; It may contain bugs, and may be subject to unintended behavior.
; Current maximum password length: 64 characters (defined in secureinput.h and bench_secureinput.c)
; The length is arbitrary, multiple of 4 or powers of 2 is nice, looks good for hex printing also.

default rel

; Constants
%define NEWLINE 10
%define SPACE 32

; Syscall numbers
%define SYS_READ 0
%define SYS_WRITE 1
%define SYS_MMAP 9
%define SYS_MUNMAP 11
%define SYS_IOCTL 16
%define SYS_MLOCK 149
%define SYS_MUNLOCK 150

; File descriptors
%define STDIN_FILENO 0
%define STDOUT_FILENO 1

; ioctl request codes
%define TCGETS 0x5401
%define TCSETS 0x5402

; termios flags
%define ECHO_FLAG 0x00000008
%define ICANON_FLAG 0x00000002

; mmap flags
%define PROT_READ 0x1
%define PROT_WRITE 0x2
%define MAP_PRIVATE 0x2
%define MAP_ANONYMOUS 0x20

; Other constants
%define MAP_FAILED -1

; pwstate structure offsets
%define PWSTATE_CURRENT_OFFSET 0
%define PWSTATE_OLD_OFFSET 8
%define PWSTATE_PASSWORD_OFFSET 16
%define PWSTATE_BUFFER_OFFSET 24
%define PWSTATE_CAPACITY_OFFSET 32
%define PWSTATE_LENGTH_OFFSET 40

; Debug print macro
%ifdef DEBUG
    %define DEBUG_PRINT 1
%else
    %define DEBUG_PRINT 0
%endif

; Newline macro
%macro newline 0
    syscall SYS_WRITE, STDOUT_FILENO, %%newline_char, 1
    jmp %%after_data
    %%newline_char: db NEWLINE
    %%after_data:
%endmacro

; Syscall macro
%macro syscall 1-7
    mov rax, %1
    %if %0 > 1
        mov rdi, %2
    %endif
    %if %0 > 2
        mov rsi, %3
    %endif
    %if %0 > 3
        mov rdx, %4
    %endif
    %if %0 > 4
        mov r10, %5
    %endif
    %if %0 > 5
        mov r8, %6
    %endif
    %if %0 > 6
        mov r9, %7
    %endif
    syscall
%endmacro

; Error handling macro
%macro check_error 1
    test rax, rax
    js .%1_error
%endmacro

; Function prologue macro
%macro function_prologue 0
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
%endmacro

; Function epilogue macro
%macro function_epilogue 0
    pop r15
    pop r14
    pop r13
    pop r12
    mov rsp, rbp
    pop rbp
    ret
%endmacro

; debug string macro
%macro debug_string 2-3 1
    %if DEBUG_PRINT && %3
        %1 db %2, NEWLINE, 0
        %1_len equ $ - %1 - 1
    %endif
%endmacro

; debug print macro
%macro debug_print 2
    %if DEBUG_PRINT
    syscall SYS_WRITE, STDOUT_FILENO, %1, %2
    %endif
%endmacro

section .data
    ; Messages
    PROMPT_SECUREINPUT db "Enter password (ASM): ", 0
    PROMPT_SECUREINPUT_LEN equ $ - PROMPT_SECUREINPUT - 1

    debug_string DEBUG_MSG1, "Debug: entered getpw_asm"
    debug_string DEBUG_MSG2, "Debug: calling disable_echo_asm"
    debug_string DEBUG_MSG3, "Debug: about to read password"
    debug_string DEBUG_MSG4, "Debug: calling enable_echo_asm"
    debug_string DEBUG_CURRENT_LFLAG, "Current c_lflag value: "
    debug_string DEBUG_MODIFIED_LFLAG, "Modified c_lflag value: "
    debug_string DEBUG_ECHO_DISABLED, "Echo should now be disabled"
    debug_string debug_enter_alloc, "Entering secure_alloc_asm"
    debug_string debug_mmap_success, "mmap successful"
    debug_string debug_mlock_success, "mlock successful"
    debug_string debug_mmap_failed, "mmap failed"
    debug_string debug_mlock_failed, "mlock failed"
    debug_string debug_enter_getpw_asm, "Entered getpw_asm function"
    debug_string debug_pwstate_addr, "pwstate address: "
    debug_string debug_buffer_addr, "Buffer addr: "
    debug_string debug_buffer_capacity, "Buffer capacity: "
    debug_string debug_password_entered, "Password entered (ASM): "
    debug_string debug_reading_input, "debug_reading_input"
    debug_string debug_read_done, "Finished reading"
    debug_string debug_read_char, "Read character"
    debug_string debug_pwstate_null, "pwstate is NULL"
    debug_string debug_buffer_null, "password buffer is NULL"
    debug_string debug_capacity_zero, "buffer capacity is 0"
    debug_string debug_before_read_loop, "Entering read_loop (ASM)"
    debug_string debug_pwstate_content, "pwstate content: "

    hex_chars db "0123456789ABCDEF"
    pwstate_ptr dq 0  ; Pointer to pwstate

section .bss
    termios_struct resb 80  ; Allocate 80 bytes for termios structure
    original_lflag resd 1   ; Reserve 4 bytes to store original c_lflag
    hex_buffer resb 24      ; Buffer for hexadecimal conversion

section .text
    global getpw_asm
    global disable_echo_asm
    global enable_echo_asm
    global secure_alloc_asm
    global secure_free_asm
    global secure_wipe_asm
    extern pwstate

; Function: secure_alloc_asm
; Description: Securely allocates memory using mmap and mlock
; Parameters: rdi = size to allocate
; Returns: rax = pointer to allocated memory, or 0 on failure
secure_alloc_asm:
    function_prologue

    mov r12, rdi           ; Store size to allocate in r12
    syscall SYS_MMAP, 0, r12, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    check_error alloc

    mov rbx, rax           ; Save allocated address in rbx

    ; Zero out the allocated memory with fast memory zeroing (rep stosb)
    mov rdi, rbx           ; Address of allocated memory
    mov rcx, r12           ; Number of bytes to zero (size)
    xor rax, rax           ; Set zero in rax
    rep stosb              ; Zero out memory using rep stosb

    ; Lock the allocated memory (mlock syscall)
    syscall SYS_MLOCK, rbx, r12
    check_error mlock

    mov rax, rbx           ; Return the allocated address in rax
    function_epilogue

.alloc_error:
    xor rax, rax           ; Return 0 on allocation error
    function_epilogue

.mlock_error:
    ; Clean up on mlock failure (unmap the allocated memory)
    syscall SYS_MUNMAP, rbx, r12
    xor rax, rax           ; Return 0 on error
    function_epilogue

; Function: secure_free_asm
; Description: Securely frees memory using munlock and munmap
; Parameters: rdi = pointer to memory, rsi = size
; Returns: nothing
secure_free_asm:
    function_prologue
    call secure_wipe_asm    ; Wipe memory before freeing

    ; Unlock the memory (munlock syscall)
    syscall SYS_MUNLOCK, rdi, rsi

    ; Unmap the memory (munmap syscall)
    syscall SYS_MUNMAP, rdi, rsi
    function_epilogue

; Function: secure_wipe_asm
; Description: Securely wipes memory
; Parameters: rdi = pointer to memory, rsi = size
; Returns: nothing
secure_wipe_asm:
    function_prologue
    mov rcx, rsi            ; Number of bytes to clear
    xor rax, rax            ; Value to set (zero)
    rep stosb               ; Fast memory zeroing using rep stosb
    function_epilogue

; Function: disable_echo_asm
; Description: Disables ECHO and ICANON flags in terminal settings
; Returns: 0 on success, -1 on failure
disable_echo_asm:
    function_prologue
    debug_print DEBUG_MSG2, DEBUG_MSG2_len

    ; Get current terminal attributes
    syscall SYS_IOCTL, STDIN_FILENO, TCGETS, termios_struct
    cmp rax, 0
    jl .disable_error

    ; Debug: Print current c_lflag
    debug_print DEBUG_CURRENT_LFLAG, DEBUG_CURRENT_LFLAG_len
    mov eax, [termios_struct + 12]
    call print_hex
    newline

    ; Save original c_lflag
    mov eax, [termios_struct + 12]
    mov [original_lflag], eax

    ; Modify termios_struct: disable ECHO and ICANON
    and dword [termios_struct + 12], ~(ECHO_FLAG | ICANON_FLAG)

    ; Debug: Print modified c_lflag
    debug_print DEBUG_MODIFIED_LFLAG, DEBUG_MODIFIED_LFLAG_len
    mov eax, [termios_struct + 12]
    call print_hex
    newline

    ; Set c_cc[VMIN] and c_cc[VTIME]
    mov byte [termios_struct + 16], 1  ; VMIN: wait for at least one character
    mov byte [termios_struct + 17], 0  ; VTIME: no timeout

    ; Set new terminal attributes
    syscall SYS_IOCTL, STDIN_FILENO, TCSETS, termios_struct
    cmp rax, 0
    jl .disable_error

    debug_print DEBUG_ECHO_DISABLED, DEBUG_ECHO_DISABLED_len

    xor rax, rax              ; Return 0 on success
    function_epilogue

.disable_error:
    mov rax, -1
    function_epilogue

; Function: enable_echo_asm
; Description: Restores the original terminal settings
; Returns: 0 on success, -1 on failure
enable_echo_asm:
    function_prologue
    debug_print DEBUG_MSG4, DEBUG_MSG4_len

    ; Restore original c_lflag
    mov eax, [original_lflag]
    mov [termios_struct + 12], eax

    ; Set terminal attributes to restore settings
    syscall SYS_IOCTL, STDIN_FILENO, TCSETS, termios_struct
    check_error enable

    xor rax, rax              ; Return 0 on success
    function_epilogue

.enable_error:
    mov rax, -1
    function_epilogue

getpw_asm:
    function_prologue
    
    debug_print debug_enter_getpw_asm, debug_enter_getpw_asm_len
    
    ; Load address of pwstate
    mov rax, [rel pwstate_ptr]
    test rax, rax
    jnz .pwstate_loaded
    lea rax, [rel pwstate wrt ..got]
    mov rax, [rax]
    mov [rel pwstate_ptr], rax
.pwstate_loaded:
    mov r13, rax

    ; Call disable_echo_asm
    call disable_echo_asm
    cmp rax, 0
    jl .getpw_asm_error

    ; Print prompt
    syscall SYS_WRITE, STDOUT_FILENO, PROMPT_SECUREINPUT, PROMPT_SECUREINPUT_LEN

    ; Read password input
    xor r12, r12            ; Clear r12 to use as character counter
    test r13, r13           ; Check if pwstate is NULL
    jz .pwstate_null_error

    mov r14, [r13 + PWSTATE_PASSWORD_OFFSET]  ; Load password buffer address
    test r14, r14           ; Check if password buffer is NULL
    jz .buffer_null_error

    mov r15, [r13 + PWSTATE_CAPACITY_OFFSET]  ; Load buffer capacity
    test r15, r15           ; Check if buffer capacity is 0
    jz .capacity_zero_error

    ; Clear the existing password buffer
    push rdi
    push rcx
    mov rdi, r14
    mov rcx, r15
    xor rax, rax
    rep stosb
    pop rcx
    pop rdi

    ; Constant-time reading loop
    mov rcx, r15            ; Set counter to buffer capacity
    dec rcx                 ; Decrement to leave space for null terminator

.read_loop:
    push rcx
    syscall SYS_READ, STDIN_FILENO, r14, 1
    pop rcx
    cmp rax, 1
    jne .read_error

    mov al, [r14]
    cmp al, NEWLINE         ; Check for newline (ASCII 10)
    je .read_done

    ; Only increment if we haven't reached the end
    cmp r12, r15
    jge .skip_increment
    inc r12                 ; Increment character count
    inc r14                 ; Move to next position in buffer

.skip_increment:
    loop .read_loop

.read_done:
    ; Ensure we always process up to capacity - 1 characters
    mov rcx, r15
    sub rcx, r12
    dec rcx
    jle .finish_read
    mov rdi, r14
    xor rax, rax
    rep stosb

.finish_read:
    mov byte [r14], 0       ; Null terminate the string
    mov [r13 + PWSTATE_LENGTH_OFFSET], r12b  ; Store length

    ; Print newline
    newline

    ; Call enable_echo_asm
    call enable_echo_asm
    cmp rax, 0
    jl .getpw_asm_error

    xor rax, rax  ; Return 0 on success
    function_epilogue

.pwstate_null_error:
    debug_print debug_pwstate_null, debug_pwstate_null_len
    jmp .getpw_asm_error

.buffer_null_error:
    debug_print debug_buffer_null, debug_buffer_null_len
    jmp .getpw_asm_error

.capacity_zero_error:
    debug_print debug_capacity_zero, debug_capacity_zero_len
    jmp .getpw_asm_error

.getpw_asm_error:
    ; Attempt to restore terminal attributes before exiting
    call enable_echo_asm
    mov rax, -1  ; Return error code
    function_epilogue

.read_error:
    ; Handle read error
    call enable_echo_asm
    mov rax, -1  ; Return error code
    function_epilogue

; Helper function to print a 32-bit value in hexadecimal
print_hex:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push rbx

    mov rcx, 8      ; Counter for 8 hex digits
    lea rsi, [rel hex_buffer + 16]  ; Point to the end of the buffer
    lea rbx, [rel hex_chars]  ; Load address of hex_chars into rbx

.print_loop:
    rol eax, 4      ; Rotate left by 4 bits
    mov edx, eax
    and edx, 0xf    ; Mask off lowest 4 bits
    movzx edx, byte [rbx + rdx]  ; Convert to ASCII using rbx
    mov [rsi], dl   ; Store ASCII char
    dec rsi         ; Move buffer pointer
    loop .print_loop

    syscall SYS_WRITE, STDOUT_FILENO, hex_buffer + 8, 8

    ; Print space instead of newline
    syscall SYS_WRITE, STDOUT_FILENO, .space, 1
    jmp .after_data
    .space: db ' '
    .after_data:

    pop rbx
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    ret