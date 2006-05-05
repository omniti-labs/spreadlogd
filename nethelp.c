/* ======================================================================
 * Copyright (c) 2006 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "sld_config.h"
#include "nethelp.h"

int tcp_listen_on(const char *addr, unsigned short port, int backlog) {
  struct sockaddr_in baddr;
  long on;
  int s;

  if(!addr) return -1;
  if(*addr == '*') baddr.sin_addr.s_addr = htonl(INADDR_ANY);
  else if(inet_pton(AF_INET, addr, &baddr.sin_addr) != 1) return -1;

  baddr.sin_port = htons(port);
  baddr.sin_family = AF_INET;

  s = socket(baddr.sin_family, SOCK_STREAM, 0);
  if(s < 0) return -1;
  
  on = 1;
  /* If there is an error... just continue on. */
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));

  on = 1;
  /* Set it into non-blocking state */
  if(ioctl(s, FIONBIO, &on)) {
    close(s);
    return -1;
  }

  if(bind(s, (struct sockaddr *)&baddr, sizeof(baddr)) == -1) {
    close(s);
    return -1;
  }
  if(listen(s, backlog) < 0) {
    close(s);
    return -1;
  }
  return s;
}

static void tcp_acceptor(int fd, short event, void *vnl) {
  socklen_t addrlen;
  netlisten_t *newnl, *nl = (netlisten_t *)vnl;

  addrlen = sizeof(nl->address);
  newnl = calloc(1, sizeof(*newnl));
  newnl->fd = accept(nl->fd, (struct sockaddr *)&newnl->address, &addrlen);
  newnl->dispatch = nl->dispatch;
  newnl->userdata = nl->userdata;
  if(newnl->fd < 0) {
    free(newnl);
    return;
  }
  event_set(&newnl->e, newnl->fd, nl->mask, newnl->dispatch, newnl);
  event_add(&newnl->e, NULL);
}

int tcp_dispatch(const char *addr, unsigned short port, int backlog,
                 int mask,
                 void (*dispatch)(int, short, void *), void *userdata) {
  int s;
  netlisten_t *nl;
  s = tcp_listen_on(addr, port, backlog);
  if(s < 0) return -1;
  nl = calloc(1, sizeof(*nl));
  nl->fd = s;
  nl->mask = mask;
  nl->dispatch = dispatch;
  nl->userdata = userdata;
  event_set(&nl->e, s, EV_READ|EV_PERSIST, tcp_acceptor, nl);
  event_add(&nl->e, NULL);
  return 0;
}
