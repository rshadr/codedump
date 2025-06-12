#ifndef __sp_gfx_h__
#define __sp_gfx_h__

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

typedef struct SpGraphicsCreateOptions_s {
  bool enable_validation_layers;
} SpGraphicsCreateOptions;

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

typedef struct SpGraphics_s {

  const SpGraphicsCreateOptions *options;
  SDL_Window *window;

  VkInstance instance;
  VkSurfaceKHR surface;

  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue present_queue;

  VkExtent2D swap_chain_extent;
  VkSwapchainKHR swap_chain;
  uint32_t swap_chain_images_count;
  VkImage *swap_chain_images;
  VkFormat swap_chain_image_format;
  uint32_t swap_chain_image_views_count;
  VkImageView *swap_chain_image_views;
  uint32_t swap_chain_framebuffers_count;
  VkFramebuffer *swap_chain_framebuffers;

  VkRenderPass render_pass;

  VkCommandPool command_pool;
  uint32_t command_buffers_count;
  VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
} SpGraphics;


SpGraphics *spGraphicsCreate (const SpGraphicsCreateOptions *options);
void spGraphicsDestroy (SpGraphics *gfx);


#endif /* !defined(__sp_gfx_h__) */

