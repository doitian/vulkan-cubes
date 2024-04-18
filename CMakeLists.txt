cmake_minimum_required(VERSION 3.29)
project(vulcan-cubes CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw)
if(WIN32)
  target_link_libraries(${PROJECT_NAME} "${glfw_LIB_DIRS_RELEASE}/glfw3.lib")
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${glfw3_INCLUDE_DIRS} ${glm_INCLUDE_DIRS})
