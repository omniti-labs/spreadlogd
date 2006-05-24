#ifndef _ONLINE_H_
#define _ONLINE_H_

#include "sld_config.h"
#include "module.h"
#include "echash.h"
#include "nethelp.h"
#include "skip_heap.h"

#define MAX_AGE 30*60
#define MAX_USER_INFO 30
#define DEFAULT_PORT 8989
#define DEFAULT_ADDRESS "*"

typedef struct urlcount {
  char *URL;               /* The URL, so we only store one copy */
  unsigned int cnt;        /* count of users who last viewed this */
} urlcount_t;

typedef struct hit {
  unsigned long long SiteUserID;  /* The viewer's ID */
  char *URL;                      /* The URL, refs urls */
  time_t Hitdate;                 /* the time of the hit */
} hit_t;

void online_service(int fd, short event, void *vnl);
urlcount_t *get_url(const char *url, int len);
void cull_old_hits();
struct skiplistnode *get_first_hit_for_url(const char *url);
unsigned int get_current_online_count();

#endif
