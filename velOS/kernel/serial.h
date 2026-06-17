#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

/* Initialize COM1 */
void serial_init(void);

/* Interrupt handler for COM1 */
void serial_handler(void);

#endif /* SERIAL_H */
