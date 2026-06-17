; ==============================================================================
; boot.asm - VelOS Stage 1 Bootloader
;
; Role: This is the very first code executed by the BIOS when the computer boots.
; It is loaded at memory address 0x7C00. Its job is to:
;   1. Set up segments and stack
;   2. Print a boot message
;   3. Load the kernel from disk into memory at 0x10000 (segment 0x1000:0x0000)
;   4. Switch from 16-bit Real Mode to 32-bit Protected Mode
;   5. Jump to the kernel entry point
; ==============================================================================

[ORG 0x7C00]        ; Tell the assembler we are loaded at 0x7C00 by BIOS
[BITS 16]           ; We start in 16-bit Real Mode

KERNEL_SECTORS      equ 120   ; ~61KB (Must be <= 127 to avoid BIOS INT 13h bug)
MAX_RETRIES         equ 3     ; Retry failed reads 3 times before giving up

; ==============================================================================
; 1. SETUP SEGMENTS AND STACK
; ==============================================================================
start:
    cli                         ; Disable interrupts during setup

    ; Save the boot drive number! 
    ; (0x80 is usually the first hard drive for IDE)
    mov [BOOT_DRIVE], dl

    ; Zero out all segment registers for a flat 16-bit memory model
    xor ax, ax
    mov ds, ax                  ; Data Segment = 0
    mov es, ax
    mov ss, ax                  ; Stack Segment = 0
    mov sp, 0x7C00              ; Stack grows DOWN from 0x7C00

    sti                         ; Re-enable interrupts

; ==============================================================================
; 2. PRINT BOOT MESSAGE
; ==============================================================================
    mov si, MSG_BOOT
.print:
    lodsb                       
    or al, al                   
    jz .print_done
    mov ah, 0x0E                
    int 0x10                    
    jmp .print
.print_done:

; ==============================================================================
; 3. LOAD KERNEL FROM DISK (Using LBA Extended Read - INT 0x13, AH=0x42)
; ==============================================================================
    ; Reset the disk controller
    xor ax, ax                  
    mov dl, [BOOT_DRIVE]
    int 0x13

    ; Setup the Disk Address Packet (DAP)
    ; We are loading KERNEL_SECTORS starting from LBA 1 into 0x1000:0x0000
    mov word [DAP_SECTORS], KERNEL_SECTORS
    mov word [DAP_OFFSET], 0x0000
    mov word [DAP_SEGMENT], 0x1000
    mov dword [DAP_LBA_LOW], 1
    mov dword [DAP_LBA_HIGH], 0

    mov di, MAX_RETRIES

.attempt_read:
    mov ah, 0x42                ; Extended Read function
    mov dl, [BOOT_DRIVE]        ; Drive number
    mov si, DAP                 ; DS:SI points to the Disk Address Packet
    int 0x13
    
    jnc .read_success           ; Carry flag clear = success!

    ; Read failed: Reset and retry
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    dec di
    jnz .attempt_read
    
    ; All retries failed
    jmp disk_error

.read_success:

; ==============================================================================
; 4. SWITCH TO 32-BIT PROTECTED MODE
; ==============================================================================
    ; Switch to VGA Mode 13h (320x200, 256 colors)
    mov ax, 0x0013
    int 0x10

    cli                         ; Disable interrupts (no IDT yet in PM)

    ; Enable A20 line via Fast A20 (Port 0x92)
    in al, 0x92
    or al, 0x02
    out 0x92, al

    ; Load the Global Descriptor Table
    lgdt [gdt_descriptor]

    ; Set PE bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; Far jump to flush pipeline and set CS=0x08
    jmp 0x08:protected_mode

; ==============================================================================
; ERROR HANDLER AND DATA
; ==============================================================================
disk_error:
    mov si, MSG_ERR
.err_print:
    lodsb
    or al, al
    jz .err_halt
    mov ah, 0x0E
    int 0x10
    jmp .err_print
.err_halt:
    cli
    hlt
    jmp .err_halt

; --- Static Data ---
BOOT_DRIVE  db 0                
MSG_BOOT    db "VelOS Booting...", 13, 10, 0
MSG_ERR     db "Disk Read Error!", 13, 10, 0

; --- Disk Address Packet (DAP) for INT 0x13, AH=0x42 ---
align 4
DAP:
    db 0x10         ; Size of DAP (16 bytes)
    db 0            ; Unused (0)
DAP_SECTORS:
    dw 0            ; Number of sectors to read
DAP_OFFSET:
    dw 0            ; Offset to read into
DAP_SEGMENT:
    dw 0            ; Segment to read into
DAP_LBA_LOW:
    dd 0            ; LBA lower 32 bits
DAP_LBA_HIGH:
    dd 0            ; LBA upper 32 bits

; --- Include the GDT ---
%include "boot/gdt.asm"

; ==============================================================================
; 5. 32-BIT PROTECTED MODE ENTRY POINT
; ==============================================================================
[BITS 32]
protected_mode:
    ; Set data segments to flat 0x10
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000
    jmp 0x10000

; ==============================================================================
; BOOT SECTOR SIGNATURE
; ==============================================================================
times 510 - ($ - $$) db 0      
dw 0xAA55                      
