/* ==============================================================================
 * paging.c - Virtual Memory Management
 *
 * Role: Enables the x86 hardware paging unit. This translates virtual memory
 * addresses into physical RAM frames. We statically allocate the Page Directory
 * so it exists securely inside our kernel's BSS section, and dynamically
 * allocate Page Tables using the PMM.
 * ============================================================================== */

#include "paging.h"
#include "pmm.h"

extern void panic(const char* msg);
extern void vga_print(const char* str, char color);
extern void vga_print_hex(unsigned int n);

/* Statically allocate the Page Directory aligned to 4KB */
static PageDirectory kernel_directory;

/**
 * paging_map - Maps a Virtual Address to a Physical Address
 */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    /* 1. Extract the 10-bit Page Directory index (top 10 bits of virtual address) */
    uint32_t pd_idx = virt >> 22;

    /* 2. Extract the 10-bit Page Table index (middle 10 bits of virtual address) */
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    /* 3. Get the Page Directory entry */
    PageEntry* pd_entry = &kernel_directory.entries[pd_idx];

    PageTable* table;

    /* 4. Check if the Page Table exists (is the Present bit set?) */
    if (!(*pd_entry & PAGE_PRESENT)) {
        /* Page table doesn't exist, we must allocate a new 4KB physical frame for it */
        uint32_t new_table_phys = pmm_alloc_frame();
        
        /* Because we are currently identity mapping, physical == virtual for the kernel space.
         * So we can just cast the physical address to a pointer and clear it. */
        table = (PageTable*)new_table_phys;
        
        for (int i = 0; i < 1024; i++) {
            table->entries[i] = 0; /* Zero out all entries */
        }

        /* 5. Insert the new Page Table into the Page Directory.
         * The address must be shifted left by 12 (or just bitwise OR'd since it's 4KB aligned),
         * combined with the flags (Present, Writable, User/Supervisor). */
        *pd_entry = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    } else {
        /* Page table already exists, extract its address by masking out the bottom 12 bits */
        table = (PageTable*)(*pd_entry & 0xFFFFF000);
    }

    /* 6. Finally, set the entry inside the Page Table to point to the actual physical frame */
    table->entries[pt_idx] = (phys & 0xFFFFF000) | flags;
}

/**
 * paging_unmap - Removes a virtual page mapping
 */
void paging_unmap(uint32_t virt) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    PageEntry* pd_entry = &kernel_directory.entries[pd_idx];

    /* If the Page Table is present, clear the specific entry */
    if (*pd_entry & PAGE_PRESENT) {
        PageTable* table = (PageTable*)(*pd_entry & 0xFFFFF000);
        table->entries[pt_idx] = 0;
        
        /* Note: Flush TLB (Translation Lookaside Buffer) using inline assembly
         * to force the CPU to forget the cached mapping. */
        asm volatile("invlpg (%0)" : : "r" (virt) : "memory");
    }
}

/**
 * virt_to_phys - Translates a virtual address to physical by walking the tables
 */
uint32_t virt_to_phys(uint32_t virt) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;
    uint32_t offset = virt & 0xFFF;

    PageEntry pd_entry = kernel_directory.entries[pd_idx];
    if (pd_entry & PAGE_PRESENT) {
        PageTable* table = (PageTable*)(pd_entry & 0xFFFFF000);
        PageEntry pt_entry = table->entries[pt_idx];
        if (pt_entry & PAGE_PRESENT) {
            return (pt_entry & 0xFFFFF000) + offset;
        }
    }
    return 0; /* Not mapped */
}

/**
 * paging_init - Sets up the Page Directory and enables CPU Paging
 */
void paging_init() {
    /* 1. Zero out the Page Directory initially */
    for (int i = 0; i < 1024; i++) {
        kernel_directory.entries[i] = 0;
    }

    /* 2. Identity Map the first 16MB of RAM.
     * Identity mapping means Virtual Address == Physical Address.
     * We need 16MB / 4KB = 4096 pages total. */
    uint32_t flags = PAGE_PRESENT | PAGE_WRITABLE;
    
    for (uint32_t i = 0; i < TOTAL_MEMORY; i += PAGE_SIZE) {
        paging_map(i, i, flags);
    }

    /* 3. Load the physical address of our Page Directory into the CR3 register.
     * CR3 is the PDBR (Page Directory Base Register). */
    uint32_t pd_addr = (uint32_t)&kernel_directory;
    asm volatile("mov %0, %%cr3" : : "r"(pd_addr));

    /* 4. Enable Paging by setting the PG bit (bit 31) in the CR0 register. */
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Set PG bit */
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

/**
 * page_fault_handler - Called by the IDT when a page fault occurs
 */
void page_fault_handler(Registers* r) {
    /* The CPU automatically stores the faulting virtual address in register CR2 */
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    /* Decode error code bits */
    int present   = !(r->err_code & 0x1); /* 0 = Page not present */
    int rw        = r->err_code & 0x2;    /* 1 = Write operation caused fault */
    int user      = r->err_code & 0x4;    /* 1 = User mode caused fault */

    vga_print("\nPAGE FAULT at virt=", 0x04);
    vga_print_hex(faulting_address);
    vga_print(" err=", 0x04);
    vga_print_hex(r->err_code);
    
    if (present) vga_print(" [Not Present]", 0x04);
    if (rw) vga_print(" [Write Fault]", 0x04);
    if (user) vga_print(" [User Mode]", 0x04);
    
    panic("Page Fault Exception Unrecoverable!");
}
