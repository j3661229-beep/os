#include "ide.h"
#include "io.h"

extern void vga_print(const char* str, char color);
extern void vga_print_hex(unsigned int n);

#define IDE_PORT_BASE   0x1F0
#define IDE_DATA        (IDE_PORT_BASE + 0)
#define IDE_SEC_COUNT   (IDE_PORT_BASE + 2)
#define IDE_LBA_LO      (IDE_PORT_BASE + 3)
#define IDE_LBA_MID     (IDE_PORT_BASE + 4)
#define IDE_LBA_HI      (IDE_PORT_BASE + 5)
#define IDE_DRIVE       (IDE_PORT_BASE + 6)
#define IDE_CMD_STATUS  (IDE_PORT_BASE + 7)

#define IDE_CMD_READ    0x20
#define IDE_CMD_WRITE   0x30
#define IDE_CMD_FLUSH   0xE7

#define IDE_STATUS_ERR  0x01
#define IDE_STATUS_DRQ  0x08
#define IDE_STATUS_DRDY 0x40
#define IDE_STATUS_BSY  0x80

static void ide_wait_bsy(void) {
    while (inb(IDE_CMD_STATUS) & IDE_STATUS_BSY);
}

static void ide_wait_drq(void) {
    while (!(inb(IDE_CMD_STATUS) & IDE_STATUS_DRQ));
}

void ide_init(void) {
    vga_print("[IDE] Initializing Primary ATA bus...\n", 0x0A);
    /* In a real OS we'd use IDENTIFY to check for presence/capabilities.
       For VelOS, we assume QEMU sets up the Primary Master as IDE Hard Drive. */
}

int ide_read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    ide_wait_bsy();
    
    outb(IDE_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_SEC_COUNT, count);
    outb(IDE_LBA_LO, (uint8_t)lba);
    outb(IDE_LBA_MID, (uint8_t)(lba >> 8));
    outb(IDE_LBA_HI, (uint8_t)(lba >> 16));
    
    outb(IDE_CMD_STATUS, IDE_CMD_READ);
    
    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ide_wait_bsy();
        ide_wait_drq();
        for (int j = 0; j < 256; j++) {
            *ptr++ = inw(IDE_DATA);
        }
    }
    return 0;
}

int ide_write_sectors(uint32_t lba, uint8_t count, const void* buffer) {
    ide_wait_bsy();
    
    outb(IDE_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_SEC_COUNT, count);
    outb(IDE_LBA_LO, (uint8_t)lba);
    outb(IDE_LBA_MID, (uint8_t)(lba >> 8));
    outb(IDE_LBA_HI, (uint8_t)(lba >> 16));
    
    outb(IDE_CMD_STATUS, IDE_CMD_WRITE);
    
    const uint16_t* ptr = (const uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ide_wait_bsy();
        ide_wait_drq();
        for (int j = 0; j < 256; j++) {
            outw(IDE_DATA, *ptr++);
        }
    }
    
    /* Flush Cache */
    outb(IDE_CMD_STATUS, IDE_CMD_FLUSH);
    ide_wait_bsy();
    
    return 0;
}
