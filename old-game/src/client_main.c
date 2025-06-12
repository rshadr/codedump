#include <inttypes.h>
#include <SDL3/SDL.h>

#include "debug.h"
#include "gfx.h"

typedef struct SpClient_s {
  SpGraphics *gfx;
} SpClient;


static const SDL_InitFlags k_sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
static const SpGraphicsCreateOptions k_gfx_options = {
  .enable_validation_layers = true,
};


static void
spClientInit (SpClient *client)
{
  (void) client;

  SP_ASSERT( SDL_InitSubSystem(k_sdl_init_flags), "failed to init SDL" );
  client->gfx = spGraphicsCreate(&k_gfx_options);
}


static void
spClientQuit (SpClient *client)
{
  (void) client;

  spGraphicsDestroy(client->gfx);
  SDL_QuitSubSystem(k_sdl_init_flags);
}


int
main (int argc, char *argv[])
{
  (void) argc;
  (void) argv;

  SpClient client = { 0 };

  spClientInit(&client);
  spClientQuit(&client);

  return 0;
}

