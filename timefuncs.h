/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#ifndef _TIMEFUNCS_H_
#define _TIMEFUNCS_H_

#define NO_REWRITE_TIMES      0
#define REWRITE_TIMES_IN_CLF  1
#define REWRITE_TIMES_FORMAT  2

void force_local_time(char *string, int *length, const int buffsize,
		      int style, char *format);

#endif
