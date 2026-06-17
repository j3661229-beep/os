; ==============================================================================
; isr.asm - Interrupt Service Routines (Assembly Stubs)
;
; Role: When a CPU exception or hardware IRQ fires, the CPU jumps here first.
; We save the CPU state (registers) onto the stack, call our C handler, 
; and then restore the state before returning to what the CPU was doing.
; ==============================================================================

[BITS 32]

extern isr_handler
extern irq_handler

; The Kernel Data Segment selector from our GDT
DATA_SEG equ 0x10

; ------------------------------------------------------------------------------
; Macros to easily define our ISRs
; ------------------------------------------------------------------------------

; Macro for exceptions that DO NOT push an error code onto the stack by default
%macro isr_no_err_stub 1
global isr%1
isr%1:
    push 0          ; Push a dummy error code so the stack looks uniform
    push %1         ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Macro for exceptions that DO push an error code (like Page Fault)
%macro isr_err_stub 1
global isr%1
isr%1:
    ; The CPU already pushed the error code
    push %1         ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Macro for hardware IRQs (they never push an error code)
%macro irq_stub 2
global irq%1
irq%1:
    push 0          ; Dummy error code
    push %2         ; The remapped interrupt number (e.g., IRQ0 = 32)
    jmp irq_common_stub
%endmacro

; ------------------------------------------------------------------------------
; Generate the 32 CPU Exception Stubs
; Exceptions 8, 10, 11, 12, 13, 14, 17, 21 push error codes.
; ------------------------------------------------------------------------------
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_err_stub    21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_no_err_stub 30
isr_no_err_stub 31

; ------------------------------------------------------------------------------
; Generate the 16 Hardware IRQ Stubs
; ------------------------------------------------------------------------------
irq_stub 0, 32
irq_stub 1, 33
irq_stub 2, 34
irq_stub 3, 35
irq_stub 4, 36
irq_stub 5, 37
irq_stub 6, 38
irq_stub 7, 39
irq_stub 8, 40
irq_stub 9, 41
irq_stub 10, 42
irq_stub 11, 43
irq_stub 12, 44
irq_stub 13, 45
irq_stub 14, 46
irq_stub 15, 47

; ------------------------------------------------------------------------------
; Common ISR Stub (CPU Exceptions)
; ------------------------------------------------------------------------------
isr_common_stub:
    pusha               ; Save all general-purpose registers (edi, esi, ebp, esp, ebx, edx, ecx, eax)

    mov ax, ds          ; Lower 16 bits of eax = ds
    push eax            ; Save the data segment descriptor
    
    mov ax, DATA_SEG    ; Load the kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; The stack pointer now points exactly to the `Registers` struct passed to C
    push esp            ; Pass pointer to the stack (Registers struct) to the C function
    call isr_handler    ; Jump to our C code
    add esp, 4          ; Clean up the pointer we just pushed
    
    pop eax             ; Restore the original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                ; Restore all general-purpose registers
    add esp, 8          ; Clean up the error code and interrupt number
    iret                ; Return from interrupt (pops CS, EIP, EFLAGS, SS, ESP)

; ------------------------------------------------------------------------------
; Common IRQ Stub (Hardware Interrupts)
; ------------------------------------------------------------------------------
irq_common_stub:
    pusha               ; Save all general-purpose registers

    mov ax, ds          
    push eax            ; Save the data segment descriptor
    
    mov ax, DATA_SEG    
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Pass pointer to stack to C function
    call irq_handler    ; Call the hardware interrupt router in C
    add esp, 4          
    
    pop eax             ; Restore original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                ; Restore registers
    add esp, 8          ; Clean up error code and interrupt number
    iret                ; Return from interrupt
