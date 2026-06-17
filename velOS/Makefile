# ==============================================================================
# Makefile - VelOS Build System
#
# Role: Automates the process of assembling, compiling, linking, and packaging
# our OS into a bootable disk image. Also provides a command to run it in QEMU.
# ==============================================================================

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel

# Tools
ASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy
QEMU = qemu-system-i386

# Flags
# -m32: Compile for 32-bit x86
# -ffreestanding: Tells GCC we have no standard library
# -fno-builtin: Disables GCC built-in functions
# -nostdlib: Do not link the standard C library
CFLAGS = -m32 -ffreestanding -fno-builtin -nostdlib -Wall -Wextra

# All target - Builds and runs the OS
all: build run

# Build target - Compiles everything and creates the disk image
build: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/entry.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/isr_asm.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/isr.o 	$(BUILD_DIR)/pic.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/pmm.o \
	$(BUILD_DIR)/paging.o $(BUILD_DIR)/heap.o $(BUILD_DIR)/panic.o \
	$(BUILD_DIR)/ide.o $(BUILD_DIR)/fs.o \
	$(BUILD_DIR)/editor.o $(BUILD_DIR)/graphics.o $(BUILD_DIR)/font8x8.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/random.o $(BUILD_DIR)/sound.o $(BUILD_DIR)/serial.o \
	$(BUILD_DIR)/shell.o $(BUILD_DIR)/libc_compat.o \
	$(BUILD_DIR)/lexer.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/interpreter.o \
	$(BUILD_DIR)/kernel.o
	# 5. Link the kernel object files into a single ELF executable
	$(LD) -m elf_i386 -T linker.ld -o $(BUILD_DIR)/kernel.elf \
		$(BUILD_DIR)/entry.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/isr_asm.o $(BUILD_DIR)/idt.o \
		$(BUILD_DIR)/isr.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/keyboard.o \
		$(BUILD_DIR)/pmm.o $(BUILD_DIR)/paging.o $(BUILD_DIR)/heap.o $(BUILD_DIR)/panic.o \
		$(BUILD_DIR)/ide.o $(BUILD_DIR)/fs.o \
		$(BUILD_DIR)/editor.o $(BUILD_DIR)/graphics.o $(BUILD_DIR)/font8x8.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/random.o $(BUILD_DIR)/sound.o $(BUILD_DIR)/serial.o \
		$(BUILD_DIR)/shell.o $(BUILD_DIR)/libc_compat.o $(BUILD_DIR)/lexer.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/interpreter.o \
		$(BUILD_DIR)/kernel.o
	
	# 6. Extract raw binary from the ELF executable (strip symbols/metadata)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin
	
	# 7. Create a proper 1.44MB floppy disk image
	# Step A: Create a blank 16MB image filled with zeros
	#         16MB = 32768 sectors of 512 bytes each
	dd if=/dev/zero of=disk.img bs=512 count=32768 status=none
	# Step B: Stamp the bootloader at sector 0 (first 512 bytes)
	dd if=$(BUILD_DIR)/boot.bin of=disk.img conv=notrunc status=none
	# Step C: Stamp the kernel starting at sector 1 (offset 512)
	dd if=$(BUILD_DIR)/kernel.bin of=disk.img bs=512 seek=1 conv=notrunc status=none

# 1. Assemble the Stage 1 Bootloader (Flat binary)
$(BUILD_DIR)/boot.bin: $(BOOT_DIR)/boot.asm $(BOOT_DIR)/gdt.asm
	$(ASM) -f bin $(BOOT_DIR)/boot.asm -o $(BUILD_DIR)/boot.bin

# 2. Assemble the GDT (ELF32 Object)
$(BUILD_DIR)/gdt.o: $(BOOT_DIR)/gdt.asm
	$(ASM) -f elf32 $(BOOT_DIR)/gdt.asm -o $(BUILD_DIR)/gdt.o

# 3. Assemble the Kernel Entry Point (ELF32 Object)
$(BUILD_DIR)/entry.o: $(KERNEL_DIR)/kernel_entry.asm
	$(ASM) -f elf32 $(KERNEL_DIR)/kernel_entry.asm -o $(BUILD_DIR)/entry.o

# Assemble the ISR Stubs
$(BUILD_DIR)/isr_asm.o: $(KERNEL_DIR)/isr.asm
	$(ASM) -f elf32 $(KERNEL_DIR)/isr.asm -o $(BUILD_DIR)/isr_asm.o

# Compile the IDT C code
$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/idt.c -o $(BUILD_DIR)/idt.o

# Compile the ISR C code
$(BUILD_DIR)/isr.o: $(KERNEL_DIR)/isr.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/isr.c -o $(BUILD_DIR)/isr.o

# Compile the PIC C code
$(BUILD_DIR)/pic.o: $(KERNEL_DIR)/pic.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/pic.c -o $(BUILD_DIR)/pic.o

# Compile the Keyboard Driver
$(BUILD_DIR)/keyboard.o: $(KERNEL_DIR)/keyboard.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/keyboard.c -o $(BUILD_DIR)/keyboard.o

