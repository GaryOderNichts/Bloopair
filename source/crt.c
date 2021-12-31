void __init_wut_malloc();

void __init_wut_newlib();

void __init_wut_stdcpp();

void __init_wut_devoptab();

void __attribute__((weak)) __init_wut_socket();

void __fini_wut_malloc();

void __fini_wut_newlib();

void __fini_wut_stdcpp();

void __fini_wut_devoptab();

void __attribute__((weak)) __fini_wut_socket();

void __attribute__((weak))
__init_wut_() {
    __init_wut_malloc();
    __init_wut_newlib();
    __init_wut_stdcpp();
    __init_wut_devoptab();
    if (&__init_wut_socket) __init_wut_socket();
}

void __attribute__((weak))
__fini_wut_() {
    __fini_wut_devoptab();
    __fini_wut_stdcpp();
    __fini_wut_newlib();
    __fini_wut_malloc();
}
