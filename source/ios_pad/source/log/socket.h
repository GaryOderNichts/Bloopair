#pragma once

#include <stdint.h>
#include <stdio.h>

#define SOL_SOCKET    0xFFFF

#define PF_UNSPEC    0
#define PF_INET      2
#define PF_INET6     10

#define AF_UNSPEC   PF_UNSPEC
#define AF_INET     PF_INET
#define AF_INET6    PF_INET6

#define SOCK_STREAM   1
#define SOCK_DGRAM    2

#define MSG_CTRUNC      0x01000000
#define MSG_DONTROUTE   0x02000000
#define MSG_EOR         0x04000000
#define MSG_OOB         0x08000000
#define MSG_PEEK        0x10000000
#define MSG_TRUNC       0x20000000
#define MSG_WAITALL     0x40000000

#define SHUT_RD        0
#define SHUT_WR        1
#define SHUT_RDWR      2

#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_USELOOPBACK  0x0040
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100
#define SO_REUSEPORT    0x0200
#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_SNDTIMEO     0x1005
#define SO_RCVTIMEO     0x1006
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008

#define INADDR_ANY        0x00000000
#define INADDR_BROADCAST  0xFFFFFFFF
#define INADDR_NONE       0xFFFFFFFF

#define INET_ADDRSTRLEN        16

#define INADDR_LOOPBACK  0x7f000001
#define INADDR_ANY       0x00000000
#define INADDR_BROADCAST 0xFFFFFFFF
#define INADDR_NONE      0xFFFFFFFF

#define INET_ADDRSTRLEN  16

#define IPPROTO_IP          0               /* dummy for IP */
#define IPPROTO_UDP        17               /* user datagram protocol */
#define IPPROTO_TCP         6               /* tcp */

#define IP_TOS              7
#define IP_TTL              8
#define IP_MULTICAST_LOOP   9
#define IP_MULTICAST_TTL   10
#define IP_ADD_MEMBERSHIP  11
#define IP_DROP_MEMBERSHIP 12

typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[];
};

struct sockaddr_storage {
    sa_family_t ss_family;
    char __ss_padding[14];
};

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;
    unsigned char sin_zero[8];
};

struct linger {
    int l_onoff;
    int l_linger;
};

int socketInit(void);

int socketExit(void);

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int closesocket(int sockfd);

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int listen(int sockfd, int backlog);

ssize_t recv(int sockfd, void *buf, size_t len, int flags);

ssize_t send(int sockfd, const void *buf, size_t len, int flags);

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

int shutdown(int sockfd, int how);

int socket(int domain, int type, int protocol);
