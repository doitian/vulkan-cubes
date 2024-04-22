#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>

struct QueueFamilyIndices
{
  uint32_t graphicsFamily;
  uint32_t presentFamily;

  bool isComplete(uint32_t guard) const
  {
    return graphicsFamily < guard && presentFamily < guard;
  }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;

  bool isAdequate() const
  {
    return !formats.empty() && !presentModes.empty();
  }

  VkSurfaceFormatKHR chooseFormat() const
  {
    for (const auto &candidate : formats)
    {
      if (candidate.format == VK_FORMAT_B8G8R8A8_SRGB && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return candidate;
      }
    }
    return formats[0];
  }

  VkPresentModeKHR choosePresentMode() const
  {
    for (const auto &candidate : presentModes)
    {
      if (candidate == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        return candidate;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(GLFWwindow *window) const
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  details.formats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
  details.presentModes.resize(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

  return details;
}

const char *const REQUIRED_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const size_t REQUIRED_EXTENSIONS_SIZE = sizeof(REQUIRED_EXTENSIONS) / sizeof(REQUIRED_EXTENSIONS[0]);

class App
{
  GLFWwindow *window = nullptr;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
  }

  ~App()
  {
    for (auto imageView : this->swapChainImageViews)
    {
      vkDestroyImageView(this->device, imageView, nullptr);
    }

    if (this->swapChain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
    }

    if (this->device != VK_NULL_HANDLE)
    {
      vkDestroyDevice(this->device, nullptr);
    }

    if (this->surface != VK_NULL_HANDLE)
    {
      vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    }

    if (this->instance != VK_NULL_HANDLE)
    {
      vkDestroyInstance(this->instance, nullptr);
    }

    if (this->window != nullptr)
    {
      glfwDestroyWindow(this->window);
      glfwTerminate();
    }
  }

private:
  void initWindow()
  {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
  }

  void initVulkan()
  {
    this->instance = createInstance();

    uint32_t graphicsFamilyIndex;
    if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }
    auto [physicalDevice, indices] = pickPhysicalDevice(this->instance, this->surface);
    this->physicalDevice = physicalDevice;
    this->device = createDevice(physicalDevice, indices);
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &this->graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &this->presentQueue);

    createSwapChain(indices);
    createImageViews();
  }

  void createSwapChain(const QueueFamilyIndices &indices)
  {
    auto swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    auto surfaceFormat = swapChainSupport.chooseFormat();
    auto presentMode = swapChainSupport.choosePresentMode();
    auto extent = swapChainSupport.chooseSwapExtent(window);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    if (indices.graphicsFamily != indices.presentFamily)
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;     // Optional
      createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &this->swapChain) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create swap chain!");
    }
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }

  void createImageViews()
  {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = swapChainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = swapChainImageFormat;
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create image views!");
      }
    }
  }

  static VkInstance createInstance()
  {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = 0;

    uint32_t glfwEnabledExtensionCount = 0;
    const char **glfwEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&glfwEnabledExtensionCount);
    std::vector<const char *> enabledExtensions(glfwEnabledExtensionNames, glfwEnabledExtensionNames + glfwEnabledExtensionCount);
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();

    VkInstance instance = VK_NULL_HANDLE;
    if (VK_SUCCESS != vkCreateInstance(&instanceCreateInfo, nullptr, &instance))
    {
      throw std::runtime_error("failed to create instance!");
    }
    return instance;
  }

  static std::pair<VkPhysicalDevice, QueueFamilyIndices> pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
  {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    auto indices = QueueFamilyIndices{};
    for (const auto &device : devices)
    {
      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

      indices.graphicsFamily = queueFamilyCount;
      indices.presentFamily = queueFamilyCount;
      for (uint32_t i = 0; i < queueFamilyCount; ++i)
      {
        if (indices.graphicsFamily == queueFamilyCount && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
          indices.graphicsFamily = i;
          if (indices.isComplete(queueFamilyCount))
          {
            break;
          }
        }

        if (indices.presentFamily == queueFamilyCount)
        {
          VkBool32 presentSupport = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
          if (presentSupport)
          {
            indices.presentFamily = i;
            if (indices.isComplete(queueFamilyCount))
            {
              break;
            }
          }
        }
      }

      if (indices.isComplete(queueFamilyCount) && checkDeviceExtensionSupport(device))
      {
        return std::make_pair(device, std::move(indices));
      }
    }

    throw std::runtime_error("failed to find a suitable GPU!");
  }

  static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
  {
    auto requiredExtensions = std::vector<std::string>(REQUIRED_EXTENSIONS, REQUIRED_EXTENSIONS + REQUIRED_EXTENSIONS_SIZE);

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    for (const auto &extension : availableExtensions)
    {
      std::erase(requiredExtensions, extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  static VkDevice
  createDevice(VkPhysicalDevice physicalDevice, const QueueFamilyIndices &indices)
  {
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriorities = 1.0f;
    std::transform(uniqueQueueFamilies.cbegin(), uniqueQueueFamilies.cend(), std::back_inserter(queueCreateInfos),
                   [indices, &queuePriorities](uint32_t queueFamily)
                   {
                     VkDeviceQueueCreateInfo queueCreateInfo{};
                     queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                     queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
                     queueCreateInfo.queueCount = 1;
                     queueCreateInfo.pQueuePriorities = &queuePriorities;
                     return queueCreateInfo;
                   });

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = REQUIRED_EXTENSIONS_SIZE;
    deviceCreateInfo.ppEnabledExtensionNames = REQUIRED_EXTENSIONS;

    VkDevice device;
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create logical device!");
    }
    return device;
  }

  void mainLoop()
  {
    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();
    }
  }
};

int main()
{
  App app;
  app.run();
  return 0;
}
