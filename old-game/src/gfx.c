#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "util.h"
#include "debug.h"
#include "gfx.h"

struct queue_family_indices {
  uint32_t graphics_family;
  uint32_t present_family;
  bool has_graphics_family;
  bool has_present_family;
};

struct swap_chain_support {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32_t formats_count;
  uint32_t present_modes_count;
  VkSurfaceFormatKHR *formats;
  VkPresentModeKHR *present_modes;
};


static const int32_t k_window_init_width = 512;
static const int32_t k_window_init_height = 512;
static const SDL_WindowFlags k_window_flags =
  SDL_WINDOW_VULKAN;
static char const * const k_validation_layers[] = {
  "VK_LAYER_KHRONOS_validation",
};
static const int32_t k_num_validation_layers = LEN(k_validation_layers);
static char const * const k_device_extensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const int32_t k_num_device_extensions = LEN(k_device_extensions);


static char const * *
spGraphicsGetRequiredExtensions (SpGraphics *gfx, uint32_t *count)
{
  (void) gfx;
  SP_ASSERT( count != NULL, "missing count pointer" );

  char const * const * sdl_extensions = SDL_Vulkan_GetInstanceExtensions(count);

  SP_ASSERT( sdl_extensions != NULL, "failed to get required extensions from SDL" );

  if (gfx->options->enable_validation_layers)
    *count += 1;

  char const * *extensions = calloc(*count, sizeof(sdl_extensions[0]));
  memcpy(extensions, sdl_extensions, (*count) * sizeof(sdl_extensions[0]));

  if (gfx->options->enable_validation_layers)
    extensions[*count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  for (uint32_t i = 0; i < *count; ++i)
    printf("  %s\n", extensions[i]);

  return extensions;
}


VKAPI_ATTR
VkBool32 VKAPI_CALL
spVulkanDebugMessengerCallback (VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                VkDebugUtilsMessageTypeFlagsEXT message_type,
                                const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                void *user_data)
{
  (void) message_severity;
  (void) message_type;
  (void) user_data;

  fprintf(stderr, "validation layer: %s\n", callback_data->pMessage);

  return VK_FALSE;
}


static void
spGraphicsPopulateDebugMessengerCreateInfo (VkDebugUtilsMessengerCreateInfoEXT *create_info)
{

  *create_info = (VkDebugUtilsMessengerCreateInfoEXT) {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = spVulkanDebugMessengerCallback,
  };

}


static struct queue_family_indices
spGraphicsFindQueueFamilies (SpGraphics *gfx, VkPhysicalDevice device)
{
  struct queue_family_indices indices = { 0 };

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

  VkQueueFamilyProperties *queue_families = calloc(queue_family_count, sizeof(queue_families[0]));
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  for (uint32_t i = 0; i < queue_family_count; ++i) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
      indices.has_graphics_family = true;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, gfx->surface, &present_support);

    if (present_support) {
      indices.present_family = i;
      indices.has_present_family = true;
    }

    if (indices.has_graphics_family
     && indices.has_present_family)
      break;
  }

  free(queue_families);
  return indices;
}


static struct swap_chain_support
spGraphicsQuerySwapChainSupport (SpGraphics *gfx, VkPhysicalDevice device)
{
  struct swap_chain_support support = { 0 };

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, gfx->surface, &support.capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(device, gfx->surface, &support.formats_count, NULL);
  support.formats = calloc(support.formats_count, sizeof(support.formats[0]));
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, gfx->surface, &support.formats_count, support.formats);

  vkGetPhysicalDeviceSurfacePresentModesKHR(device, gfx->surface, &support.present_modes_count, NULL);
  support.present_modes = calloc(support.present_modes_count, sizeof(support.present_modes[0]));
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, gfx->surface,
   &support.present_modes_count, support.present_modes);

  return support;
}


static void
freeSwapChainSupport (struct swap_chain_support *support)
{
  free(support->formats);
  free(support->present_modes);
}


