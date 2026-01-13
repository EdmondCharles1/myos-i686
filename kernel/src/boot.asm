; Multiboot header
section .multiboot
    dd 0x1BADB002
    dd 0x00
    dd -(0x1BADB002 + 0x00)

; Stack (16KB)
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; Entry point
section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
