/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sp.h>
#include <errno.h>

#include "config.h"

#define SPREADLOGD_VERSION "1.5.0"

#define _TODO_JOIN 1
#define _TODO_PARANOID_CONNECT 2

#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

extern char *optarg;
extern int optind, opterr, optopt;

int verbose = 0;
int extralog = 0;
int terminate = 0;
int huplogs = 0;
int skiplocking = 0;
int buffsize = -1;
SpreadConfiguration **fds;
int fdsetsize;
int nr_open;

static char *default_configfile = "/etc/spreadlogd.conf";

void usage(char *progname) {
  fprintf(stderr, "%s\t\tVERSION: %s\n \
\t-c configfile\t\t[default /etc/spreadlogd.conf]\n \
\t-s\t\t\tskip locking (flock) files (NOT RECOMMENDED)\n \
\t-v\t\t\tverbose mode\n \
\t-x\t\t\tlog errors talking with spread\n \
\t-D\t\t\tdo not daemonize (debug)\n \
\t-V\t\t\tshow version information\n", progname, SPREADLOGD_VERSION);
  exit(1);
}

void sig_handler(int signum) {
  /* Set a "hup my logs" flag */
  if(signum == SIGHUP)
    huplogs = 1;
  else if(signum == SIGTERM)
    terminate = 1;
}

int join(LogFacility *lf, void *vpfd) {
  int ret;
  int fd = *(int *)vpfd;
  if((ret = SP_join(fd, lf->groupname)) == 0) {
    if(verbose)
      fprintf(stderr, "Joined %s.\n", lf->groupname);
  }
  return ret;
}

int connectandjoin(SpreadConfiguration *sc, void *uv) {
  int mbox;
  int *todo = (int *)uv;
  char sld[MAX_GROUP_NAME];
  snprintf(sld, MAX_GROUP_NAME, "sld-%05d", getpid());
  if(sc->connected && *todo & _TODO_PARANOID_CONNECT) {
    /* We _think_ we are connected, but want to really check */
    int i;
    int retval = 0;
    struct timeval timeout;
    fd_set readset, exceptset;

    for(i=0;i<fdsetsize;i++)
      if(fds[i] == sc)
	break;
    if(i>=fdsetsize) {
      sc->connected = 0;
    } else {
      timeout.tv_sec = 0L;
      timeout.tv_usec = 0L;
      FD_ZERO(&readset); FD_SET(i, &readset);
      FD_ZERO(&exceptset); FD_SET(i, &exceptset);

      retval = select(fdsetsize, &readset, NULL, &exceptset, &timeout);
      if(retval < 0 && errno == EBADF) {
        sc->connected = 0;
        fds[i] = NULL;
      }
    }
  }
  if(sc->connected || SP_connect(config_get_spreaddaemon(sc),
				 sld, 1, 0, &mbox,
				 sc->private_group) == ACCEPT_SESSION) {
    if(!sc->connected) {
      if(verbose)
	fprintf(stderr, "Successfully connected to spread at %s%c%s\n",
		(sc->host)?sc->host:"/tmp",
		(sc->host)?':':'/',
		sc->port);
      fds[mbox] = sc;
    }
    sc->connected = 1;
    if(*todo & _TODO_JOIN)
      config_foreach_logfacility(sc, join, &mbox);
    return mbox;
  } else {
    if(verbose)
      fprintf(stderr, "Failed connection to spread at %s%c%s\n",
	      (sc->host)?sc->host:"/tmp",
	      (sc->host)?':':'/',
	      sc->port);
    sc->connected = 0;
  }
  return -1;  
}

int paranoid_establish_spread_connections() {
  int tojoin = _TODO_JOIN | _TODO_PARANOID_CONNECT;
  return config_foreach_spreadconf(connectandjoin, (void *)&tojoin);
}
int establish_spread_connections() {
  int tojoin = _TODO_JOIN;
  return config_foreach_spreadconf(connectandjoin, (void *)&tojoin);
}

void handle_signals() {
  if(terminate) {
    if(extralog) fprintf(stderr, "Received SIGTERM, closing log files.\n");
    config_close();
    if(extralog) fprintf(stderr, "Log files closed, exiting.\n");
    exit(0);
  }
  if(huplogs) {
    huplogs = 0;
    config_hup();
  }
}
void daemonize(void) {
  if(fork()!=0) exit(0);
  setsid();
  if(fork()!=0) exit(0);
}
int getnropen(void) {
  struct rlimit rlim;
  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim.rlim_cur = rlim.rlim_max;
  setrlimit(RLIMIT_NOFILE, &rlim);
  return rlim.rlim_cur;
}

