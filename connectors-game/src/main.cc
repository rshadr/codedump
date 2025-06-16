
#include <cinttypes>
#include <cstdlib>

#include <stdexcept>
#include <vector>
#include <set>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool
  isComplete (void)
  {
    return graphics_family.has_value()
        && present_family.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;
};


class Game {
public:
  Game(void);
  ~Game();

private:
  SDL_Window *window;

  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkSurfaceKHR surface;

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;
  VkDevice device = VK_NULL_HANDLE;

  VkQueue graphics_queue;
  VkQueue present_queue;

  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapchain_images;
  VkFormat swapchain_image_format;
  VkExtent2D swapchain_extent;
  std::vector<VkImageView> swapchain_image_views;
  std::vector<VkFramebuffer> swapchain_framebuffers;

  VkRenderPass render_pass;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;

  VkImage color_image;
  VkDeviceMemory color_image_memory;
  VkImageView color_image_view;

  VkImage depth_image;
  VkDeviceMemory depth_image_memory;
  VkImageView depth_image_view;

  // UNUSED
  uint32_t mip_levels;
  VkImage texture_image;
  VkDeviceMemory texture_image_memory;
  VkImageView texture_image_view;
  VkSampler texture_sampler;

  // UNUSED ?
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  std::vector<VkBuffer> uniform_buffers;
  std::vector<VkDeviceMemory> uniform_buffers_memory;
  std::vector<void *> uniform_buffers_mapped;

  VkDescriptorPool descriptor_pool;
  std::vector<VkDescriptorSet> descriptor_sets;

  std::vector<VkCommandBuffer> command_buffers;

  std::vector<VkSemaphore> image_available_semaphores;
  std::vector<VkSemaphore> render_finished_semaphores;
  std::vector<VkFence> in_flight_fences;
  uint32_t current_frame = 0;

  bool framebuffer_resized = false;
  bool enable_validation_layers = true;

public:
  void run (void);

private:
  void initWindow (void);
  void initVulkan (void);
  void cleanupSwapchain (void);
  void cleanup (void);
  void mainLoop (void);

  void createInstance (void);
  void createSurface (void);
  void pickPhysicalDevice (void);
  void createLogicalDevice (void);
  void createSwapchain (void);
  void createImageViews (void);
  void createRenderPass (void);

  std::vector<char const *> getRequiredExtensions (void);
  void populateDebugMessengerCreateInfo (VkDebugUtilsMessengerCreateInfoEXT &create_info);
  SwapchainSupportDetails querySwapchainSupport (VkPhysicalDevice device);
  bool isDeviceSuitable (VkPhysicalDevice device);
  bool checkDeviceExtensionSupport (VkPhysicalDevice device);
  VkSampleCountFlagBits getMaxUsableSampleCount (void);
  QueueFamilyIndices findQueueFamilies (VkPhysicalDevice device);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat (std::vector<VkSurfaceFormatKHR> const *available_formats);
  VkPresentModeKHR chooseSwapPresentMode (std::vector<VkPresentModeKHR> const *available_present_modes);
  VkExtent2D chooseSwapExtent (VkSurfaceCapabilitiesKHR const *capabilities);
  VkImageView createImageView (VkImage, VkFormat, VkImageAspectFlags, uint32_t);

  VkFormat findSupportedFormat (std::vector<VkFormat> const *, VkImageTiling, VkFormatFeatureFlags);
  VkFormat findDepthFormat (void);

private:
  static constexpr uint32_t WIDTH  = 512;
  static constexpr uint32_t HEIGHT = 512;
  static constexpr SDL_WindowFlags WINDOW_FLAGS = SDL_WINDOW_VULKAN;

  static const std::vector<char const *> validation_layers;
  static const std::vector<char const *> device_extensions;
};

const std::vector<char const *> Game::validation_layers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<char const *> Game::device_extensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


Game::Game (void)
{
}

Game::~Game()
{
}


void
Game::run (void)
{
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}


void
Game::initWindow (void)
{
  SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

  window = SDL_CreateWindow("Connectors Game",
    Game::WIDTH, Game::HEIGHT,
    Game::WINDOW_FLAGS);

  if (window == nullptr)
    throw std::runtime_error("failed to create window");
}


void
Game::initVulkan (void)
{
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapchain();
  createImageViews();
  createRenderPass();
}


