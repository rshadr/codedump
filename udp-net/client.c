
#include <math.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "common.h"

typedef struct SpGraphics_s {
  SDL_Window *win;
  SDL_Renderer *rend;
  int32_t w;
  int32_t h;
} SpGraphics;

typedef struct SpPlayer_s {
  uint16_t radius;
  uint16_t x;
  uint16_t y;
  bool alive;
} SpPlayer;

typedef struct SpPlayerList_s {
  size_t size;
  SpPlayer data[MAX_PLAYERS];
} SpPlayerList;

typedef struct SpClient_s {
  // SpGraphics *gfx;
  int sock_fd;
  struct sockaddr_in sa_to;
  struct sockaddr_in sa_from;

  SpPlayerList players;
  uint8_t self_id;

  struct {
    bool have;
    uint16_t x;
    uint16_t y;
  } queued_move;
} SpClient;

static char const *k_server_ip = "127.0.0.1";
static const uint16_t k_server_port = 12000;

#define BUF_LEN 256


static int
spGraphicsCreate (SpGraphics *self)
{
  self->w = 512;
  self->h = 512;

  SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

  self->win = SDL_CreateWindow("udp-net",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    self->w, self->h,
    SDL_WINDOW_SHOWN);

  self->rend = SDL_CreateRenderer(self->win, -1, SDL_RENDERER_ACCELERATED);

  return 0;
}


static int
spGraphicsDestroy (SpGraphics *self)
{
  SDL_DestroyRenderer(self->rend);
  SDL_DestroyWindow(self->win);
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  SDL_Quit();

  return 0;
}


