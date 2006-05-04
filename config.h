/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdlib.h>
#include <sys/file.h>
#ifdef HAVE_GNUREGEX_H
#include <gnuregex.h>
#else
#include <regex.h>
#endif
#include <sp.h>
#include <event.h>

#include "hash.h"
#include "skiplist.h"

#define SHELL_PATH "/bin/sh"

typedef struct {
  char *port;
  char *host;
  char private_group[MAX_GROUP_NAME];
  int connected;
  Skiplist *logfacilities;
  int fd;
  struct event event;
} SpreadConfiguration;

typedef struct {
  char *filename;
  int fd;
} LogFile;

typedef struct sld_module_list {
  char *module_name;
  struct sld_module_abi *module;
  struct sld_module_list *next;
} sld_module_list_t;

typedef struct {
  char *groupname;
  LogFile *logfile;
  int nmatches;
  int rewritetimes;
  char *rewritetimesformat;
  regex_t match_expression[10]; /* only up to ten */
  char *vhostdir;
  hash_element *hash;
  char *perl_log_handler;
  char *perl_hup_handler;
  char *python_handler;
  sld_module_list_t *modulelog;
} LogFacility;

int config_init(char *); /* Initialize global structures */
void config_cleanup(); /* Initialize global structures */

char *config_get_spreaddaemon(SpreadConfiguration *);
SpreadConfiguration *config_new_spread_conf(void);
void config_set_spread_port(SpreadConfiguration *,char *);
void config_set_spread_host(SpreadConfiguration *, char *);
void config_add_spreadconf(SpreadConfiguration *);

int config_foreach_spreadconf(int (*)(SpreadConfiguration *, void *), void *);

LogFacility *config_new_logfacility(void);
void config_add_logfacility(SpreadConfiguration *, LogFacility *);
void config_set_logfacility_group(LogFacility *, char *);
void config_set_logfacility_filename(LogFacility *, char *);
void config_set_logfacility_external_perl(LogFacility *, char *);
void config_add_logfacility_match(LogFacility *, char *);
void config_set_logfacility_external_module(LogFacility *, char *);
void config_set_hupfacility_external_perl(LogFacility *, char *);
void config_set_logfacility_vhostdir(LogFacility *lf, char *vhd);
void config_set_logfaclity_rewritetimes_clf(LogFacility *lf);
void config_set_logfaclity_rewritetimes_user(LogFacility *lf, char *format);

int config_foreach_logfacility(SpreadConfiguration *,
			       int (*)(LogFacility *, void *), void *);
char *config_process_message(SpreadConfiguration *sc, char *group, char *message, int *len);
void config_hup(void);  /* config_close(); config_start(); */
int config_close(void); /* Close files */
int config_start(void); /* Open files and get ready to log */

int config_get_fd(SpreadConfiguration *sc,
		  char *group, char *message); /* -1 if no write */

int config_do_external_module(SpreadConfiguration *sc,
		  char *sender, char *group, char *message); /* -1 if no write */
int config_do_external_perl(SpreadConfiguration *sc,
		  char *sender, char *group, char *message); /* -1 if no write */
#define YYSTYPE MY_YYSTYPE
typedef char * MY_YYSTYPE;
extern YYSTYPE sld_lval;
extern int yysemanticerr;


#endif