void
Game::cleanupSwapchain (void)
{
#if 0
  vkDestroyImageView(device, depth_image_view, nullptr);
  vkDestroyImage(device, depth_image, nullptr);
  vkFreeMemory(device, depth_image_memory, nullptr);

  vkDestroyImageView(device, color_image_view, nullptr);
  vkDestroyImage(device, color_image, nullptr);
  vkFreeMemory(device, color_image_memory, nullptr);

  for (auto framebuffer : swapchain_framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);
#endif

  for (auto image_view : swapchain_image_views)
    vkDestroyImageView(device, image_view, nullptr);

  vkDestroySwapchainKHR(device, swapchain, nullptr);
}


void
Game::cleanup (void)
{
  cleanupSwapchain();

  // ...
  vkDestroyRenderPass(device, render_pass, nullptr);
  // ...

  vkDestroyDevice(device, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  SDL_DestroyWindow(window);

  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}


void
Game::createInstance (void)
{

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Connectors Game";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.apiVersion = VK_API_VERSION_1_0;
  app_info.pNext = nullptr;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  auto extensions = getRequiredExtensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};

  if (enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
    populateDebugMessengerCreateInfo(debug_create_info);
    create_info.pNext = &debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("failed to create Vulkan instance");
}


void
Game::createSurface (void)
{
  if (! SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface) )
    throw std::runtime_error("failed to create KHR surface");
}


void
Game::pickPhysicalDevice (void)
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  if (device_count == 0)
    throw std::runtime_error("failed to find GPUs with Vulkan support");

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physical_device = device;
      msaa_samples = getMaxUsableSampleCount();
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE)
    throw std::runtime_error("failed to find a suitable GPU");
}


void
Game::createLogicalDevice (void)
{
  QueueFamilyIndices indices = findQueueFamilies(physical_device);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t> unique_queue_families = {
    indices.graphics_family.value(),
    indices.present_family.value()
  };

  float queue_priority = 1.0f;
  for (uint32_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};
  device_features.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();

  create_info.pEnabledFeatures = &device_features;

  create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  if (enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
  } else {
    create_info.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
    throw std::runtime_error("failed to create logical device");

  vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
  vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}


void
Game::createSwapchain (void)
{
  SwapchainSupportDetails swapchain_support = querySwapchainSupport(physical_device);

  VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(&swapchain_support.formats);
  VkPresentModeKHR present_mode = chooseSwapPresentMode(&swapchain_support.present_modes);
  VkExtent2D extent = chooseSwapExtent(&swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
  if (swapchain_support.capabilities.maxImageCount > 0
   && image_count > swapchain_support.capabilities.maxImageCount)
    image_count = swapchain_support.capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface;

  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physical_device);
  uint32_t queue_family_indices[2] = {
    indices.graphics_family.value(),
    indices.present_family.value()
  };

  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = swapchain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS)
    throw std::runtime_error("failed to create swap chain");

  vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
  swapchain_images.resize(image_count);
  vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());

  swapchain_image_format = surface_format.format;
  swapchain_extent = extent;
}


void
Game::createImageViews (void)
{
  swapchain_image_views.resize(swapchain_images.size());

  for (uint32_t i = 0; i < swapchain_images.size(); ++i)
    swapchain_image_views[i] = createImageView(swapchain_images[i],
      swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}


void
Game::createRenderPass (void)
{
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swapchain_image_format;
  color_attachment.samples = msaa_samples;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depth_attachment{};
  depth_attachment.format = findDepthFormat();
  depth_attachment.samples = msaa_samples;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription color_attachment_resolve{};
  color_attachment_resolve.format = swapchain_image_format;
  color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_resolve_ref{};
  color_attachment_resolve_ref.attachment = 2;
  color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;
  subpass.pResolveAttachments = &color_attachment_resolve_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                           | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                           | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 3> attachments = {
    color_attachment, depth_attachment, color_attachment_resolve
  };

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
  render_pass_info.pAttachments = attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass");
}


std::vector<char const *>
Game::getRequiredExtensions (void)
{
  uint32_t sdl_extension_count = 0;
  char const * const * sdl_extensions = nullptr;
  sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

  std::vector<char const *> extensions(sdl_extensions, sdl_extensions + sdl_extension_count);

  if (enable_validation_layers)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  for (char const *ext : extensions)
    std::cout << "  extension: " << ext << std::endl;

  return extensions;
}


VKAPI_ATTR
static VkBool32 VKAPI_CALL
debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT message_type,
               const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
               void *user_data)
{
  (void) message_severity;
  (void) message_type;
  (void) user_data;

  std::cerr << "validation layer: " << callback_data->pMessage << std::endl;

  return VK_FALSE;
}


