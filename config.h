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
#include <regex.h>
#include <sp.h>

#include "hash.h"
#include "skiplist.h"

#define SHELL_PATH "/bin/sh"

typedef struct {
  char *port;
  char *host;
  char private_group[MAX_GROUP_NAME];
  int connected;
  Skiplist *logfacilities;
} SpreadConfiguration;

typedef struct {
  char *filename;
  int fd;
} LogFile;


typedef struct {
  char *groupname;
  LogFile *logfile;
  int nmatches;
  int rewritetimes;
  char *rewritetimesformat;
  regex_t match_expression[10]; /* only up to ten */
  char *vhostdir;
  hash_element *hash;
} LogFacility;

int config_init(char *); /* Initialize global structures */

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
void config_add_logfacility_match(LogFacility *, char *);
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

#define YYSTYPE YYSTYPE
typedef char * YYSTYPE;
extern YYSTYPE yylval;
extern int yysemanticerr;


#endif

