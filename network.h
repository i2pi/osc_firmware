#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <arpa/inet.h>

typedef struct {
  int fd;
  struct sockaddr_storage addr;
  socklen_t addr_len;
} conT;

typedef struct connectionT {
  conT con;
  size_t (*send)(struct connectionT *, const void *, size_t);
  size_t (*receive)(struct connectionT *, const void *, size_t);
} connectionT;

#endif
