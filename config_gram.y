%{
/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "config.h"

extern int line_num, semantic_errors;
extern int buffsize;
extern char *yytext;

static SpreadConfiguration *current_sc = NULL;
static LogFacility *current_lf = NULL;

#define NEW_SC_IFNEEDED if(!current_sc) current_sc=config_new_spread_conf();

#define NEW_LF_IFNEEDED if(!current_sc) current_sc=config_new_spread_conf(); \
                        if(!current_lf) current_lf=config_new_logfacility();
%}
%start Config
%token BUFFERSIZE SPREAD PORT HOST LOG GROUP FILENAME MATCH
%token OPENBRACE CLOSEBRACE EQUALS STRING
%%
Config		:	Globals SpreadConfs
			{ config_start(); }

Globals		:	GlobalParam Globals
		|
		;

GlobalParam	:	BUFFERSIZE EQUALS STRING
			{ if(buffsize<0) {
			    buffsize = atoi($3);
			  }
			}

SpreadConfs	:	SpreadConf SpreadConfs
                |	SpreadConf
		;

SpreadConf	:	SPREAD OPENBRACE SPparams LogStructs CLOSEBRACE
                        { config_add_spreadconf(current_sc); 
			  current_sc = NULL; }
		;

SPparams	:	SPparam SPparams
		|       
		;

SPparam		:	PORT EQUALS STRING
			{ NEW_SC_IFNEEDED;
			  config_set_spread_port(current_sc, $3); }
		|	HOST EQUALS STRING
			{ NEW_SC_IFNEEDED;
			  config_set_spread_host(current_sc, $3); }
		;

LogStructs	:	LogStruct LogStructs
		|
		;

LogStruct	:	LOG OPENBRACE Logparams CLOSEBRACE
			{ config_add_logfacility(current_sc, current_lf);
			  current_lf = NULL; }
		;

Logparams	:	Logparams Logparam
		|
		;

Logparam	:	GROUP EQUALS STRING
			{ NEW_LF_IFNEEDED;
			  config_set_logfacility_group(current_lf, $3); }
		|	FILENAME EQUALS STRING
			{ NEW_LF_IFNEEDED;
			  config_set_logfacility_filename(current_lf, $3); }
		|	MATCH EQUALS STRING
			{ NEW_LF_IFNEEDED;
			  config_add_logfacility_match(current_lf, $3); }
		;


%%
int yyerror(char *str) {
  fprintf(stderr, "Parser error on or before line %d\n", line_num);
  fprintf(stderr, "Offending token: %s\n", yytext);
  return -1;
}
