#ifndef PYTHON_H
#define PYTHON_H

void python_startup();
void python_shutdown();
int python_inc(char *path);
int python_use(char *module);
int python_log(char *func, char *sender, char *group, char *message);

#endif
