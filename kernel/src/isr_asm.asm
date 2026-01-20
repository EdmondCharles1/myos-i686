; isr_asm.asm - Stubs pour ISR et IRQ

[BITS 32]

; Importations externes
extern isr_handler
extern irq_handler

; ============================================================================
; Macro pour ISR sans code d'erreur
; ============================================================================
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0          ; Dummy error code
    push byte %1         ; Interrupt number
    jmp isr_common_stub
%endmacro

; ============================================================================
; Macro pour ISR avec code d'erreur
; ============================================================================
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push byte %1         ; Interrupt number
    jmp isr_common_stub
%endmacro

; ============================================================================
; Macro pour IRQ
; ============================================================================
%macro IRQ 2
global irq%1
irq%1:
    cli
    push byte 0          ; Dummy error code
    push byte %2         ; Interrupt number
    jmp irq_common_stub
%endmacro

; ============================================================================
; ISR 0-31 (Exceptions CPU)
; ============================================================================
ISR_NOERRCODE 0      ; Division par zéro
ISR_NOERRCODE 1      ; Debug
ISR_NOERRCODE 2      ; NMI
ISR_NOERRCODE 3      ; Breakpoint
ISR_NOERRCODE 4      ; Overflow
ISR_NOERRCODE 5      ; Bound Range Exceeded
ISR_NOERRCODE 6      ; Invalid Opcode
ISR_NOERRCODE 7      ; Device Not Available
ISR_ERRCODE   8      ; Double Fault (avec error code)
ISR_NOERRCODE 9      ; Coprocessor Segment Overrun
ISR_ERRCODE   10     ; Invalid TSS (avec error code)
ISR_ERRCODE   11     ; Segment Not Present (avec error code)
ISR_ERRCODE   12     ; Stack-Segment Fault (avec error code)
ISR_ERRCODE   13     ; General Protection Fault (avec error code)
ISR_ERRCODE   14     ; Page Fault (avec error code)
ISR_NOERRCODE 15     ; Réservé
ISR_NOERRCODE 16     ; x87 FPU Error
ISR_ERRCODE   17     ; Alignment Check (avec error code)
ISR_NOERRCODE 18     ; Machine Check
ISR_NOERRCODE 19     ; SIMD Floating-Point Exception
ISR_NOERRCODE 20     ; Virtualization Exception
ISR_NOERRCODE 21     ; Réservé
ISR_NOERRCODE 22     ; Réservé
ISR_NOERRCODE 23     ; Réservé
ISR_NOERRCODE 24     ; Réservé
ISR_NOERRCODE 25     ; Réservé
ISR_NOERRCODE 26     ; Réservé
ISR_NOERRCODE 27     ; Réservé
ISR_NOERRCODE 28     ; Réservé
ISR_NOERRCODE 29     ; Réservé
ISR_NOERRCODE 30     ; Security Exception
ISR_NOERRCODE 31     ; Réservé

; ============================================================================
; IRQ 0-15 (Interruptions hardware)
; ============================================================================
IRQ 0,  32    ; IRQ0  - Timer
IRQ 1,  33    ; IRQ1  - Keyboard
IRQ 2,  34    ; IRQ2  - Cascade
IRQ 3,  35    ; IRQ3  - COM2
IRQ 4,  36    ; IRQ4  - COM1
IRQ 5,  37    ; IRQ5  - LPT2
IRQ 6,  38    ; IRQ6  - Floppy
IRQ 7,  39    ; IRQ7  - LPT1
IRQ 8,  40    ; IRQ8  - RTC
IRQ 9,  41    ; IRQ9  - Free
IRQ 10, 42    ; IRQ10 - Free
IRQ 11, 43    ; IRQ11 - Free
IRQ 12, 44    ; IRQ12 - PS2 Mouse
IRQ 13, 45    ; IRQ13 - FPU
IRQ 14, 46    ; IRQ14 - ATA Primary
IRQ 15, 47    ; IRQ15 - ATA Secondary

; ============================================================================
; Stub commun pour ISR
; ============================================================================
isr_common_stub:
    pusha                ; Sauvegarder tous les registres généraux
    
    mov ax, ds
    push eax             ; Sauvegarder le segment de données
    
    mov ax, 0x10         ; Charger le segment de données du kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp             ; Passer un pointeur vers la structure registers_t
    call isr_handler
    add esp, 4           ; Nettoyer la pile
    
    pop eax              ; Restaurer le segment de données original
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                 ; Restaurer tous les registres généraux
    add esp, 8           ; Nettoyer int_no et err_code
    sti
    iret                 ; Retour de l'interruption

; ============================================================================
; Stub commun pour IRQ
; ============================================================================
irq_common_stub:
    pusha                ; Sauvegarder tous les registres généraux
    
    mov ax, ds
    push eax             ; Sauvegarder le segment de données
    
    mov ax, 0x10         ; Charger le segment de données du kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp             ; Passer un pointeur vers la structure registers_t
    call irq_handler
    add esp, 4           ; Nettoyer la pile
    
    pop eax              ; Restaurer le segment de données original
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                 ; Restaurer tous les registres généraux
    add esp, 8           ; Nettoyer int_no et err_code
    sti
    iret                 ; Retour de l'interruption
