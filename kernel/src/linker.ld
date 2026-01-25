ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Le header Multiboot DOIT être dans les premiers 8KB */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Ignorer les sections inutiles */
    /DISCARD/ : {
        *(.note*)
        *(.eh_frame*)
        *(.comment)
    }
}
