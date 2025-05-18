#include <assert.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "common.h"

#define BUF_LEN 256

typedef struct SpClientInfo_s {
  char host[INET_ADDRSTRLEN];
  uint16_t port;
  bool alive;
} SpClientInfo;

typedef struct SpClientData_s {
  SpClientInfo info;
  uint16_t radius;
  uint16_t x;
  uint16_t y;
} SpClientData;

typedef struct SpList_s {
  SpClientData data[MAX_PLAYERS];
  size_t size;
  mtx_t mutex;
  cnd_t t_lock;
} SpList;

typedef struct SpThreadArgs_s {
  SpList *list;
  int fd;
} SpThreadArgs;


static char const *k_server_ip = "127.0.0.1";
static const uint16_t k_server_port = 12000;


static void
fill_sockaddr (struct sockaddr_in *sa, char const *ip, uint16_t port)
{
  memset(sa, 0, sizeof(*sa));
  sa->sin_family = AF_INET;
  sa->sin_port = htons(port);
  inet_pton(AF_INET, ip, &(sa->sin_addr));
}


static void
get_client_info (struct sockaddr_in *sa, SpClientInfo *info)
{
  memset(info, 0, sizeof(*info));
  info->port = ntohs(sa->sin_port);
  inet_ntop(sa->sin_family, &sa->sin_addr, info->host, INET_ADDRSTRLEN);
}


static bool
client_info_equal (SpClientInfo *lhs, SpClientInfo *rhs)
{
  return ((lhs->port == rhs->port) && !strcmp(lhs->host, rhs->host));
}


static void
respond_badreq (int server_fd, struct sockaddr_in *sa_from)
{
  uint8_t resbuf[BUF_LEN] = { 0 };
  uint8_t *r = resbuf;
  *r++ = SP_SVPKT_BADREQ;
  sendto(server_fd, resbuf, r - resbuf, 0, (struct sockaddr *)sa_from, sizeof(*sa_from));
}


static size_t
prepare_snapshot_body (SpThreadArgs *args,
                       size_t buf_left,
                       uint8_t buf[buf_left])
{
  SpList *list = args->list;
  uint8_t *p = buf;
  //const uint8_t * const end = &buf[buf_left];

  *p++ = (uint8_t)(list->size);

  for (uint16_t i = 0; i < list->size; ++i) {
    if (! list->data[i].info.alive )
      continue;

    SpClientData *cldata = &list->data[i];

    *p++ = i;
    uint16_t *q = (uint16_t *)p;
    *q++ = htons(cldata->radius);
    *q++ = htons(cldata->x);
    *q++ = htons(cldata->y);

    p = (uint8_t *)q;
  }

  return (p - buf);
}


static void
process_join_packet (SpThreadArgs *args,
                     SpClientInfo *cinfo, struct sockaddr_in *sa_from,
                     size_t buf_size, uint8_t buf[buf_size])
{
  int server_fd = args->fd;
  SpList *list = args->list;
  uint8_t res[BUF_LEN] = { 0 };
  const uint8_t * const rend = &res[sizeof(res)];
  uint8_t *r = res;

  if (list->size == LEN(list->data)) {
    printf("too many guys here sorry\n");
    *r++ = SP_SVPKT_GOODBYE;
    *r++ = SP_SVPKT_GOODBYE_TOOMANY;
    sendto(server_fd, res, r - res, 0, (struct sockaddr *)sa_from, sizeof(*sa_from));
    return;
  }

  for (int i = 0; i < list->size; ++i) {
    if ( !list->data[i].info.alive &&
         client_info_equal(cinfo, &list->data[i].info) ) {
      printf("he's already here??\n");
      *r++ = SP_SVPKT_GOODBYE;
      *r++ = SP_SVPKT_GOODBYE_ALREADYHERE;
      sendto(server_fd, res, r - res, 0, (struct sockaddr *)sa_from, sizeof(*sa_from));
      return;
    }
  }

  for (uint16_t i = 0; i < LEN(list->data); ++i) {
    SpClientData *data = &list->data[i];

    if (data->info.alive)
      continue;

    if (i >= list->size)
      ++list->size;

    data->info = *cinfo;
    data->info.alive = true;

    data->x = 0;
    data->y = 0;
    data->radius = 8;

    *r++ = SP_SVPKT_INIT;
    *r++ = (uint8_t)(i);
    r += prepare_snapshot_body(args, rend - r, r);
    sendto(server_fd, res, r - res, 0, (struct sockaddr *)sa_from, sizeof(*sa_from));

    printf("Player %s registered as %zu\n", data->info.host, (size_t)(i));

    return;
  }

  // *r++ = SP_SVPKT_INIT;
}


static void
process_leave_packet (SpThreadArgs *args,
                      SpClientInfo *cinfo, struct sockaddr_in *sa_from,
                      size_t buf_size, uint8_t buf[buf_size])
{
  int server_fd = args->fd;
  SpList *list = args->list;
  uint8_t res[BUF_LEN] = { 0 };
  uint8_t *r = res;

  for (int i = 0; i < list->size; ++i) {
    if (list->data[i].info.alive &&
        client_info_equal(&list->data[i].info, cinfo)) {
      SpClientData *cdata = &list->data[i];

      *r++ = SP_SVPKT_GOODBYE;
      
      sendto(server_fd, res, r - res, 0, (struct sockaddr *)sa_from, sizeof(*sa_from));
      memset(cdata, 0, sizeof(*cdata));

      return;
    }
  }

  respond_badreq(server_fd, sa_from);
}

