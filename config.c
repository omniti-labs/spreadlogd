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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/param.h>
#include <unistd.h>

#include "config.h"
#include "skiplist.h"
#include "timefuncs.h"

extern FILE *yyin;
extern int buffsize;
extern int nr_open;

int yyparse (void);
int line_num, semantic_errors;
static Skiplist logfiles;
static Skiplist spreaddaemons;
extern int verbose;
extern int skiplocking;
int logfile_compare(void *a, void *b) {
  LogFile *ar = (LogFile *)a;
  LogFile *br = (LogFile *)b;
  return strcmp(ar->filename, br->filename);
} 
int logfile_compare_key(void *a, void *b) {
  LogFile *br = (LogFile *)b;
  return strcmp(a, br->filename);
}  
int spreadd_compare(void *a, void *b) {
  SpreadConfiguration *ar = (SpreadConfiguration *)a;
  SpreadConfiguration *br = (SpreadConfiguration *)b;
  int temp;
  if((temp = strcmp(ar->port, br->port))!=0)
    return temp;
  if((!ar->host) && (!br->host)) return 0;
  if(!ar->host) return -1;
  if(!br->host) return 1;
  return strcmp(ar->host, br->host);
} 
int spreadd_compare_key(void *a, void *b) {
  SpreadConfiguration *br = (SpreadConfiguration *)b;
  int temp;
  if((temp = strcmp(a, br->port))!=0)
    return temp;
  if((!a) && (!br->host)) return 0;
  if(!a) return -1;
  if(!br->host) return 1;
  return strcmp(a, br->host);
}  
int facility_compare(void *a, void *b) {
  LogFacility *ar = (LogFacility *)a;
  LogFacility *br = (LogFacility *)b;
  return strcmp(ar->groupname, br->groupname);
} 
int facility_compare_key(void *a, void *b) {
  LogFacility *br = (LogFacility *)b;
  return strcmp(a, br->groupname);
}  
int config_init(char *filename) {
  int ret;
  sl_init(&logfiles);
  sl_set_compare(&logfiles, logfile_compare, logfile_compare_key);
  sl_init(&spreaddaemons);
  sl_set_compare(&spreaddaemons, spreadd_compare, spreadd_compare_key);
  /*
    sl_init(&logfacilities);
    sl_set_compare(&logfacilities, facility_compare, facility_compare_key);
  */
  yyin = fopen(filename, "r");
  if (!yyin) {
    fprintf(stderr, "Couldn't open input file: %s\n", filename);
    return -1;
  }
  ret = yyparse();
  fclose(yyin);
  return ret;
}  
char *config_get_spreaddaemon(SpreadConfiguration *sc) {
  static char buffer[1024];
  if(sc->host) {
    snprintf(buffer, 1024, "%s@%s", sc->port, sc->host);
  } 
  else {
    strncpy(buffer, sc->port, 1024);
  }
  return buffer;
} 