static VkPhysicalDevice
spGraphicsPickPhysicalDevice (SpGraphics *gfx)
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(gfx->instance, &device_count, NULL);

  SP_ASSERT( device_count != 0, "failed to find GPUs with Vulkan support" );

  VkPhysicalDevice *devices = calloc(device_count, sizeof(devices[0]));
  vkEnumeratePhysicalDevices(gfx->instance, &device_count, devices);

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;

  for (uint32_t i = 0; i < device_count; ++i) {
    printf(" Physical Device %d\n", i);
    struct queue_family_indices indices = spGraphicsFindQueueFamilies(gfx, devices[i]);

    printf("  Graphics Family: %d\n", (int)indices.graphics_family);
    printf("  Present Family: %d\n", (int)indices.present_family);

    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extension_count, NULL);

    VkExtensionProperties *available_extensions = calloc(extension_count, sizeof(available_extensions[0]));
    vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extension_count, available_extensions);

    bool required_extensions_supported = true;

    for (int j = 0; j < k_num_device_extensions; ++j) {
      bool extension_found = false;

      for (int k = 0; k < (int)extension_count; ++k) {
        if (strcmp(k_device_extensions[j], available_extensions[k].extensionName) == 0) {
          extension_found = true;
          break;
        }
      }

      if (! extension_found ) {
        required_extensions_supported = false;
        printf("extension %s not supported \n", k_device_extensions[j]);
        break;
      }
    }

    if (! required_extensions_supported )
      continue;

    struct swap_chain_support sc_support = spGraphicsQuerySwapChainSupport(gfx, devices[i]);
    bool swap_chain_adequate = (sc_support.formats_count > 0
                             && sc_support.present_modes_count > 0);

    freeSwapChainSupport(&sc_support);

    if (indices.has_graphics_family
     && indices.has_present_family
     && swap_chain_adequate) {
      printf(" Device %d is suitable\n", (int)i);
      physical_device = devices[i];
      break;
    }
  }

  free(devices);
  return physical_device;
}


static VkExtent2D
spGraphicsChooseSwapExtent (SpGraphics *gfx, VkSurfaceCapabilitiesKHR *capabilities)
{
  if (capabilities->currentExtent.width != 0xFFFFFFFF)
    return capabilities->currentExtent;

  int32_t w, h;
  SDL_GetWindowSize(gfx->window, &w, &h);

  VkExtent2D actual_extent = {
    CLAMP(capabilities->minImageExtent.width, capabilities->maxImageExtent.width, w),
    CLAMP(capabilities->minImageExtent.height, capabilities->maxImageExtent.height, h),
  };

  return actual_extent;
}


static void
spGraphicsCreateSwapChain (SpGraphics *gfx)
{
  struct swap_chain_support sc_support =
    spGraphicsQuerySwapChainSupport(gfx, gfx->physical_device);

  SP_ASSERT( sc_support.formats_count > 0,
    "swap chain support not available (no formats)" );
  SP_ASSERT( sc_support.present_modes_count > 0,
    "swap chain support not availanle (no present modes)" );

  printf("Swap chain support %d formats, %d present modes\n",
    sc_support.formats_count, sc_support.present_modes_count);

  VkSurfaceFormatKHR surface_format = sc_support.formats[0];
  for (int i = 0; i < (int)sc_support.formats_count; ++i) {
    if (sc_support.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
     && sc_support.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surface_format = sc_support.formats[i];
      break;
    }
  }

  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (int i = 0; i < (int)sc_support.present_modes_count; ++i) {
    if (sc_support.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = sc_support.present_modes[i];
      break;
    }
  }

  gfx->swap_chain_extent = spGraphicsChooseSwapExtent(gfx, &sc_support.capabilities);
  printf(" swap chain extent: %d x %d\n",
    (int)gfx->swap_chain_extent.width,
    (int)gfx->swap_chain_extent.height);

  uint32_t image_count = sc_support.capabilities.minImageCount + 1;
  if (sc_support.capabilities.maxImageCount > 0
   && image_count > sc_support.capabilities.maxImageCount)
    image_count = sc_support.capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR swap_chain_create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = gfx->surface,
    .minImageCount = image_count,
    .imageFormat = surface_format.format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent = gfx->swap_chain_extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
  };

  struct queue_family_indices indices = spGraphicsFindQueueFamilies(gfx, gfx->physical_device);
  uint32_t queue_family_indices[2] = {
    indices.graphics_family,
    indices.present_family
  };

  if (indices.graphics_family != indices.present_family) {
    swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swap_chain_create_info.queueFamilyIndexCount = 2;
    swap_chain_create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  swap_chain_create_info.preTransform = sc_support.capabilities.currentTransform;
  swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swap_chain_create_info.presentMode = present_mode;
  swap_chain_create_info.clipped = VK_TRUE;

  SP_VK_ASSERT( vkCreateSwapchainKHR(gfx->device, &swap_chain_create_info, NULL, &gfx->swap_chain),
    "failed to create swap chain" );

  vkGetSwapchainImagesKHR(gfx->device, gfx->swap_chain, &gfx->swap_chain_images_count, NULL);
  gfx->swap_chain_images = calloc(gfx->swap_chain_images_count,
    sizeof(gfx->swap_chain_images[0]));
  vkGetSwapchainImagesKHR(gfx->device, gfx->swap_chain, &gfx->swap_chain_images_count,
    gfx->swap_chain_images);

  gfx->swap_chain_image_format = surface_format.format;

  freeSwapChainSupport(&sc_support);

  printf(" swap chain created\n");
}


