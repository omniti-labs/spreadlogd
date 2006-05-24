#include "online.h"

static Skiplist hits;      /* Tracks each users's last hit */
static ec_hash_table urls; /* Tracks the count on each URL */

void urlcount_free(void *vuc) {
  if(((urlcount_t *)vuc)->URL) free(((urlcount_t *)vuc)->URL);
  free(vuc);
}
urlcount_t *get_url(const char *url, int len) {
  void *uc;
  if(echash_retrieve(&urls, url, len, &uc)) return uc;
  return NULL;
}
unsigned int get_current_online_count() {
  return hits.size;
}
static void urlcount_decrement(const char *url) {
  urlcount_t *uc;
  if((uc = get_url(url, strlen(url))) != NULL) {
    if(!(--uc->cnt))
      echash_delete(&urls, url, strlen(url), NULL, urlcount_free);
  }
}
void hit_free(void *vhit) {
  urlcount_decrement(((hit_t *)vhit)->URL);
  free(vhit);
}

/* comparator for the URL,Hitdate index */
static int url_hitdate_comp(const void *a, const void *b) {
  int ret;
  ret = strcmp(((hit_t *)a)->URL, ((hit_t *)b)->URL);
  if(ret) return ret;
  /* Newest (greatest) in front */
  return (((hit_t *)a)->Hitdate < ((hit_t *)b)->Hitdate)?1:-1;
}
/* comparator for the Hitdate */
static int hitdate_comp(const void *a, const void *b) {
  /* Oldest in front... so we can pop off expired ones */
  return (((hit_t *)a)->Hitdate < ((hit_t *)b)->Hitdate)?-1:1;
}
/* comparator for the SiteUserID */
static int SiteUserID_comp(const void *a, const void *b) {
  if(((hit_t *)a)->SiteUserID == ((hit_t *)b)->SiteUserID) return 0;
  if(((hit_t *)a)->SiteUserID < ((hit_t *)b)->SiteUserID) return -1;
  return 1;
}
static int SiteUserID_comp_key(const void *a, const void *b) {
  if(*((unsigned long long *)a) == ((hit_t *)b)->SiteUserID) return 0;
  if(*((unsigned long long *)a) < ((hit_t *)b)->SiteUserID) return -1;
  return 1;
}
void cull_old_hits() {
  hit_t *hit;
  time_t oldest;
  oldest = time(NULL) - MAX_AGE;
  while((hit = sl_peek(&hits)) != NULL && (hit->Hitdate < oldest))
    sl_pop(&hits, hit_free);
}

struct skiplistnode *get_first_hit_for_url(const char *url) {
  struct skiplistnode *match, *left, *right;
  hit_t target;
  target.URL = (char *)url;
  /* ask for the node one second in the future.  We'll miss and */
  /* 'right' will point to the newest node for that URL.        */
  target.Hitdate = time(NULL) + 1;
  sl_find_compare_neighbors(&hits, &target, &match, &left, &right,
                            url_hitdate_comp);
  return right;
}

static int online_init(const char *config) {
  char *host = NULL, *sport = NULL;
  unsigned int port;
  echash_init(&urls);
  sl_init(&hits);
  sl_set_compare(&hits, hitdate_comp, hitdate_comp);
  sl_add_index(&hits, SiteUserID_comp, SiteUserID_comp_key);
  sl_add_index(&hits, url_hitdate_comp, url_hitdate_comp);

  if(config) host = strdup(config);
  if(host) sport = strchr(host, ':');
  if(sport) {
    *sport++ = '\0';
    port = atoi(sport);
  } else
    port = DEFAULT_PORT;
  if(!host) host = DEFAULT_ADDRESS;
  if(tcp_dispatch(host, port, 100, EV_READ|EV_PERSIST, online_service,
                  NULL) < 0) {
    fprintf(stderr, "Could not start service on %s\n", config);
    return -1;
  }
  return 0;
}

#define SET_FROM_TOKEN(a,b) do { \
  a ## _len = tokens[(b)+1]-tokens[(b)]-1; \
  a = tokens[(b)]; \
} while(0)

static void online_logline(SpreadConfiguration *sc,
      const char *sender, const char *group, const char *message) {
  const char *tokens[8];
  const char *user, *url;
  unsigned long long SiteUserID;
  int user_len, url_len;
  urlcount_t *uc;
  hit_t *hit;
  int i;
  struct skiplistnode *slnode;

  tokens[0] = message;
  for(i=1; i<8; i++) {
    tokens[i] = strchr(tokens[i-1], ' ');
    if(!tokens[i]++) return;  /* couldn't find token */
  }
  /* the userid is field 3 and the URI is field 7 based on white space */
  SET_FROM_TOKEN(user, 2);
  SET_FROM_TOKEN(url, 6);
  SiteUserID = strtoul(user, NULL, 10);
  /* Find the URL in the URL counts, creating if necessary */
  if((uc = get_url(url, url_len)) == NULL) {
    uc = calloc(1, sizeof(*uc));
    uc->URL = malloc(url_len+1);
    memcpy(uc->URL, url, url_len);
    uc->URL[url_len] = '\0';
    echash_store(&urls, uc->URL, url_len, uc);
  }
  /* Increment the counter on the URL */
  uc->cnt++;

  /* Fetch this users's last hit */
  hit = sl_find_compare(&hits, &SiteUserID, &slnode, SiteUserID_comp);
  if(!hit) {
    /* No hit for this user, allocate one */
    hit = calloc(1, sizeof(*hit));
  }
  else {
    /* We have an old hit.  We must reduce the count on the old URL.
     * it is not our string, so we don't free it. */
    sl_remove_compare(&hits, &SiteUserID, NULL, SiteUserID_comp);
    urlcount_decrement(hit->URL);
  }
  hit->URL = uc->URL;
  hit->SiteUserID = SiteUserID;
  hit->Hitdate = time(NULL);
  sl_insert(&hits, hit);
  cull_old_hits();
}
static void online_shutdown() {
  fprintf(stderr, "Stopping online module.\n");
}

sld_module_abi_t online = {
 "online",
 online_init,
 online_logline,
 online_shutdown,
};
