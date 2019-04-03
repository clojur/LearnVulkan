cmake_minimum_required(VERSION 3.10)

project(LearnVulkan)

include_directories(../glfw/include ../glm ../VulkanSDK/1.1.101.0/Include)

LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

link_libraries(../glfw/lib/glfw3_ds ../VulkanSDK/1.1.101.0/Lib32/vulkan-1)

add_executable(${PROJECT_NAME} main.cpp)

#target_link_libraries(../glfw/lib/glfw3_d)

