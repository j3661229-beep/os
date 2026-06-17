#ifndef FS_H
#define FS_H

#include <stdint.h>

#define FS_MAX_FILES       64
#define FS_DIR_SECTOR      100
#define FS_DATA_SECTOR     110

typedef struct {
    char name[16];          /* Null-terminated, empty string if unused */
    uint32_t start_sector;
    uint32_t size_bytes;
} __attribute__((packed)) VelFile;

void fs_init(void);
int fs_read_file(const char* name, char* buffer);
int fs_write_file(const char* name, const char* data);
void fs_list_files(void);

#endif