static void
spGraphicsCreateImageViews (SpGraphics *gfx)
{
  gfx->swap_chain_image_views_count =
    gfx->swap_chain_images_count;
  gfx->swap_chain_image_views =
    calloc(gfx->swap_chain_images_count, sizeof(gfx->swap_chain_image_views[0]));

  for (size_t i = 0; i < gfx->swap_chain_image_views_count; ++i) {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = gfx->swap_chain_images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = gfx->swap_chain_image_format,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };

    SP_VK_ASSERT( vkCreateImageView(gfx->device, &create_info, NULL, &gfx->swap_chain_image_views[i]),
      "failed to create image views" );
  }

  printf(" image views created\n");
}


static void
spGraphicsCreateFramebuffers (SpGraphics *gfx)
{
  gfx->swap_chain_framebuffers_count =
    gfx->swap_chain_image_views_count;
  gfx->swap_chain_framebuffers =
    calloc(gfx->swap_chain_framebuffers_count,
      sizeof(gfx->swap_chain_framebuffers[0]));

  for (size_t i = 0; i < gfx->swap_chain_image_views_count; ++i) {
    VkImageView attachments[] = {
      gfx->swap_chain_image_views[i]
    };

    VkFramebufferCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = gfx->render_pass,
      .attachmentCount = LEN(attachments),
      .pAttachments = attachments,
      .width = gfx->swap_chain_extent.width,
      .height = gfx->swap_chain_extent.height,
      .layers = 1,
    };

    SP_VK_ASSERT( vkCreateFramebuffer(gfx->device, &create_info, NULL,
      &gfx->swap_chain_framebuffers[i]),
      "failed to create framebuffer" );
  }
  printf(" created %d framebuffers\n", (int)gfx->swap_chain_framebuffers_count);
}