void
Game::populateDebugMessengerCreateInfo (VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                          | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                          | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debugCallback;
}


SwapchainSupportDetails
Game::querySwapchainSupport (VkPhysicalDevice device)
{
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
      details.present_modes.data());
  }

  return details;
}


bool
Game::isDeviceSuitable (VkPhysicalDevice device)
{
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensions_supported = checkDeviceExtensionSupport(device);

  bool swapchain_adequate = false;
  if (extensions_supported) {
    SwapchainSupportDetails swapchain_support = querySwapchainSupport(device);
    swapchain_adequate = !swapchain_support.formats.empty()
                      && !swapchain_support.present_modes.empty();
  }

  VkPhysicalDeviceFeatures supported_features;
  vkGetPhysicalDeviceFeatures(device, &supported_features);

  return (indices.isComplete()
       && extensions_supported
       && swapchain_adequate
       && supported_features.samplerAnisotropy);
}


bool
Game::checkDeviceExtensionSupport (VkPhysicalDevice device)
{
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

  std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

  for (const auto &extension : available_extensions)
    required_extensions.erase(extension.extensionName);

  return required_extensions.empty();
}


VkSampleCountFlagBits
Game::getMaxUsableSampleCount (void)
{
  static const std::vector<VkSampleCountFlagBits> k_superior_samples = {
    VK_SAMPLE_COUNT_64_BIT,
    VK_SAMPLE_COUNT_32_BIT,
    VK_SAMPLE_COUNT_16_BIT,
    VK_SAMPLE_COUNT_8_BIT,
    VK_SAMPLE_COUNT_4_BIT,
    VK_SAMPLE_COUNT_2_BIT,
  };

  VkPhysicalDeviceProperties physical_device_properties;
  vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

  VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts
                            & physical_device_properties.limits.framebufferDepthSampleCounts;

  for (const auto &bit : k_superior_samples)
    if (counts & bit)
      return bit;

  return VK_SAMPLE_COUNT_1_BIT;
}


QueueFamilyIndices
Game::findQueueFamilies (VkPhysicalDevice device)
{
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  uint32_t i = 0;
  for (const auto &queue_family : queue_families) {

    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphics_family = i;

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support)
      indices.present_family = i;

    if (indices.isComplete())
      break;

    ++i;
  }

  return indices;
}


VkSurfaceFormatKHR
Game::chooseSwapSurfaceFormat (std::vector<VkSurfaceFormatKHR> const *available_formats)
{
  for (const auto &available_format : *available_formats)
    if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB
     && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return available_format;

  return (*available_formats)[0];
}


VkPresentModeKHR
Game::chooseSwapPresentMode (std::vector<VkPresentModeKHR> const *available_present_modes)
{
  for (const auto &mode : *available_present_modes)
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return mode;

  return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D
Game::chooseSwapExtent (VkSurfaceCapabilitiesKHR const *capabilities)
{
  if (capabilities->currentExtent.width != std::numeric_limits<uint32_t>::max())
    return capabilities->currentExtent;

  int width, height;
  SDL_GetWindowSize(window, &width, &height);

  VkExtent2D actual_extent = {
    static_cast<uint32_t>(width),
    static_cast<uint32_t>(height)
  };

  actual_extent.width = std::clamp(actual_extent.width,
    capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
  actual_extent.height = std::clamp(actual_extent.height,
    capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

  return actual_extent;
}


VkImageView
Game::createImageView (VkImage image, VkFormat format,
                       VkImageAspectFlags aspect_flags, uint32_t mip_levels)
{
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = aspect_flags;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = mip_levels;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkImageView image_view;
  if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS)
    throw std::runtime_error("failed to create image view");

  return image_view;
}


VkFormat
Game::findSupportedFormat (std::vector<VkFormat> const *candidates,
                           VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for (VkFormat format : *candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR
     && (props.linearTilingFeatures & features) == features)
      return format;

    if (tiling == VK_IMAGE_TILING_OPTIMAL
     && (props.optimalTilingFeatures & features) == features)
      return format;
  }

  throw std::runtime_error("failed to find supported format");
}


VkFormat
Game::findDepthFormat (void)
{
  std::vector<VkFormat> fmt = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
  };
  return findSupportedFormat(
    &fmt,
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}


void
Game::mainLoop (void)
{
}


int
main (int argc, char *argv[])
{
  (void) argc;
  (void) argv;

  Game game;

  game.run();

  return EXIT_SUCCESS;
}

