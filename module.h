/* ======================================================================
 * Copyright (c) 2006 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#ifndef _SLD_MODULE_H
#define _SLD_MODULE_H

#include "sld_config.h"

typedef int  (*ModuleInit)(const char *);
typedef void (*ModuleLogLine)(SpreadConfiguration *,
                              const char *, const char *, const char *);
typedef void (*ModuleShutdown)();

typedef struct sld_module_abi {
  char          *module_name;
  ModuleInit     init;
  ModuleLogLine  logline;
  ModuleShutdown shutdown;
} sld_module_abi_t;

void module_init();
void module_load(const char *symbol, const char *config);
int  module_load_finalize();
sld_module_abi_t *module_get(const char *symbol);

#endif
