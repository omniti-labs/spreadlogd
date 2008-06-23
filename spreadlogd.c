/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "sld_config.h"
#include "module.h"

#define SPREADLOGD_VERSION "2.0.1"

#define _TODO_JOIN 1
#define _TODO_PARANOID_CONNECT 2

#define RECONNECT_INTERVAL 5

extern char *optarg;
extern int optind, opterr, optopt;

int verbose = 0;
int extralog = 0;
int terminate = 0;
int huplogs = 0;
int skiplocking = 0;
int buffsize = -1;
char *module_dir = NULL;

static char *default_configfile = ETCDIR "/spreadlogd.conf";
static int connectandjoin(SpreadConfiguration *sc, void *uv);
static void handle_message(int fd, short event, void *arg);
static int paranoid_establish_spread_connections();

void usage(char *progname) {
  fprintf(stderr, "%s\t\tVERSION: %s\n \
\t-c configfile\t\t[default " ETCDIR "/spreadlogd.conf]\n \
\t-s\t\t\tskip locking (flock) files (NOT RECOMMENDED)\n \
\t-v\t\t\tverbose mode\n \
\t-x\t\t\tlog errors talking with spread\n \
\t-D\t\t\tdo not daemonize (debug)\n \
\t-V\t\t\tshow version information\n", progname, SPREADLOGD_VERSION);
  exit(1);
}

void sig_handler(int fd, short event, void *arg) {
  struct event *signal = arg;
  if(EVENT_SIGNAL(signal) == SIGHUP)
    huplogs = 1;
  else if(EVENT_SIGNAL(signal) == SIGTERM)
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

static int connectandjoin(SpreadConfiguration *sc, void *uv) {
  int mbox, err;
  int *todo = (int *)uv;
  char sld[MAX_GROUP_NAME];
  snprintf(sld, MAX_GROUP_NAME, "sld-%05d", getpid());
  if(sc->connected ||
     (err = SP_connect(config_get_spreaddaemon(sc),
                       sld, 1, 0, &mbox,
                       sc->private_group)) == ACCEPT_SESSION) {
    if(!sc->connected) {
      if(verbose)
	fprintf(stderr, "Successfully connected to spread at %s%c%s on %d\n",
		(sc->host)?sc->host:"/tmp",
		(sc->host)?':':'/',
		sc->port, mbox);
      sc->fd = mbox;
      event_set(&sc->event, mbox, EV_READ|EV_PERSIST, handle_message, sc);
      event_add(&sc->event, NULL);
    }
    sc->connected = 1;
    if(*todo & _TODO_JOIN)
      config_foreach_logfacility(sc, join, &mbox);
    return mbox;
  }
  if(verbose)
    fprintf(stderr, "Failed connection to spread at %s%c%s\n",
            (sc->host)?sc->host:"/tmp",
            (sc->host)?':':'/',
            sc->port);
  sc->connected = 0;
  return -1;  
}

static void handle_signals() {
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
  int fd;
  if(fork()!=0) exit(0);
  setsid();
  if(fork()!=0) exit(0);
  chdir("/");
  if((fd = open("/dev/null", O_RDWR, 0)) == -1) {
    fprintf(stderr, "Cannot open /dev/null\n");
    exit(0);
  }
  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  if(fd > STDERR_FILENO)
    close(fd);
}

static void handle_message(int fd, short event, void *arg) {
  SpreadConfiguration *sc = (SpreadConfiguration *)arg;
  service service_type;
  char sender[MAX_GROUP_NAME];
  char *pmessage;
  int len, num_groups, endian, logfd;
  char groups[1][MAX_GROUP_NAME];
  int16 mess_type;
#ifdef DROP_RECV
  service_type = DROP_RECV;
#endif
  char *message = NULL;

  message = (char *)malloc(buffsize*sizeof(char) + 1);

  len = SP_receive(sc->fd, &service_type, sender,
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
      int retval, tojoin;

      if(extralog) {
        fprintf(stderr, "Error closing spread mailbox %d\n", sc->fd);
      }
      SP_disconnect(sc->fd);
      event_del(&sc->event);
      sc->connected = 0;
      tojoin = _TODO_JOIN;
      retval = connectandjoin(sc, &tojoin);
    }
  }
  else if(Is_regular_mess(service_type)) {
    logfd = config_get_fd(sc, groups[0], message);
    message[len] = '\0';
    pmessage = config_process_message(sc ,groups[0], message, &len);
    config_do_external_module(sc, sender, groups[0], message);
#ifdef PERL
    config_do_external_perl(sc, sender, groups[0], message);
#endif
#ifdef PYTHON
    config_do_external_python(sc, sender, groups[0], message);
#endif
    if(logfd>=0) write(logfd, pmessage, len);
  }
  else if(len < 0) {
    if(errno == EBADF) {
      /* Our Spread connection is bad */
      paranoid_establish_spread_connections();
    }
  }
  handle_signals();
  if(message) free(message);
}

static int paranoid_establish_spread_connections() {
  int tojoin = _TODO_JOIN | _TODO_PARANOID_CONNECT;
  return config_foreach_spreadconf(connectandjoin, (void *)&tojoin);
}
static int establish_spread_connections() {
  int tojoin = _TODO_JOIN;
  return config_foreach_spreadconf(connectandjoin, (void *)&tojoin);
}

static void reconnect_spread(int fd, short event, void *arg) {
  struct timeval tv;
  struct event *reconn = (struct event *)arg;
  int tojoin;

  tojoin = _TODO_JOIN;
  config_foreach_spreadconf(connectandjoin, (void *)&tojoin);

  timerclear(&tv);
  tv.tv_sec = RECONNECT_INTERVAL;
  event_add(reconn, &tv);
}

static void stderr_debug(int s, const char *m) {
  fprintf(stderr, "[%d] %s\n", s, m);
}
int main(int argc, char **argv) {
  char *configfile = default_configfile;
  struct event signal_hup, signal_term, reconn;
  int getoption, debug = 0, rv;
  struct timeval tv;

  event_init();
  module_init();

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
  else event_set_log_callback(stderr_debug);

  /* SIGHUP and SIGTERM */
  event_set(&signal_hup, SIGHUP, EV_SIGNAL|EV_PERSIST, sig_handler,
            &signal_hup); 
  event_add(&signal_hup, NULL);
  event_set(&signal_term, SIGTERM, EV_SIGNAL|EV_PERSIST, sig_handler,
            &signal_term); 
  event_add(&signal_term, NULL);
  /* Periodic reconnect */
  evtimer_set(&reconn, reconnect_spread, &reconn);
  timerclear(&tv);
  tv.tv_sec = RECONNECT_INTERVAL;
  event_add(&reconn, &tv);
  
  establish_spread_connections();
  rv = event_dispatch();
  printf("event_dispatch -> %d [%d]\n", rv, errno);
  config_cleanup();
  return 0;
}