static void
spGraphicsStep (SpClient *client, SpGraphics *gfx)
{
  SDL_SetRenderDrawBlendMode(gfx->rend, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(gfx->rend, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(gfx->rend);

  SDL_SetRenderDrawColor(gfx->rend, 0x00, 0xFF, 0x00, 0xFF);
  for (uint16_t player_idx = 0;
       player_idx < client->players.size;
       ++player_idx)
  {
    if (! client->players.data[player_idx].alive )
      continue;
    SpPlayer *player = &client->players.data[player_idx];

    int ox = (int)player->x - player->radius;
    int oy = (int)player->y - player->radius;

    for (int y = oy; y < (int)player->y + player->radius; ++y)
    for (int x = ox; x < (int)player->x + player->radius; ++x)
    {
      double dx = player->x - x;
      double dy = player->y - y;
      if (sqrt(dy*dy + dx*dx) <= (double)player->radius)
        SDL_RenderDrawPoint(gfx->rend, x, y);
    }

  }

  SDL_RenderPresent(gfx->rend);
}


static void
fill_sockaddr (struct sockaddr_in *sa, char const *ip, uint16_t port)
{
  memset(sa, 0, sizeof(*sa));
  sa->sin_family = AF_INET;
  sa->sin_port = htons(port);
  inet_pton(AF_INET, ip, &sa->sin_addr);
}


static int
read_snapshot_body (SpClient *client,
                    size_t buf_size, const uint8_t buf[buf_size])
{
  uint8_t const *p = buf;

  uint16_t count = (uint16_t) *p++;
  printf("currently have %"PRIu16" players\n", count);

  for (uint16_t i = 0; i < client->players.size; ++i)
    client->players.data[i].alive = false;

  for (uint16_t i = 0; i < count; ++i) {
    uint8_t pidx = *p++;
    SpPlayer *player = &client->players.data[pidx];

    player->alive = true;
    uint16_t *q = (uint16_t *)p;
    player->radius = ntohs(*q++);
    player->x = ntohs(*q++);
    player->y = ntohs(*q++);

    p = (uint8_t *)q;
  }

  return 0;
}


static int
process_live_packet (SpClient *client,
                     size_t buf_size, const uint8_t buf[buf_size])
{
  uint8_t const *p = buf;
  const uint8_t * const end = &buf[buf_size];

  if (buf_size == 0) {
    printf("null live packet!\n");
    return -1;
  }

  switch ((SpServerPacketType) *p++)
  {
    case SP_SVPKT_SNAPSHOT:
      return read_snapshot_body(client, end - p, p);

    default:
      break;
  }

  printf("bad live packet!\n");
  return -1;
}


static int
process_join_response (SpClient *client, size_t buf_size, uint8_t buf[buf_size])
{
  uint8_t *p = buf;
  uint8_t const * const end = &buf[buf_size];

  if (buf_size == 0)
    return -1;

  SpServerPacketType svpkt = *p++;

  switch (svpkt)
  {
    case SP_SVPKT_INIT:
      printf("Initted.\n");
      client->self_id = *p++;
      return read_snapshot_body(client, end - p, p);

    case SP_SVPKT_GOODBYE:
      // shouldn't even be received; we leave without recv back
      printf("goodbye\n");
      return -1;

    default:
      printf("something else entirely\n");
      return -1;
  }

  // unreached
  return 0;
}


static int
do_input (SpClient *client, SpGraphics *gfx)
{
  SDL_Event ev;

  while (SDL_PollEvent(&ev))
  {
    if (ev.type == SDL_QUIT)
      return -1;

    if (ev.type == SDL_MOUSEBUTTONDOWN) {
      client->queued_move.have = true;
      client->queued_move.x = ev.button.x;
      client->queued_move.y = ev.button.y;
      continue;
    }
  }

  return 0;
}


static size_t
spClientDisconnect (SpClient *client)
{
  uint8_t req[BUF_LEN] = { 0 };
  uint8_t *p = req;

  *p++ = SP_CLPKT_LEAVE;
  // ...
  
  size_t numbytes = sendto(client->sock_fd, req, p - req, 0,
                     (struct sockaddr *)&client->sa_to, sizeof(client->sa_to));

  return numbytes;
}

static size_t
spClientMaybeStatus (SpClient *client)
{
  uint8_t req[BUF_LEN] = { 0 };
  uint8_t *p = req;
  size_t numbytes = 0;

  if (client->queued_move.have) {
    *p++ = SP_CLPKT_MOVE;
    uint16_t *q = (uint16_t *)p;
    *q++ = htons(client->queued_move.x);
    *q++ = htons(client->queued_move.y);
    p = (uint8_t *)q;

    numbytes = sendto(client->sock_fd, req, p - req, 0,
                 (struct sockaddr *)&client->sa_to, sizeof(client->sa_to));
  }

  return numbytes;
}


int
main (int argc, char *argv[])
{
  (void) argc;
  (void) argv;

  SpClient client = { 0 };
  SpGraphics gfx = { 0 };

  spGraphicsCreate(&gfx);

  uint8_t buf[BUF_LEN] = { 0 };
  size_t buf_size = 0;

  client.sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  fill_sockaddr(&client.sa_to, k_server_ip, k_server_port);

  buf[0] = SP_CLPKT_JOIN;
  ++buf_size;

  size_t numbytes = sendto(client.sock_fd, buf, buf_size, 0,
                     (struct sockaddr *)&client.sa_to, sizeof(client.sa_to));

  printf("%zu bytes sent at first\n", numbytes);

  memset(buf, 0, sizeof(buf));
  socklen_t from_size = sizeof(client.sa_from);
  numbytes = recvfrom(client.sock_fd, buf, sizeof(buf), 0,
              (struct sockaddr *) &client.sa_from, &from_size);
  buf_size = numbytes;

  if (process_join_response(&client, buf_size, buf) != 0)
    goto stopit;

  while (true) {
    memset(buf, 0, sizeof(buf));
    socklen_t from_size = sizeof(client.sa_from);
    numbytes = recvfrom(client.sock_fd, buf, sizeof(buf), 0,
                (struct sockaddr *) &client.sa_from, &from_size);
    buf_size = numbytes;
    process_live_packet(&client, buf_size, buf);

    if (do_input(&client, &gfx) != 0)
      break;

    spClientMaybeStatus(&client);

    spGraphicsStep(&client, &gfx);
  }
  
stopit:
  spClientDisconnect(&client);
  spGraphicsDestroy(&gfx);
  close(client.sock_fd);

  return 0;
}

