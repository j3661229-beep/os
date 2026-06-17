#include "fs.h"
#include "ide.h"
#include "vel/libc_compat.h"

extern void vga_print(const char* str, char color);
extern void vga_print_hex(unsigned int n);

static VelFile directory[FS_MAX_FILES];
static uint32_t next_free_sector = FS_DATA_SECTOR;

/* Helper to write the directory table back to disk */
static void fs_sync_dir(void) {
    /* 64 files * 24 bytes = 1536 bytes (3 sectors) */
    ide_write_sectors(FS_DIR_SECTOR, 3, directory);
}

void fs_init(void) {
    vga_print("[FS] Initializing VelFS...\n", 0x0A);
    ide_init();
    
    /* Read directory from disk */
    ide_read_sectors(FS_DIR_SECTOR, 3, directory);
    
    /* Calculate next_free_sector by finding the file that ends furthest on disk */
    next_free_sector = FS_DATA_SECTOR;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (directory[i].name[0] != '\0') {
            uint32_t sectors_used = (directory[i].size_bytes + 511) / 512;
            uint32_t end_sector = directory[i].start_sector + sectors_used;
            if (end_sector > next_free_sector) {
                next_free_sector = end_sector;
            }
        }
    }
}

int fs_write_file(const char* name, const char* data) {
    int size = strlen(data);
    int sectors_needed = (size + 511) / 512;
    
    /* Find existing file or empty slot */
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            /* If existing file, we just append to the end of the disk anyway 
               because we don't have fragmentation management yet. */
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (directory[i].name[0] == '\0') {
                slot = i;
                break;
            }
        }
    }
    
    if (slot == -1) {
        vga_print("FS Error: Directory full\n", 0x0C);
        return -1;
    }
    
    /* Setup file metadata */
    strncpy(directory[slot].name, name, 15);
    directory[slot].name[15] = '\0';
    directory[slot].start_sector = next_free_sector;
    directory[slot].size_bytes = size;
    
    /* Write file data */
    /* We need to pad the data to 512-byte boundaries since IDE writes full sectors */
    char* padded_buffer = (char*)malloc(sectors_needed * 512);
    memset(padded_buffer, 0, sectors_needed * 512);
    memcpy(padded_buffer, data, size);
    
    ide_write_sectors(next_free_sector, sectors_needed, padded_buffer);
    free(padded_buffer);
    
    next_free_sector += sectors_needed;
    
    /* Update directory on disk */
    fs_sync_dir();
    
    return 0;
}

int fs_read_file(const char* name, char* buffer) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            int sectors = (directory[i].size_bytes + 511) / 512;
            
            char* temp_buffer = (char*)malloc(sectors * 512);
            ide_read_sectors(directory[i].start_sector, sectors, temp_buffer);
            
            /* Copy only the exact file bytes */
            memcpy(buffer, temp_buffer, directory[i].size_bytes);
            buffer[directory[i].size_bytes] = '\0';
            
            free(temp_buffer);
            return directory[i].size_bytes;
        }
    }
    return -1; /* Not found */
}

void fs_list_files(void) {
    vga_print("Files on disk:\n", 0x0E);
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (directory[i].name[0] != '\0') {
            vga_print("  ", 0x0F);
            vga_print(directory[i].name, 0x0F);
            vga_print(" (", 0x07);
            vga_print_hex(directory[i].size_bytes);
            vga_print(" bytes)\n", 0x07);
            count++;
        }
    }
    if (count == 0) {
        vga_print("  (Empty)\n", 0x07);
    }
}
