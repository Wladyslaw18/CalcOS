/*
 * File: IDT.h
 * Author: W. Kowal
 * Description: Interrupt Descriptor Table initialisation.
 * 
 * WITHOUT THIS, any CPU exception on real hardware causes an instant
 * Triple Fault and reboot no error message, no diagnosis.
 * This provides proper exception handlers that print register state
 * to VGA and COM1 before halting.
 * 
 * Handlers implemented:
 * 0 - #DE - Division Error
 * 6 - #UD - Invalid Opcode
 * 8 - #DF - Double Fault (via IST)
 * 13 - #GP - General Protection Fault
 * 14 - #PF - Page Fault
 */

#ifndef CALC_IDT_H
#define CALC_IDT_H

#include <stdint.h>

// Initialize the IDT with stub exception handlers.
// Sets up IST entries for Double Fault (#DF) to prevent triple fault cascade.
void idt_init(void);

#endif // CALC_IDT_H
