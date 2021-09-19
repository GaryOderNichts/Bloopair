#pragma once

int log_init(void);

void log_deinit(void);

void log_print(const char *str, int len);

void log_printf(const char *format, ...);
