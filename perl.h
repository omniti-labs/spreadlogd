#ifndef __PERL_H_
#define __PERL_H_

#include <EXTERN.h>
#include <perl.h>

void perl_startup();
void perl_shutdown();
I32 perl_inc(char *path);
I32 perl_use(char *module);
I32 perl_log(char *func, char *sender, char *group, char *message);
I32 perl_hup(char *func);

#endif
