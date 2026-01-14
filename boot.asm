; boot.asm - Version simplifiée avec Multiboot dans .text

BITS 32

section .text
align 4

; Multiboot header
multiboot_header:
    dd 0x1BADB002                       ; magic
    dd 0x00000000                       ; flags
    dd -(0x1BADB002 + 0x00000000)       ; checksum

; Entry point
global _start
extern kernel_main

_start:
    mov esp, stack_space + 16384        ; Setup stack
    call kernel_main                    ; Call kernel
    cli                                 ; Disable interrupts
.loop:
    hlt                                 ; Halt
    jmp .loop                           ; Loop forever

; Stack
section .bss
align 16
stack_space:
    resb 16384                          ; 16KB stack
