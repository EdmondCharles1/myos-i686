; =============================================================================
; isr_asm.asm - Interrupt Service Routine stubs
; =============================================================================

; Ces macros créent des stubs pour chaque interruption
; Certaines exceptions pushent automatiquement un error code, d'autres non

; -----------------------------------------------------------------------------
; Macro pour ISR sans error code
; -----------------------------------------------------------------------------
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli                     ; Désactive les interruptions
        push byte 0             ; Push un dummy error code
        push byte %1            ; Push le numéro d'interruption
        jmp isr_common_stub     ; Jump vers le handler commun
%endmacro

; -----------------------------------------------------------------------------
; Macro pour ISR avec error code (pushé automatiquement par le CPU)
; -----------------------------------------------------------------------------
%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli                     ; Désactive les interruptions
        push byte %1            ; Push le numéro d'interruption
        jmp isr_common_stub     ; Jump vers le handler commun
%endmacro

; -----------------------------------------------------------------------------
; Macro pour IRQ (Hardware interrupts)
; -----------------------------------------------------------------------------
%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push byte 0             ; Dummy error code
        push byte %2            ; Numéro IRQ
        jmp irq_common_stub
%endmacro

; =============================================================================
; Exceptions CPU (0-31)
; =============================================================================

ISR_NOERRCODE 0     ; Division par zéro
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; Non Maskable Interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound Range Exceeded
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device Not Available
ISR_ERRCODE   8     ; Double Fault (avec error code)
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Invalid TSS (avec error code)
ISR_ERRCODE   11    ; Segment Not Present (avec error code)
ISR_ERRCODE   12    ; Stack-Segment Fault (avec error code)
ISR_ERRCODE   13    ; General Protection Fault (avec error code)
ISR_ERRCODE   14    ; Page Fault (avec error code)
ISR_NOERRCODE 15    ; Réservé
ISR_NOERRCODE 16    ; x87 Floating-Point Exception
ISR_ERRCODE   17    ; Alignment Check (avec error code)
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating-Point Exception
ISR_NOERRCODE 20    ; Virtualization Exception
ISR_NOERRCODE 21    ; Réservé
ISR_NOERRCODE 22    ; Réservé
ISR_NOERRCODE 23    ; Réservé
ISR_NOERRCODE 24    ; Réservé
ISR_NOERRCODE 25    ; Réservé
ISR_NOERRCODE 26    ; Réservé
ISR_NOERRCODE 27    ; Réservé
ISR_NOERRCODE 28    ; Réservé
ISR_NOERRCODE 29    ; Réservé
ISR_ERRCODE   30    ; Security Exception (avec error code)
ISR_NOERRCODE 31    ; Réservé

; =============================================================================
; IRQs (32-47) - Hardware interrupts
; =============================================================================

IRQ 0,  32    ; IRQ 0  - Timer
IRQ 1,  33    ; IRQ 1  - Clavier
IRQ 2,  34    ; IRQ 2  - Cascade (PIC esclave)
IRQ 3,  35    ; IRQ 3  - COM2
IRQ 4,  36    ; IRQ 4  - COM1
IRQ 5,  37    ; IRQ 5  - LPT2
IRQ 6,  38    ; IRQ 6  - Floppy disk
IRQ 7,  39    ; IRQ 7  - LPT1
IRQ 8,  40    ; IRQ 8  - CMOS real-time clock
IRQ 9,  41    ; IRQ 9  - Free
IRQ 10, 42    ; IRQ 10 - Free
IRQ 11, 43    ; IRQ 11 - Free
IRQ 12, 44    ; IRQ 12 - PS/2 Mouse
IRQ 13, 45    ; IRQ 13 - FPU / Coprocessor
IRQ 14, 46    ; IRQ 14 - Primary ATA Hard Disk
IRQ 15, 47    ; IRQ 15 - Secondary ATA Hard Disk

; =============================================================================
; Handler commun pour les ISR (exceptions CPU)
; =============================================================================

extern isr_handler

isr_common_stub:
    ; Sauvegarder tous les registres
    pusha                   ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    
    ; Sauvegarder les segments
    mov ax, ds
    push eax
    
    ; Charger le segment de données du kernel
    mov ax, 0x10            ; 0x10 = kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Appeler le handler C
    call isr_handler
    
    ; Restaurer les segments
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Restaurer les registres
    popa
    
    ; Nettoyer la pile (error code + int number)
    add esp, 8
    
    ; Retour de l'interruption
    sti                     ; Réactive les interruptions
    iret

; =============================================================================
; Handler commun pour les IRQ (hardware interrupts)
; =============================================================================

extern irq_handler

irq_common_stub:
    pusha
    
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call irq_handler
    
    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    
    popa
    add esp, 8
    sti
    iret

; =============================================================================
; Fonction pour charger l'IDT
; =============================================================================

global idt_load
idt_load:
    mov eax, [esp + 4]      ; Récupérer l'adresse de l'IDT pointer
    lidt [eax]              ; Charger l'IDT
    ret