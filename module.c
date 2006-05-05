/* ======================================================================
 * Copyright (c) 2006 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "sld_config.h"
#include "module.h"
#include "skiplist.h"

extern char *module_dir;

static int compare_module(const void *a, const void *b) {
  sld_module_abi_t *am = (sld_module_abi_t *)a;
  sld_module_abi_t *bm = (sld_module_abi_t *)b;
  return strcmp(am->module_name, bm->module_name);
}
static int compare_module_key(const void *a, const void *b) {
  char *a_module_name = (char *)a;
  sld_module_abi_t *bm = (sld_module_abi_t *)b;
  return strcmp(a_module_name, bm->module_name);
}

static int modules_init = 0;
static Skiplist modules;
static struct __plist {
  char *name;
  char *config;
  struct __plist *next;
} *prospects = NULL;

void module_init() {
  if(modules_init) return;
  sl_init(&modules);
  sl_set_compare(&modules, compare_module, compare_module_key);
}

void module_load(const char *module, const char *config) {
  struct __plist *newhead;
  newhead = malloc(sizeof(*newhead));
  newhead->name = strdup(module);
  newhead->config = config?strdup(config):NULL;
  newhead->next = prospects;
  prospects = newhead;
}

int module_load_finalize() {
  struct __plist *node = prospects;
  char filename[4096];
  for(node=prospects; node; node=node->next) {
    void *handle;
    sld_module_abi_t *module;
    snprintf(filename, sizeof(filename), "%s%s%s.so",
             module_dir?module_dir:"", module_dir?"/":"", node->name);
    handle = dlopen(filename, RTLD_NOW|RTLD_GLOBAL);
    if(!handle) {
      fprintf(stderr, "Error loading %s\n", filename);
      return -1;
    }
    module = (sld_module_abi_t *)dlsym(handle, node->name);
    if(!module) {
      fprintf(stderr, "Error loading %s from %s\n", node->name, filename);
      return -1;
    }
    sl_insert(&modules, module);
    if(module->init(node->config)) return -1;
  }
  return 0;
}

sld_module_abi_t *module_get(const char *symbol) {
  sld_module_abi_t *module;
  module = sl_find(&modules, symbol, NULL);
  return module;
}
