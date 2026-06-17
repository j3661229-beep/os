; ==============================================================================
; gdt.asm - Global Descriptor Table
;
; Role: Defines the memory segments for 32-bit Protected Mode.
; The GDT tells the CPU about memory segments, their base addresses, limits,
; and access privileges (e.g., read/write/execute, Ring 0 vs Ring 3).
; We use a "flat" memory model where both Code and Data segments span the
; entire 4GB address space (Base=0, Limit=0xFFFFF with 4KB granularity).
; ==============================================================================

; Provide a global symbol so the linker can find it if compiled to an object file
global gdt_descriptor

gdt_start:

; 1. Null Descriptor (Required by the x86 architecture)
; The CPU requires the first entry to be exactly 8 bytes of zeros.
gdt_null:
    dd 0x0
    dd 0x0

; 2. Code Segment Descriptor
; Base: 0x0, Limit: 0xFFFFF (spanning 4GB with 4KB granularity)
; Access byte: 10011010b (Present, Ring 0, Code/Data, Executable, Readable)
; Flags: 1100b (4KB Granularity, 32-bit Protected Mode)
gdt_code:
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10011010b    ; Access byte (Present=1, Privilege=00, Type=1, Executable=1, Conforming=0, Readable=1, Accessed=0)
    db 11001111b    ; Flags (Granularity=1, 32-bit=1, 64-bit=0, AVL=0) + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

; 3. Data Segment Descriptor
; Base: 0x0, Limit: 0xFFFFF (spanning 4GB with 4KB granularity)
; Access byte: 10010010b (Present, Ring 0, Code/Data, Not Executable, Writable)
; Flags: 1100b (4KB Granularity, 32-bit Protected Mode)
gdt_data:
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10010010b    ; Access byte (Present=1, Privilege=00, Type=1, Executable=0, Direction=0, Writable=1, Accessed=0)
    db 11001111b    ; Flags (Granularity=1, 32-bit=1, 64-bit=0, AVL=0) + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

gdt_end:

; ------------------------------------------------------------------------------
; GDT Descriptor
; This is the structure we pass to the `lgdt` instruction.
; It contains the size of the GDT and its memory address.
; ------------------------------------------------------------------------------
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size of GDT (16-bit), always 1 less than true size
    dd gdt_start                ; Start address of GDT (32-bit)

; ------------------------------------------------------------------------------
; Constants for Segment Selectors
; These are the offsets of our segments in the GDT.
; Each entry is 8 bytes. Code is at offset 8, Data is at offset 16 (0x10).
; ------------------------------------------------------------------------------
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