static void
process_move_body (SpThreadArgs *args,
                   SpClientInfo *cinfo, struct sockaddr_in *sa_from,
                   size_t buf_size, const uint8_t buf[buf_size])
{
  SpList *list = args->list;
  uint16_t const *q = (uint16_t const *)buf;

  for (int i = 0; i < list->size; ++i) {
    if (list->data[i].info.alive &&
        client_info_equal(&list->data[i].info, cinfo)) {
      SpClientData *cdata = &list->data[i];

      cdata->x = ntohs(*q++);
      cdata->y = ntohs(*q++);

      return;
    }
  }

}


static void
process_packet (SpThreadArgs *args,
                SpClientInfo *cinfo, struct sockaddr_in *sa_from,
                size_t buf_size, uint8_t buf[buf_size])
{
  printf("start processing packet\n");

  int server_fd = args->fd;
  uint8_t *p = buf;
  const uint8_t * const end = &buf[buf_size];


  if (buf_size == 0) {
    printf("empty packet!\n");
    goto failure;
  }

  SpClientPacketType clpkt = *p++;

  switch (clpkt)
  {
    case SP_CLPKT_JOIN:
      printf("join packet\n");
      process_join_packet(args, cinfo, sa_from, end - p, p);
      break;
    case SP_CLPKT_LEAVE:
      printf("leave packet\n");
      process_leave_packet(args, cinfo, sa_from, end - p, p);
      break;
    case SP_CLPKT_MOVE:
      printf("move packet\n");
      process_move_body(args, cinfo, sa_from, end - p, p);
      break;
    default:
      printf("bad packet\n");
      goto failure;
  }
  return;

failure:
  respond_badreq(server_fd, sa_from);
  return;
}


static int
recv_handler (void *args_)
{
  SpThreadArgs *args = args_;
  SpList *list = args->list;
  int server_fd = args->fd;

  uint8_t buf[BUF_LEN] = { 0 };
  size_t buf_size = 0;
  struct sockaddr_in sockaddr_from;

  socklen_t from_len = sizeof(sockaddr_from);

  while (1) {
    buf_size = recvfrom(server_fd, buf, sizeof(buf), 0,
                (struct sockaddr *)(&sockaddr_from), &from_len);
    printf("received packet of %zu bytes\n", buf_size);

    SpClientInfo client_info;
    get_client_info(&sockaddr_from, &client_info);

    mtx_lock(&list->mutex);

      process_packet(args, &client_info, &sockaddr_from, buf_size, buf);

    cnd_signal(&list->t_lock);
    mtx_unlock(&list->mutex);
  }

  return 0;
}


static int
send_handler (void *args_)
{
  static const struct timespec request = { 1, 0 };

  SpThreadArgs *args = args_;
  SpList *list = args->list;
  int client_fd = args->fd;
  struct timespec remaining;

  uint8_t res[BUF_LEN] = { 0 };
  uint8_t *r = res;

  while (1) {
    mtx_lock(&list->mutex);

      for (int client_idx = 0; client_idx < list->size; ++client_idx)
      {
        SpClientData *cdata = &list->data[client_idx];
        if (! cdata->info.alive )
          continue;

        *r++ = SP_SVPKT_SNAPSHOT;
        r += prepare_snapshot_body(args, r - res, r);

        struct sockaddr_in sockaddr_to = { 0 };
        socklen_t to_len = sizeof(sockaddr_to);
        fill_sockaddr(&sockaddr_to, cdata->info.host, cdata->info.port);
        sendto(client_fd, res, r - res, 0, (struct sockaddr *)&sockaddr_to, to_len);
      }

    cnd_signal(&list->t_lock);
    mtx_unlock(&list->mutex);
    nanosleep(&request, &remaining);
  }

  return 0;
}


int
main (int argc, char *argv[])
{
  (void) argc;
  (void) argv;

  int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  int client_fd = socket(AF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in sockaddr_to;
  fill_sockaddr(&sockaddr_to, k_server_ip, k_server_port);

  const int so_reuseaddr = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(int));

  printf("Binding...\n");
  bind(server_fd, (struct sockaddr *)(&sockaddr_to), sizeof(sockaddr_to));

  SpList list = { 0 };
  SpThreadArgs server_info = { .list = &list, .fd = server_fd };
  SpThreadArgs client_info = { .list = &list, .fd = client_fd };

  thrd_t recv_thread;
  thrd_t send_thread;

  thrd_create(&recv_thread, recv_handler, (void *) &server_info);
  thrd_create(&send_thread, send_handler, (void *) &client_info);

  while (1) {
    sleep(1);
  }

  // unreached yet
  close(server_fd);
  close(client_fd);

  int retval;
  thrd_join(recv_thread, &retval);
  thrd_join(send_thread, &retval);

  return 0;
}