static int
spGraphicsInitVulkan (SpGraphics *gfx)
{

  /*
   * Create instance
   */
  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Game",
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "GameEngine",
    .engineVersion = VK_MAKE_VERSION(0, 0, 1),
    .apiVersion = VK_API_VERSION_1_0,
  };

  VkInstanceCreateInfo instance_create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
  };
  char const * *enabled_extensions;

  enabled_extensions =
    spGraphicsGetRequiredExtensions(gfx, &instance_create_info.enabledExtensionCount);
  instance_create_info.ppEnabledExtensionNames = (char const * const *) enabled_extensions;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { 0 };
  if (gfx->options->enable_validation_layers) {
    instance_create_info.enabledLayerCount = k_num_validation_layers;
    instance_create_info.ppEnabledLayerNames = k_validation_layers;
    spGraphicsPopulateDebugMessengerCreateInfo(&debug_create_info);
    instance_create_info.pNext = &debug_create_info;
  } else {
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.pNext = NULL;
  }

  SP_VK_ASSERT( vkCreateInstance(&instance_create_info, NULL, &gfx->instance),
    "failed to create instance" );

  free(enabled_extensions); /* XXX: memleak? */


  /*
   * Create window surface
   */
  SP_ASSERT( SDL_Vulkan_CreateSurface(gfx->window, gfx->instance, NULL, &gfx->surface),
    "failed to create window surface" );


  /*
   * Pick physical device
   */
  VkPhysicalDevice physical_device = spGraphicsPickPhysicalDevice(gfx);
  SP_ASSERT( physical_device != VK_NULL_HANDLE, "failed to find a suitable GPU" );
  gfx->physical_device = physical_device;

  /*
   * Create logical device
   */
  struct queue_family_indices physical_indices = spGraphicsFindQueueFamilies(gfx, gfx->physical_device);
  uint32_t unique_queue_families[2] = {
    physical_indices.graphics_family,
    physical_indices.present_family
  };
  uint32_t num_unique_queue_families =
    (unique_queue_families[0] == unique_queue_families[1] ? 1 : 2);
  VkDeviceQueueCreateInfo *queue_create_infos =
    calloc(num_unique_queue_families, sizeof(queue_create_infos[0]));

  float queue_priority = 1.0f;
  for (uint32_t i = 0; i < num_unique_queue_families; ++i) {
    queue_create_infos[i] = (VkDeviceQueueCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = unique_queue_families[i],
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    };
  }

  VkPhysicalDeviceFeatures device_features = { 0 };
  VkDeviceCreateInfo device_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = num_unique_queue_families,
    .pQueueCreateInfos = queue_create_infos,
    .pEnabledFeatures = &device_features,
    .enabledExtensionCount = k_num_device_extensions,
    .ppEnabledExtensionNames = k_device_extensions,
  };

  if (gfx->options->enable_validation_layers) {
    device_create_info.enabledLayerCount = k_num_validation_layers;
    device_create_info.ppEnabledLayerNames = k_validation_layers;
  } else {
    device_create_info.enabledLayerCount = 0;
  }

  SP_VK_ASSERT( vkCreateDevice(physical_device, &device_create_info, NULL, &gfx->device),
    "failed to create logical device" );

  free(queue_create_infos);

  vkGetDeviceQueue(gfx->device, physical_indices.graphics_family, 0, &gfx->graphics_queue);
  vkGetDeviceQueue(gfx->device, physical_indices.present_family, 0, &gfx->present_queue);


  /*
   * Create swap chain
   */
  spGraphicsCreateSwapChain(gfx);

  /*
   * Create image views
   */
  spGraphicsCreateImageViews(gfx);

  /*
   * Create render pass
   */
  VkAttachmentDescription color_attachment = {
    .format = gfx->swap_chain_image_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference color_attachment_ref = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_ref,
  };

  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  VkRenderPassCreateInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency,
  };

  SP_VK_ASSERT( vkCreateRenderPass(gfx->device, &render_pass_info, NULL, &gfx->render_pass),
    "failed to create render pass" );

  // ...

  VkCommandPoolCreateInfo command_pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = physical_indices.graphics_family,
  };

  SP_VK_ASSERT( vkCreateCommandPool(gfx->device, &command_pool_info, NULL, &gfx->command_pool),
    "failed to create command pool" );

  // ...

  
  return 0;
}


static void
spGraphicsDestroyImageViews (SpGraphics *gfx)
{
  for (size_t i = 0; i < gfx->swap_chain_image_views_count; ++i)
    vkDestroyImageView(gfx->device, gfx->swap_chain_image_views[i], NULL);
  free(gfx->swap_chain_image_views);
  gfx->swap_chain_image_views = NULL;
  gfx->swap_chain_image_views_count = 0;
}


static void
spGraphicsDestroySwapChain (SpGraphics *gfx)
{
  spGraphicsDestroyImageViews(gfx);
  vkDestroySwapchainKHR(gfx->device, gfx->swap_chain, NULL);
}


static int
spGraphicsDestroyVulkan (SpGraphics *gfx)
{
  vkDestroyRenderPass(gfx->device, gfx->render_pass, NULL);
  spGraphicsDestroySwapChain(gfx);
  vkDestroyDevice(gfx->device, NULL);
  vkDestroySurfaceKHR(gfx->instance, gfx->surface, NULL);
  vkDestroyInstance(gfx->instance, NULL);
  return 0;
}


SpGraphics *
spGraphicsCreate (const SpGraphicsCreateOptions *options)
{
  SpGraphics *gfx = calloc(1, sizeof(*gfx));
  gfx->options = options;

  gfx->window = SDL_CreateWindow("game",
    k_window_init_width, k_window_init_height,
    k_window_flags);
  SP_ASSERT( gfx->window, "failed to create window" );

  SP_ASSERT( spGraphicsInitVulkan(gfx) == 0, "failed to init Vulkan" );

  return gfx;
}


void
spGraphicsDestroy (SpGraphics *gfx)
{
  SP_ASSERT( spGraphicsDestroyVulkan(gfx) == 0, "failed to destroy Vulkan" );

  SDL_DestroyWindow(gfx->window);
  free(gfx);
}