void config_set_spread_port(SpreadConfiguration *sc, char *newport) {
  if(sc->port) free(sc->port);
  sc->port = newport;
} 
void config_set_spread_host(SpreadConfiguration *sc, char *newhost) {
  if(sc->host) free(sc->host);
  sc->host = newhost;
} 
void config_add_spreadconf(SpreadConfiguration *sc) {
  sl_insert(&spreaddaemons, sc);
} 
SpreadConfiguration *config_new_spread_conf(void) {
  SpreadConfiguration *newsc;
  newsc = (SpreadConfiguration *)malloc(sizeof(SpreadConfiguration));
  newsc->host=NULL;
  newsc->port=strdup("4803");
  newsc->connected = 0;
  newsc->logfacilities = (Skiplist *)malloc(sizeof(Skiplist));
  sl_init(newsc->logfacilities);
  sl_set_compare(newsc->logfacilities,
		 facility_compare, facility_compare_key);  
  return newsc;
}  
LogFacility *config_new_logfacility(void) {
  LogFacility *newlf;
  newlf = malloc(sizeof(LogFacility));
  newlf->groupname=NULL;
  newlf->logfile=NULL;
  newlf->nmatches=0;
  newlf->rewritetimes=NO_REWRITE_TIMES;
  newlf->rewritetimesformat=NULL;
  return newlf;
} 
void config_add_logfacility(SpreadConfiguration *sc, LogFacility *lf) {
  sl_insert(sc->logfacilities, lf);
} 
void config_set_logfacility_group(LogFacility *lf, char *ng) {
  if(lf->groupname) free(lf->groupname);
  lf->groupname = ng;
} 
void config_set_logfacility_filename(LogFacility *lf, char *nf) {
  LogFile *logf;
  logf = sl_find(&logfiles, nf, NULL);
  lf->logfile = logf;
  if(!lf->logfile) {
    logf = (LogFile *)malloc(sizeof(LogFile));
    logf->filename = nf;
    logf->fd = -1;
    sl_insert(&logfiles, logf);
    lf->logfile = logf;
  } 
  else {
    free(nf);
  }
} 
void config_set_logfacility_vhostdir(LogFacility *lf, char *vhd) {
  int i;
  lf->vhostdir = vhd;
  lf->hash = (hash_element *) malloc (nr_open * sizeof(hash_element));
  fprintf( stderr, "\nZeroing vhost hash for usage!\n");
  for(i=0; i< nr_open; i++) {
    lf->hash[i].fd = -1;
    lf->hash[i].hostheader = NULL;
  }

}
void config_set_logfaclity_rewritetimes_clf(LogFacility *lf) {
  lf->rewritetimes = REWRITE_TIMES_IN_CLF;
}
void config_set_logfaclity_rewritetimes_user(LogFacility *lf, char *format) {
  lf->rewritetimes = REWRITE_TIMES_FORMAT;
  if(lf->rewritetimesformat) {
    free(lf->rewritetimesformat);
  }
  lf->rewritetimesformat = format;
}
void config_add_logfacility_match(LogFacility *lf, char *nm) {
#ifdef RE_SYNTAX_EGREP
  const char *ret;
#else  
  int ret;
#endif  
  if(lf->nmatches>=10) {
    fprintf(stderr, "Already 10 regex's on group\n");
    return;
  }
#ifdef RE_SYNTAX_EGREP
  re_set_syntax(RE_SYNTAX_EGREP);
  if((ret = re_compile_pattern(nm, strlen(nm),
			       &lf->match_expression[lf->nmatches]))!=0) {
    fprintf(stderr, ret);
#else  
if((ret = regcomp(&lf->match_expression[lf->nmatches], nm, REG_EGREP))!=0) {
      char errbuf[120];
      regerror(ret, &lf->match_expression[lf->nmatches], errbuf, sizeof errbuf);
      fprintf(stderr, errbuf);
#endif  
  } else { 
    lf->nmatches++;
  }
}

int config_foreach_spreadconf(int (*func)(SpreadConfiguration *, void *),
			      void *closure) {
  int i=0;
  struct skiplistnode *iter;
  SpreadConfiguration *sc;
  iter = sl_getlist(&spreaddaemons);
  if(!iter) return i;
  sc = iter->data;
  do {
    if(func(sc, closure)==0) i++;
  } while((sc = sl_next(&spreaddaemons, &iter))!=NULL);
  return i;  
}

int config_foreach_logfacility(SpreadConfiguration *sc, 
			       int (*func)(LogFacility *, void *),
			       void *closure) {
  int i=0;
  struct skiplistnode *iter;
  LogFacility *lf;
  iter = sl_getlist(sc->logfacilities);
  if(!iter) return i;
  lf = iter->data;
  do {
    if(func(lf, closure)==0) i++;
  } while((lf = sl_next(sc->logfacilities, &iter))!=NULL);
  return i;  
}

char *config_process_message(SpreadConfiguration *sc, char *group,
			     char *message, int *len) {
  LogFacility *lf;
  char *cp;
  lf = sl_find(sc->logfacilities, group, NULL);
  if(lf->rewritetimes)
    force_local_time(message, len, buffsize,
		     lf->rewritetimes, lf->rewritetimesformat);
  if(lf->vhostdir) {
    cp=message;
    while(*cp != ' ') {
	cp++;
	--*len;
    }
    --*len;
    return cp+1;
  }
  return message;
}   

void config_hup(void) {
  config_close();
  config_start();
}  

int config_close(void) {
  struct skiplistnode *sciter, *lfiter;
  SpreadConfiguration *sc;
  LogFacility *lf;
				int i;
  sciter = sl_getlist(&spreaddaemons);
  if(!sciter) return 0;
  sc = (SpreadConfiguration *)sciter->data;
  /* For each spread configuration: */
  do {
    lfiter = sl_getlist(sc->logfacilities);
    if(!lfiter) return 0;
    lf = (LogFacility *)lfiter->data;
    /* For each log facility in that spread configuration: */
    do {
      if(lf->vhostdir) {
	for (i=0;i< nr_open;i++) {
	  if(lf->hash[i].fd>0) {
	    if(!skiplocking) flock(lf->hash[i].fd, LOCK_UN);
	    close(lf->hash[i].fd);
	    lf->hash[i].fd = -1;
	  }
	}
      } else if(lf->logfile->fd>0) {
	if(!skiplocking) flock(lf->logfile->fd, LOCK_UN);
	close(lf->logfile->fd);
	lf->logfile->fd = -1;
      }
    } while((lf = sl_next(sc->logfacilities, &lfiter))!=NULL);
  } while((sc = sl_next(&spreaddaemons, &sciter))!=NULL);
  return 0;
}  

int config_start(void) {
  struct skiplistnode *sciter, *lfiter;
  SpreadConfiguration *sc;
  LogFacility *lf;
  sciter = sl_getlist(&spreaddaemons);
  if(!sciter) return 0;
  sc = (SpreadConfiguration *)sciter->data;
  /* For each spread configuration: */
  do {
    lfiter = sl_getlist(sc->logfacilities);
    if(!lfiter) return 0;
    lf = (LogFacility *)lfiter->data;
    /* For each log facility in that spread configuration: */
    do {
      if(lf->vhostdir) continue;
      else if(lf->logfile->fd<0)
	lf->logfile->fd = open(lf->logfile->filename,
#ifdef __USE_LARGEFILE64
			       O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE,
#else  
			       O_CREAT|O_APPEND|O_WRONLY,
#endif  
			       00644);
      if(!skiplocking) {
	if(flock(lf->logfile->fd, LOCK_NB|LOCK_EX)==-1) {
	  fprintf(stderr, "Cannot lock %s, is another spreadlogd running?\n",
		  lf->logfile->filename);
	  exit(1);
	}
      }
      if(verbose) {
	fprintf(stderr, "LogFacility: %s\n\tFile: %s\n\tFD: %d\n\t%d regexs\n",
		lf->groupname, lf->logfile->filename,
		lf->logfile->fd, lf->nmatches);
      }
    } while((lf = sl_next(sc->logfacilities, &lfiter))!=NULL);
  } while((sc = sl_next(&spreaddaemons, &sciter))!=NULL);
  return 0;
}  

int config_get_fd(SpreadConfiguration *sc, char *group, char *message) {
  LogFacility *lf;
  int i, ret, slen, fd;
  hash_element temp;
  char *cp;
  char fullpath[MAXPATHLEN];
  lf = sl_find(sc->logfacilities, group, NULL);
  if(!lf) return -1;
  if(lf->vhostdir) {
    cp = message;
    while(*cp != ' '){
      cp++;
    }
    *cp = '\0';
    if((fd = gethash(message, lf->hash)) < 0) {
      temp.hostheader = strdup(message);
      *cp = ' ';
      snprintf(fullpath, MAXPATHLEN, "%s/%s", lf->vhostdir,temp.hostheader);
      temp.fd = open(fullpath,
#ifdef __USE_LARGEFILE64
		     O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE,
#else  
		     O_CREAT|O_APPEND|O_WRONLY,
#endif  
	 	     00644);
      if(!skiplocking) {
	if(flock(temp.fd, LOCK_NB|LOCK_EX)==-1) {
	  fprintf(stderr, "Cannot lock %s, is another spreadlogd running?\n",
		  fullpath);
	  exit(1);
	}
      }
      inshash(temp,lf->hash);
      return temp.fd;
    }						
    *cp = ' ';
    return fd; 
  }     
  if(!lf->nmatches) return lf->logfile->fd;
  slen = strlen(message);
  for(i=0; i<lf->nmatches; i++) {
#ifdef RE_SYNTAX_EGREP
    if((ret = re_search(&lf->match_expression[i],
			message, slen, 0, slen, NULL)) >= 0)
      return lf->logfile->fd;
    else if(ret==-2 && verbose)
      fprintf(stderr, "Internal error in re_search.\n");
    else if(ret==-1 && verbose)
      fprintf(stderr, "Failed match!\n");
#else  if(!regexec(&lf->match_expression[i], message, 0, NULL, 0))
    return lf->logfile->fd;
#endif  
  }
  return -1;
}  

