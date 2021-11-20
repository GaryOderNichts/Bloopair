#include "logger.h"
#include <stdarg.h>
#include <imports.h>
#include "socket.h"

static int log_socket = -1;

int log_init(void)
{
    if (socketInit() < 0) {
        return -1;
    }

    log_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (log_socket < 0) {
        return log_socket;
    }

    int enable_broadcast = 1;
    setsockopt(log_socket, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, 4);

    struct sockaddr_in connect_addr;
    memset(&connect_addr, 0, sizeof(connect_addr));
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = 4405;
    connect_addr.sin_addr.s_addr = INADDR_BROADCAST;

    if(connect(log_socket, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) < 0) {
        closesocket(log_socket);
        log_socket = -1;
    }

    log_printf("Bloopair logging init\n");

    return log_socket;
}

void log_deinit(void)
{
    if(log_socket >= 0) {
        closesocket(log_socket);
        log_socket = -1;
    }
}

void log_print(const char *str, int len)
{
    if(log_socket < 0 && log_init() < 0) {
        return;
    }

    int ret;
    while (len > 0) {
        int block = len < 1400 ? len : 1400; // take max 1400 bytes per UDP packet
        ret = send(log_socket, str, block, 0);
        if(ret < 0) {
            break;
        }

        len -= ret;
        str += ret;
    }
}

void log_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[0x100];

    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    log_print(buffer, len);

    va_end(args);
}

#ifdef VPRINTF_HOOK
int (*const real_vprintf)(const char* fmt, va_list arg) = (void*) 0x11f7efc8;
int vprintf_hook(const char* fmt, va_list arg)
{
    char buffer[0x100];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, arg);
    log_print(buffer, len);

    return real_vprintf(fmt, arg);
}
#endif
