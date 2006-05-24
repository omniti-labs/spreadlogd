#include "online.h"

#define DEFAULT_PORT 8989
#define DEFAULT_ADDRESS "*"

typedef struct user_hit_info {
  unsigned long long SiteUserID;  /* This viewer's ID */
  int age;                        /* Seconds since view */
  int pad;                        /* 4-byte alignment pad */
} user_hit_info_t;

static void get_hit_info(const char *url, unsigned int *total,
                         unsigned int *url_total,
                         user_hit_info_t *uinfo,
                         unsigned int *nusers) {
  struct skiplistnode *first;
  urlcount_t *uc;
  time_t now;
  int max_users = *nusers;

  /* Clean up any hits that are too old */
  cull_old_hits();
  *total = get_current_online_count();
  if((uc = get_url(url, strlen(url))) == NULL) {
    *nusers = *url_total = 0;
    return;
  }
  now = time(NULL);
  *url_total = uc->cnt;
  first = get_first_hit_for_url(uc->URL);
  *nusers = 0;
  if(!first) return; /* something is very wrong */
  while(*nusers < max_users && first) {
    hit_t *hit = (hit_t *)first->data;
    /* keep going until we see a new URL */
    if(!hit || strcmp(hit->URL,uc->URL)) break;
    uinfo[*nusers].SiteUserID = hit->SiteUserID;
    uinfo[*nusers].age = now - hit->Hitdate;
    (*nusers)++;
    sl_next(&first);
  }
  return;
}
void online_service(int fd, short event, void *vnl) {
  netlisten_t *nl = (netlisten_t *)vnl;
  int expected_write, actual_write, i;
  struct iovec io[4];
  unsigned int total, url_total, nusers = MAX_USER_INFO;
  user_hit_info_t uinfo[MAX_USER_INFO];
  struct {
    unsigned short sofar;
    unsigned short ulen;
    char *url;
  } *req;

  if(NULL == (req = nl->userdata))
    nl->userdata = req = calloc(1, sizeof(*req));

  if(!req->ulen) {
    /* Read the length the URL to be passed (network short) */
    if(read(nl->fd, &req->ulen, sizeof(req->ulen)) != sizeof(req->ulen))
      goto bail;
    req->ulen = ntohs(req->ulen);
    /* Allocate our read length plus a terminator */
    req->url = malloc(req->ulen + 1);
    req->url[req->ulen] = '\0';
  }
  while(req->sofar < req->ulen) {
    int len = read(nl->fd, req->url, req->ulen - req->sofar);
    if(len == -1 && errno == EAGAIN) return; /* incomplete read */
    if(len <= 0) goto bail;                  /* error */
    req->sofar += len;
  }

  /* Get the answer */
  get_hit_info(req->url, &total, &url_total, uinfo, &nusers);

  /* Pack it on the network */
  expected_write = sizeof(total) * 3 + nusers * sizeof(*uinfo);
  io[0].iov_base = &total;     io[0].iov_len = sizeof(total);
  io[1].iov_base = &url_total; io[1].iov_len = sizeof(url_total);
  io[2].iov_base = &nusers;    io[2].iov_len = sizeof(nusers);
  io[3].iov_base = uinfo;
  io[3].iov_len = nusers * sizeof(*uinfo);

  total = htonl(total);
  url_total = htonl(url_total);
  for(i=0;i<nusers;i++) {
    uinfo[i].SiteUserID = bswap64(uinfo[i].SiteUserID);
    uinfo[i].age = htonl(uinfo[i].age);
  }
  nusers = htonl(nusers);

  /* We should be able to write it all at once. We don't support */
  /* command pipelining, so the total contents of the outbound   */
  /* buffer will only ever be this large.                        */
  actual_write = writev(nl->fd, io, 4);
  if(actual_write != expected_write) goto bail;
  
  free(req->url);
  memset(req, 0, sizeof(*req));
  return;
 bail:
  if(req) {
    if(req->url) free(req->url);
    free(req);
  }
  close(nl->fd);
  event_del(&nl->e);
  return;
}
