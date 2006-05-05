#ifndef __PERL_H_
#define __PERL_H_

void perl_startup();
void perl_shutdown();
#ifndef I32
#define I32 int
#endif
I32 perl_inc(char *path);
I32 perl_use(char *module);
I32 perl_log(char *func, char *sender, char *group, char *message);
I32 perl_hup(char *func);

#endif
