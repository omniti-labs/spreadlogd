/* ======================================================================
 * Copyright (c) 2006 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#ifndef NETHELP_H
#define NETHELP_H

#include "sld_config.h"

typedef struct netlisten {
  struct event e;
  int fd;
  int mask;
  struct sockaddr_in address;
  void (*dispatch)(int, short, void *);
  void *userdata;
} netlisten_t;

int tcp_listen_on(const char *addr, unsigned short port, int backlog);
int tcp_dispatch(const char *addr, unsigned short port, int backlog, int mask,
                 void (*dispatch)(int, short, void *), void *userdata);


#if BYTE_ORDER == BIG_ENDIAN
static inline unsigned long long bswap64(unsigned long long _x) {
  return _x;
}
#elif BYTE_ORDER == LITTLE_ENDIAN
static inline unsigned long long bswap64(unsigned long long _x) {
  return ((_x >> 56) | ((_x >> 40) & 0xff00) | ((_x >> 24) & 0xff0000) |
    ((_x >> 8) & 0xff000000) |
    ((_x << 8) & ((unsigned long long)0xff << 32)) |
    ((_x << 24) & ((unsigned long long)0xff << 40)) |
    ((_x << 40) & ((unsigned long long)0xff << 48)) | ((_x << 56)));
}
#else
#error Unsupported byte order
#endif
#endif
