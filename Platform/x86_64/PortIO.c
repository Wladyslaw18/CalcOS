#include "PortIO.h"

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(__inbyte)
#endif

void outb(uint16_t port, uint8_t val) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__ ("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
#else
    (void)port; (void)val;
#endif
}

uint8_t inb(uint16_t port) {
#if defined(__GNUC__) || defined(__clang__)
    uint8_t ret;
    __asm__ __volatile__ ("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
#else
    (void)port;
    return 0;
#endif
}

void outw(uint16_t port, uint16_t val) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__ ("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
#else
    (void)port; (void)val;
#endif
}

uint16_t inw(uint16_t port) {
#if defined(__GNUC__) || defined(__clang__)
    uint16_t ret;
    __asm__ __volatile__ ("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
#else
    (void)port;
    return 0;
#endif
}

void outl(uint16_t port, uint32_t val) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__ ("outl %k0, %w1" : : "a"(val), "Nd"(port) : "memory");
#else
    (void)port; (void)val;
#endif
}

uint32_t inl(uint16_t port) {
#if defined(__GNUC__) || defined(__clang__)
    uint32_t ret;
    __asm__ __volatile__ ("inl %w1, %k0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
#else
    (void)port;
    return 0;
#endif
}

void io_wait(void) {
#if defined(__GNUC__) || defined(__clang__)
    #if defined(__x86_64__) || defined(__i386__)
        uint8_t __dummy;
        __asm__ __volatile__ ("inb %w1, %b0" : "=a"(__dummy) : "Nd"(0xEB) : "memory");
    #else
        __asm__ __volatile__ ("" : : : "memory");
    #endif
#elif defined(_MSC_VER)
    #if defined(_M_IX86) || defined(_M_X64)
        __inbyte(0xEB);
    #else
        _ReadWriteBarrier();
    #endif
#else
    // Fallback block
#endif
}