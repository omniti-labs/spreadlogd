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
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sp.h>

#include "config.h"

#define SPREADLOGD_VERSION "1.4"

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
  int *tojoin = (int *)uv;
  char sld[MAX_GROUP_NAME];
  snprintf(sld, MAX_GROUP_NAME, "sld-%05d", getpid());
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
    if(*tojoin)
      config_foreach_logfacility(sc, join, &mbox);
    return 0;
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

int establish_spread_connections() {
  int tojoin = 1;
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

int main(int argc, char **argv) {
  char *configfile = default_configfile;
  char *message;
  int getoption, debug = 0;
  struct sigaction signalaction;
  sigset_t ourmask;

  fdsetsize = getdtablesize();
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
    case 'V':
      fprintf(stderr,
	      "(%s VERSION: %s) %s\nLinked against libsp VERSION: %.2f\n",
	      "spreadlogd", SPREADLOGD_VERSION, argv[0], SP_version());
      exit(0);
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
	fprintf(stderr, "Setting FD: %d\n", fd);
	FD_SET(fd, &masterset);
      }
    }
    lasttry = E_get_time();
    while(1) {
      /* Build out select */
      struct timeval timeout;
      readset = masterset;
      exceptset = masterset;
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
		tojoin = 1;
		thissc->connected = 0;
		connectandjoin(thissc, &tojoin);
	      }
	    } else if(Is_regular_mess(service_type)) {
	      logfd = config_get_fd(fds[fd], groups[0], message);
	      if(logfd<0) continue;
	      pmessage = config_process_message(fds[fd],groups[0], message, &len);
	      write(logfd, pmessage, len);
	    }
#ifdef DROP_RECV
	    /* Set DROP_RECV flag if we can */
	    service_type = DROP_RECV;
#endif
	  }
      }
      handle_signals();
      thistry = E_get_time();
      timediff = E_sub_time(thistry, lasttry);
      if(timediff.sec > 5) {
	lasttry = thistry;
	tojoin = 1;
	config_foreach_spreadconf(connectandjoin, (void *)&tojoin);
      }
    }
  }
  return -1;
}
