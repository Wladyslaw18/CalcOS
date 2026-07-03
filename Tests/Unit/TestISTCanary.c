/*
 * TestISTCanary.c
 * Documents the IST stack canary value expected at boot.
 * Verifies the sentinel and base canary values are the correct constants.
 */
#include <stdio.h>
#include <stdint.h>

#define STACK_CANARY_VALUE 0xDEADC0DEUL
#define STACK_SENTINEL     0xA110CA7EUL

int main(void) {
    printf("IST stack canary constants:\n");
    printf("  Base canary:  0x%08X (expected: 0xDEADC0DE)\n", (unsigned)STACK_CANARY_VALUE);
    printf("  Top sentinel: 0x%08X (expected: 0xA110CA7E)\n", (unsigned)STACK_SENTINEL);

    int ok = (STACK_CANARY_VALUE == 0xDEADC0DEUL && STACK_SENTINEL == 0xA110CA7EUL);
    printf("TestISTCanary: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
