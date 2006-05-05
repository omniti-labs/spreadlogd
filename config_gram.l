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
#include "y.tab.h"

extern int line_num;
extern int semantic_errors;

#define yylval sld_lval
#define yytext sld_text

%}
qstring \"[^\"]*\"|\'[^\']*\'
string  [^ \t\r\n#{}]+
%option noyywrap
%%
#.*                             {} /* Comments */
[ \t\r]                         {} /* White space */
\n                              { line_num++;}
"{"                             { return OPENBRACE; }
"}"                             { return CLOSEBRACE; }
"="                             { return EQUALS; }
BufferSize			{ return BUFFERSIZE; }
Spread                          { return SPREAD; }
Port                            { return PORT; }
Host                            { return HOST; }
Log                             { return LOG; }
VhostGroup			{ return VHOSTGROUP; }
Group                           { return GROUP; }
File                            { return FILENAME; }
VhostDir			{ return VHOSTDIR; }
Match                           { return MATCH; }
RewriteTimestamp		{ return REWRITETIMES; }
CommonLogFormat			{ return CLF; }
ModuleDir			{ return MODULEDIR; }
LoadModule			{ return LOADMODULE; }
ModuleLog			{ return MODULELOG; }
PerlLib                         { return PERLLIB; }
PerlUse                         { return PERLUSE; }
PerlLog                         { return PERLLOG; }
PerlHup				{ return PERLHUP; }
PythonImport                    { return PYTHONIMPORT; }
PythonLog                       { return PYTHONLOG; }
{qstring}                       { int l = strlen(yytext);
                                  yytext[l-1] = 0;
                                  yylval = strdup(yytext+1);
                                  return STRING; }
{string}                        { yylval = strdup(yytext);
                                  return STRING; }
%%
