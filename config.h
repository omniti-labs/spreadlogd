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

#include "skiplist.h"

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
  regex_t match_expression[10]; /* only up to ten */
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

int config_foreach_logfacility(SpreadConfiguration *,
			       int (*)(LogFacility *, void *), void *);

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