# Compile the Memory Management files
$(BUILD_DIR)/pmm.o: $(KERNEL_DIR)/pmm.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/pmm.c -o $(BUILD_DIR)/pmm.o

$(BUILD_DIR)/paging.o: $(KERNEL_DIR)/paging.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/paging.c -o $(BUILD_DIR)/paging.o

$(BUILD_DIR)/heap.o: $(KERNEL_DIR)/heap.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/heap.c -o $(BUILD_DIR)/heap.o

$(BUILD_DIR)/panic.o: kernel/panic.c
	$(CC) $(CFLAGS) -c kernel/panic.c -o $(BUILD_DIR)/panic.o

$(BUILD_DIR)/ide.o: kernel/ide.c
	$(CC) $(CFLAGS) -c kernel/ide.c -o $(BUILD_DIR)/ide.o

$(BUILD_DIR)/fs.o: kernel/fs.c
	$(CC) $(CFLAGS) -c kernel/fs.c -o $(BUILD_DIR)/fs.o

$(BUILD_DIR)/editor.o: kernel/editor.c
	$(CC) $(CFLAGS) -c kernel/editor.c -o $(BUILD_DIR)/editor.o

$(BUILD_DIR)/graphics.o: kernel/graphics.c
	$(CC) $(CFLAGS) -c kernel/graphics.c -o $(BUILD_DIR)/graphics.o

$(BUILD_DIR)/font8x8.o: kernel/font8x8.c
	$(CC) $(CFLAGS) -c kernel/font8x8.c -o $(BUILD_DIR)/font8x8.o

$(BUILD_DIR)/timer.o: kernel/timer.c
	$(CC) $(CFLAGS) -c kernel/timer.c -o $(BUILD_DIR)/timer.o

$(BUILD_DIR)/random.o: kernel/random.c
	$(CC) $(CFLAGS) -c kernel/random.c -o $(BUILD_DIR)/random.o

$(BUILD_DIR)/sound.o: kernel/sound.c
	$(CC) $(CFLAGS) -c kernel/sound.c -o $(BUILD_DIR)/sound.o

$(BUILD_DIR)/serial.o: kernel/serial.c
	$(CC) $(CFLAGS) -c kernel/serial.c -o $(BUILD_DIR)/serial.o

# Compile Shell and Vel components
$(BUILD_DIR)/shell.o: $(KERNEL_DIR)/shell.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/shell.c -o $(BUILD_DIR)/shell.o

$(BUILD_DIR)/libc_compat.o: $(KERNEL_DIR)/vel/libc_compat.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/vel/libc_compat.c -o $(BUILD_DIR)/libc_compat.o

$(BUILD_DIR)/lexer.o: $(KERNEL_DIR)/vel/lexer.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/vel/lexer.c -o $(BUILD_DIR)/lexer.o

$(BUILD_DIR)/parser.o: $(KERNEL_DIR)/vel/parser.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/vel/parser.c -o $(BUILD_DIR)/parser.o

$(BUILD_DIR)/interpreter.o: $(KERNEL_DIR)/vel/interpreter.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/vel/interpreter.c -o $(BUILD_DIR)/interpreter.o

# 4. Compile the C Kernel (ELF32 Object)
$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/kernel.c -o $(BUILD_DIR)/kernel.o

# Run target - Boots our disk image in QEMU
run:
	$(QEMU) -drive format=raw,file=disk.img,if=ide -serial stdio

# Clean target - Removes all generated files
clean:
	rm -f $(BUILD_DIR)/*
	rm -f disk.img

# ==============================================================================
# HOST CLI TARGET - Compile Vel as a native Linux executable
# ==============================================================================
VEL_SRCS = $(KERNEL_DIR)/vel/lexer.c $(KERNEL_DIR)/vel/parser.c \
           $(KERNEL_DIR)/vel/interpreter.c $(KERNEL_DIR)/vel/libc_compat.c

host:
	gcc -O2 -Wall -DHOST_CLI \
		$(VEL_SRCS) \
		$(KERNEL_DIR)/vel/host_main.c \
		-o vel -lm
	@echo "Built: ./vel <script.vel>"

# ==============================================================================
# WEB PLAYGROUND TARGET - Compile Vel to WebAssembly via Emscripten
# ==============================================================================
web:
	@mkdir -p /tmp/vel_web_build
	emcc -O2 -DHOST_CLI \
		$(VEL_SRCS) \
		web/web_main.c \
		-o /tmp/vel_web_build/vel.js \
		-s EXPORTED_FUNCTIONS='["_run_vel_code","_reset_vel_env"]' \
		-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s MODULARIZE=0
	cp /tmp/vel_web_build/vel.js web/vel.js
	cp /tmp/vel_web_build/vel.wasm web/vel.wasm
	rm -rf /tmp/vel_web_build
	@echo "Built: web/vel.js + web/vel.wasm"
	@echo "Open web/index.html in a browser to use the playground!"

clean-host:
	rm -f vel

clean-web:
	rm -f web/vel.js web/vel.wasm

.PHONY: all build run clean host web clean-host clean-web

