// src/kernel/app_registry.c
#include "../../include/kernel/app.h"

// Define boundaries for Microsoft compiler link-time sections
#if defined(_MSC_VER)
    DECLARE_APP_START const Application* const __appreg_start = (void*)0;
    DECLARE_APP_END   const Application* const __appreg_end   = (void*)0;
#endif
