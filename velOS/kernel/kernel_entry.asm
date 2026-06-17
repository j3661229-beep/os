; ==============================================================================
; kernel_entry.asm - Kernel Entry Point
;
; Role: When the bootloader jumps to the kernel, it lands here.
; We are in 32-bit Protected Mode, but we need to ensure our segments
; and stack are correctly set up before calling C code.
; This file bridges the gap between Assembly and C, calling `kernel_main`.
; ==============================================================================

[BITS 32]           ; We are in 32-bit Protected Mode now
[EXTERN kernel_main] ; Declare that kernel_main is defined in another file (kernel.c)
[EXTERN __bss_start] ; BSS section boundaries from linker script
[EXTERN __bss_end]

global _start       ; Export _start so the linker knows where the entry point is

; Define our data segment selector based on the GDT (0x10 is the 3rd entry)
DATA_SEG equ 0x10

section .entry
_start:
    ; 1. Set up data segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 2. Set up a secure stack for our C kernel
    mov esp, 0x90000

    ; 3. Zero the BSS section
    ; objcopy -O binary does NOT include BSS in the output,
    ; so all uninitialized globals (pmm_bitmap, page_directory,
    ; heap_head, etc.) contain garbage. We MUST zero them.
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi            ; ecx = size of BSS in bytes
    shr ecx, 2              ; ecx = size in dwords (divide by 4)
    xor eax, eax            ; eax = 0
    rep stosd               ; Fill [edi..edi+ecx*4] with zero

    ; 4. Call our C kernel main function
    call kernel_main

    ; 4. Hang forever if the C kernel returns
    ; This prevents the CPU from executing random memory as code.
hang:
    jmp $           ; Jump to the current address, creating an infinite loop