int main(int argc, char **argv) {
  char *configfile = default_configfile;
  char *message;
  int getoption, debug = 0;
  struct sigaction signalaction;
  sigset_t ourmask;
  nr_open = getnropen();

  fdsetsize = FD_SETSIZE;
  fds = (SpreadConfiguration **)malloc(sizeof(SpreadConfiguration *)*
				       fdsetsize);
  memset(fds, 0, sizeof(SpreadConfiguration *)*fdsetsize);

  while((getoption = getopt(argc, argv, "b:c:svxDV")) != -1) {
    switch(getoption) {
    case 'b':
      buffsize = atoi(optarg);
    case 'c':
      configfile = optarg;
      break;
    case 's':
      skiplocking = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'x':
      extralog = 1;
      break;
    case 'D':
      debug = 1;
      break;
    default:
      usage(argv[0]);
    }
  }
  
  /* Read our configuration */
  if(config_init(configfile)) exit(-1);

  if(buffsize<0) buffsize = 1024*8; /* 8k buffer (like Apache) */
  message = (char *)malloc(buffsize*sizeof(char));

  if(verbose) {
    fprintf(stderr, "running spreadlogd as %s\n\tconfigfile:\t\t%s\n\tdebug:\t\t%s\n\tverbose:\t\t%s\n\tlog spread errors:\t%s\n\tbuffer size:\t\t%d\n",
	    argv[0],
	    configfile,
            (debug)?"YES":"NO",
	    (verbose)?"YES":"NO",
	    (extralog)?"YES":"NO",
	    buffsize);
  }

  if(!debug) daemonize();
 
  /* Set up HUP signal */
  signalaction.sa_handler = sig_handler;
  sigemptyset(&signalaction.sa_mask);
  signalaction.sa_flags = 0;
  if(sigaction(SIGHUP, &signalaction, NULL)) {
    fprintf(stderr, "An error occured while registering a SIGHUP handler\n");
    perror("sigaction");
  }
  if(sigaction(SIGTERM, &signalaction, NULL)) {
    fprintf(stderr, "An error occured while registering a SIGTERM handler\n");
    perror("sigaction");
  }
  sigemptyset(&ourmask);
  sigaddset(&ourmask, SIGHUP);
  sigprocmask(SIG_UNBLOCK, &ourmask, NULL);

  /* Connect to spread */
  while(1) {
    int fd, tojoin;
    fd_set readset, exceptset, masterset;
    sp_time lasttry, thistry, timediff;
    service service_type;
    char sender[MAX_GROUP_NAME];
    char *pmessage;
    int len, num_groups, endian, logfd;
    char groups[1][MAX_GROUP_NAME];
    int16 mess_type;
#ifdef DROP_RECV
    service_type = DROP_RECV;
#endif

    establish_spread_connections();
    FD_ZERO(&masterset);
    for(fd=0;fd<fdsetsize;fd++) {
      if(fds[fd]) {
	FD_SET(fd, &masterset);
      }
    }
    lasttry = E_get_time();
    while(1) {
      /* Build out select */
      struct timeval timeout;
      int i;

      memcpy(&readset, &masterset, sizeof(fd_set));
      memcpy(&exceptset, &masterset, sizeof(fd_set));
      timeout.tv_sec = 1L;
      timeout.tv_usec = 0L;
      if(select(fdsetsize, &readset, NULL, &exceptset, &timeout) > 0) {
	for(fd=0;fd<fdsetsize;fd++)
	  if(FD_ISSET(fd, &readset) || FD_ISSET(fd, &exceptset)) {
	    len = SP_receive(fd, &service_type, sender,
			     1, &num_groups, groups,
			     &mess_type, &endian, buffsize, message);
	    /* Handle errors correctly */
	    if(len == ILLEGAL_SESSION || len == CONNECTION_CLOSED ||
	       len == ILLEGAL_MESSAGE || len == BUFFER_TOO_SHORT) {
	      if(extralog) {
		fprintf(stderr, "Error receiving from spread:\n\t");
		SP_error(len);
	      }
	      /* These are errors that require reestablishing a connection */
	      if(len == ILLEGAL_SESSION || len == CONNECTION_CLOSED) {
		/* So, let's try */
		SpreadConfiguration *thissc = fds[fd];
		int retval;

		if(extralog) {
		  fprintf(stderr, "Terminal error closing spread mailbox %d\n",
		  	  fd);
		}
		fds[fd] = NULL;
		FD_CLR(fd, &masterset);
		tojoin = 1;
		thissc->connected = 0;
		retval = connectandjoin(thissc, &tojoin);
		if(retval >= 0)
		  FD_SET(retval, &masterset);
		else if(extralog)
		  fprintf(stderr, "Error connecting to spread daemon\n");
	      }
	    } else if(Is_regular_mess(service_type)) {
	      logfd = config_get_fd(fds[fd], groups[0], message);
	      pmessage = config_process_message(fds[fd],groups[0], message, &len);
#ifdef PERL
              config_do_external_perl(fds[fd], sender, groups[0], message);
#endif
#ifdef PYTHON
              config_do_external_python(fds[fd], sender, groups[0], message);
#endif
	      if(logfd<0) continue;
	      write(logfd, pmessage, len);
	    }
#ifdef DROP_RECV
	    /* Set DROP_RECV flag if we can */
	    service_type = DROP_RECV;
#endif
	  }
      } else {
        if(errno == EBADF) {
          /* Our Spread connection is bad */
          paranoid_establish_spread_connections();
        }
      }
      handle_signals();
      thistry = E_get_time();
      timediff = E_sub_time(thistry, lasttry);
      if(timediff.sec > 5) {
	lasttry = thistry;
	tojoin = 1;
	config_foreach_spreadconf(connectandjoin, (void *)&tojoin);
	FD_ZERO(&masterset);
	for(i=0;i<fdsetsize;i++) {
	  if(fds[i]) FD_SET(i, &masterset);
	}
      }
    }
  }
  config_cleanup();
  return -1;
}
