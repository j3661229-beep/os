# ShunyaOS & Shunya Programming Language

Welcome to the **Shunya** project! This repository contains a custom-built programming language ("Shunya") and a bare-metal operating system ("ShunyaOS") designed specifically to run it.

We have successfully built a full-stack, from-scratch language ecosystem that runs across three entirely different environments:
1. **Bare-Metal OS** (Bootable via GRUB, runs without an underlying OS)
2. **Host CLI** (Native Linux/Windows executable)
3. **WebAssembly** (Browser-based interactive playground)

---

## 🌟 Project Milestones Completed

### 1. The Shunya Language Engine (C-based)
We built a custom interpreter from scratch in pure C, featuring:
- **Lexer/Scanner**: Tokenizes source code, handles numbers, strings, booleans, and complex operators (like `|>` pipeline).
- **Parser**: A recursive descent parser that builds an Abstract Syntax Tree (AST) handling precedence rules, blocks, and variable scopes.
- **Interpreter**: Evaluates the AST at runtime, manages variable environments, and executes logic.
- **Foreign Function Interface (FFI)**: A bridge that allows Shunya scripts to call native C functions (e.g., printing to screen, delaying execution).

### 2. The Desi / Fun Edition (Hinglish Support)
The language features a fully localized "Desi Edition" that maps Hinglish keywords to internal tokens, making it fun and localized to write!
- `mahavir` (let), `rakho` (const)
- `kaam` (func), `wapas` (return)
- `agar` (if), `warna` (else), `mila` (match)
- `ghuma` (loop), `jab_tak` (while)
- `dikha` (show), `badal` (to_string)
- Syntactic sugar boundaries: `aarambh` (start) and `khatam` (end)

### 3. Bare-Metal OS (ShunyaOS)
A custom 32-bit operating system that boots up and runs Shunya scripts natively.
- **Bootloader**: Multiboot-compliant assembly stub (`boot.asm`) that boots via GRUB.
- **VGA Driver**: Custom text-mode driver for colored screen output.
- **Keyboard Driver**: PS/2 keyboard driver via port I/O to read user input.
- **Hardware Timer**: Programmable Interval Timer (PIT) for precise delays (`sleep` function).
- **Memory Management**: Custom bare-metal heap allocator (`kmalloc` / `kfree`).

### 4. Standalone Host CLI (`make host`)
We adapted the engine to compile natively on Linux/WSL using standard GCC.
- Replaced bare-metal OS functions with standard C library equivalents (e.g., `printf`, `malloc`, `usleep`) using `#ifdef HOST_CLI`.
- Reads `.shunya` script files directly from the hard drive and executes them.
- Features `setjmp/longjmp` error recovery to catch parser errors gracefully without segfaulting.

### 5. WebAssembly Playground (`make web`)
We ported the entire C interpreter to the web using Emscripten!
- Compiles the engine to `vel.wasm` and `vel.js`.
- Custom `app.js` bridge to pass code from a browser text editor into the Wasm C engine.
- A beautiful, responsive, "Gen-Z" styled frontend with dark mode, neon glow effects, and interactive examples.
- Completely robust cache-busting implementation to ensure users always get the latest engine.

---

## 🚀 How to Run

### Run the Web Playground
```bash
# Start the local server
cd web
python3 server.py
# Open http://localhost:8080 in your browser
```

### Run the Host CLI
```bash
# Compile the native executable
make host
# Run a script
./vel test_desi.vel
```

### Run the Bare-Metal OS
```bash
# Compile the OS kernel and launch in QEMU emulator
make run
```

---

*This project is a complete showcase of compiler design, operating systems development, and WebAssembly integration!*
