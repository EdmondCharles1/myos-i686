; boot.asm - Multiboot header + Entry point
; Compatible GRUB/QEMU multiboot

BITS 32

; =============================================================================
; Multiboot Header (DOIT être dans les premiers 8KB du binaire)
; =============================================================================
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002                       ; Magic number
    dd 0x00000003                       ; Flags (align modules + memory info)
    dd -(0x1BADB002 + 0x00000003)       ; Checksum

; =============================================================================
; Entry Point
; =============================================================================
section .text
global _start
extern kernel_main

_start:
    ; Désactiver les interruptions immédiatement
    cli

    ; Configurer la pile
    mov esp, stack_space + 16384

    ; Réinitialiser les flags
    push 0
    popf

    ; Appeler le kernel
    call kernel_main

    ; Si kernel_main retourne (ne devrait pas)
    cli
.halt_loop:
    hlt
    jmp .halt_loop

; =============================================================================
; Stack (16 KB)
; =============================================================================
section .bss
align 16
stack_space:
    resb 16384
