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

#include "sld_config.h"

extern int line_num, semantic_errors;
extern int buffsize;
extern char *sld_text;
extern char *module_dir;

static SpreadConfiguration *current_sc = NULL;
static LogFacility *current_lf = NULL;

int sld_error(char *str);

#define NEW_SC_IFNEEDED if(!current_sc) current_sc=config_new_spread_conf();

#define NEW_LF_IFNEEDED if(!current_sc) current_sc=config_new_spread_conf(); \
                        if(!current_lf) current_lf=config_new_logfacility();
%}
%start Config
%token BUFFERSIZE SPREAD PORT HOST LOG GROUP FILENAME MATCH VHOSTGROUP VHOSTDIR
%token OPENBRACE CLOSEBRACE EQUALS STRING CLF REWRITETIMES
%token PERLLIB PERLUSE PERLLOG PERLHUP PYTHONIMPORT PYTHONLOG PYTHONLIB
%token MODULEDIR LOADMODULE MODULELOG
%%
Config		:	Globals SpreadConfs
			{ config_start(); }
		;

Globals		:	GlobalParam Globals
                |
                ;

GlobalParam	:	BUFFERSIZE EQUALS STRING
			{ if(buffsize<0) {
			    buffsize = atoi($3);
			  }
			}
		|	MODULEDIR EQUALS STRING
			{ module_dir = strdup($3); }
		|	LOADMODULE STRING
			{ module_load($2, NULL); }
		|	LOADMODULE STRING STRING
			{ module_load($2, $3); }
		|	PERLLIB STRING
			{
#ifdef PERL
			perl_inc($2);
#else
			fprintf(stderr, "PERL not compiled in\n");
			exit(0);
#endif
			}
		|	PERLUSE STRING
			{
#ifdef PERL
			perl_use($2);
#else
			fprintf(stderr, "PERL not compiled in\n");
			exit(0);
#endif
			}
                |       PYTHONLIB STRING
                        {
#ifdef PYTHON
			python_inc($2);
#else
			fprintf(stderr, "PYTHON not compiled in\n");
			exit(0);
#endif
			}
                |       PYTHONIMPORT STRING
                        {
#ifdef PYTHON
			python_import($2);
#else
			fprintf(stderr, "PYTHON not compiled in\n");
			exit(0);
#endif
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
		|	MODULELOG STRING
			{ NEW_LF_IFNEEDED;
			  config_set_logfacility_external_module(current_lf, $2);
			}
		|	PERLLOG STRING
			{ NEW_LF_IFNEEDED;
#ifdef PERL
			  config_set_logfacility_external_perl(current_lf, $2);
#else
			fprintf(stderr, "PERL not compiled in\n");
			exit(0);
#endif
			}
		|	PERLHUP STRING
			{ NEW_LF_IFNEEDED;
#ifdef PERL
			  config_set_hupfacility_external_perl(current_lf, $2);
#else
			fprintf(stderr, "PERL not compiled in\n");
			exit(0);
#endif
			}
			
		|	PYTHONLOG STRING
			{ NEW_LF_IFNEEDED;
#ifdef PYTHON
			  config_set_logfacility_external_python(current_lf, $2);
#else
			fprintf(stderr, "PYTHON not compiled in\n");
			exit(0);
#endif
			}
		|	MATCH EQUALS STRING
			{ NEW_LF_IFNEEDED;
			  config_add_logfacility_match(current_lf, $3); }
		|	VHOSTDIR EQUALS STRING
			{ NEW_LF_IFNEEDED;
			  config_set_logfacility_vhostdir(current_lf, $3); }
		|	REWRITETIMES EQUALS CLF
			{ NEW_LF_IFNEEDED;
			  config_set_logfaclity_rewritetimes_clf(current_lf); }
		|	REWRITETIMES EQUALS STRING
			{ NEW_LF_IFNEEDED;
			  config_set_logfaclity_rewritetimes_user(current_lf,
								  $3); }
		;


%%
int sld_error(char *str) {
  fprintf(stderr, "Parser error on or before line %d\n", line_num);
  fprintf(stderr, "Offending token: %s\n", sld_text);
  return -1;
}
