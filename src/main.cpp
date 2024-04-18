#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount);
  createInfo.enabledLayerCount = 0;

  VkInstance instance;
  if (VK_SUCCESS != vkCreateInstance(&createInfo, nullptr, &instance))
  {
    throw std::runtime_error("failed to create instance!");
  }

  glm::mat4 matrix;
  glm::vec4 vec;
  auto test = matrix * vec;

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }

  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
