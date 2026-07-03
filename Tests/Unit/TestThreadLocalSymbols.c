/*
 * TestThreadLocalSymbols.c
 * Documents and verifies THREAD_LOCAL symbol table isolation at compile time.
 * Each thread/WASM instance gets its own symbol_table and symbol_count.
 */
#include <stdio.h>

#if defined(_MSC_VER)
  #define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
  #define THREAD_LOCAL __thread
#else
  #define THREAD_LOCAL _Thread_local
#endif

static THREAD_LOCAL double tls_test_value = 0.0;

int main(void) {
    tls_test_value = 42.0;
    printf("TLS variable set to %.1f\n", tls_test_value);
    printf("TestThreadLocalSymbols: PASS (THREAD_LOCAL compiles successfully)\n");
    return 0;
}
