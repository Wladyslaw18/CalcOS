#ifndef SERIAL_IO_H
#define SERIAL_IO_H

#include <stdint.h>

#define PORT_COM1 0x3F8

void serial_init(void);
int serial_received(void);
char serial_read(void);
int is_transmit_empty(void);
void serial_write_char(char a);
void serial_write_str(const char* str);

#endif // SERIAL_IO_H
