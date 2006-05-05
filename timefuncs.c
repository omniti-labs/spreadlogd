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
#include "timefuncs.h"

#define MAXTIMESTRLEN 128

static char *apmonthnames[12] =
  {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static char *strnchr(char *string, char chr, int len) {
  char *cp=string;
  while(len>0) {
    if(*cp == chr) return cp;
    cp++; len--;
  }
  return NULL;
}
void force_local_time(char *string, int *length, const int buffsize,
		      int style, char *format) {
 char *cp, *cpend, *newcpend;
 char timebuff[MAXTIMESTRLEN];
 int timestrlen, newtimestrlen;
 int timz;
 struct tm *t;
 time_t secs;
 struct timeval now;
 struct timezone tz;

 cp = strnchr(string, '[', *length);
 if(!cp) return;
 cpend = strnchr(cp, ']', *length-(cp-string));
 if(!cpend) return;
 cpend++;
 timestrlen = cpend-cp;
 gettimeofday(&now, &tz);
 timz = tz.tz_minuteswest;
 secs = now.tv_sec;
 t = localtime(&secs);
 if(style == NO_REWRITE_TIMES)
  return;
 else if(style == REWRITE_TIMES_IN_CLF) {
  char sign = (timz > 0 ? '-' : '+');
  timz = (timz < 0)?(-timz):(timz);
  snprintf(timebuff, sizeof(timebuff), "[%02d/%s/%d:%02d:%02d:%02d %c%.2d%.2d]",
           t->tm_mday, apmonthnames[t->tm_mon], t->tm_year+1900, 
           t->tm_hour, t->tm_min, t->tm_sec,
           sign, timz / 60, timz % 60);
 } else if((style == REWRITE_TIMES_FORMAT) && format) {
  strftime(timebuff, sizeof(timebuff), format, t);
 }

 /* No measure and squeeze it in there */
 newtimestrlen = strlen(timebuff);
 newcpend = cp+newtimestrlen;
 
 if(newcpend != cpend) {
   /* Ugh different size... this is going to me slower */
   int chunk = MIN(MIN(string+buffsize-cpend, string+buffsize-newcpend),
		   string+*length-cpend);
   if(chunk>0)
     memmove(newcpend, cpend, chunk);
   *length += newcpend-cpend;
   if(*length >= buffsize) {
     newtimestrlen -= *length-buffsize;
     *length = buffsize;
   }
 }
 memcpy(cp, timebuff, newtimestrlen);
 return;
}
