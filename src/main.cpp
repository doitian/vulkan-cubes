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

  bool isComplete(uint32_t guard)
  {
    return graphicsFamily < guard && presentFamily < guard;
  }
};

class App
{
  GLFWwindow *window = nullptr;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
  }

  ~App()
  {
    if (this->device != VK_NULL_HANDLE)
    {
      vkDestroyDevice(this->device, nullptr);
    }

    if (this->surface != VK_NULL_HANDLE)
    {
      vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    if (this->instance != VK_NULL_HANDLE)
    {
      vkDestroyInstance(this->instance, nullptr);
    }

    if (this->window != nullptr)
    {
      glfwDestroyWindow(window);
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
    if (glfwCreateWindowSurface(instance, window, nullptr, &this->surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }
    auto [physicalDevice, indices] = pickPhysicalDevice(this->instance, this->surface);
    this->physicalDevice = physicalDevice;
    this->device = createDevice(physicalDevice, indices);
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &this->graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &this->presentQueue);
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
    instanceCreateInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instanceCreateInfo.enabledExtensionCount);
    instanceCreateInfo.enabledLayerCount = 0;

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

      if (indices.isComplete(queueFamilyCount))
      {
        return std::make_pair(device, std::move(indices));
      }
    }

    throw std::runtime_error("failed to find a suitable GPU!");
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
