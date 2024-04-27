cmake_minimum_required(VERSION 3.29)
project(vulcan-cubes CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(stb REQUIRED)
find_package(tinyobjloader REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME}
  Vulkan::Vulkan
  glfw
  tinyobjloader::tinyobjloader
  glm::glm
  stb::stb
)

file(GLOB_RECURSE GLSL_SOURCE_FILES
  "shaders/*.frag"
  "shaders/*.vert"
)
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  string(REPLACE "shader." "" SPIRV ${GLSL})
  set(SPIRV "${SPIRV}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
  ${PROJECT_NAME}Shaders 
  DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}Shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_SOURCE_DIR}/shaders"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/textures/"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_SOURCE_DIR}/textures"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/textures"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_SOURCE_DIR}/models"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models"
)
