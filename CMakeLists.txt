cmake_minimum_required(VERSION 3.10)

project(LearnVulkan)
set(CMAKE_BUILD_TYPE Debug)


include_directories(../glfw/include ../glm ../VulkanSDK/1.1.101.0/Include)

LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

link_libraries(../glfw/lib/glfw3 ../VulkanSDK/1.1.101.0/Lib32/vulkan-1)

list(APPEND
	ShaderFiles
	shaders/vertex.vert
	shaders/pixel.frag
)

source_group(Shaders FILES ${ShaderFiles})

add_subdirectory(shaders)

add_executable(${PROJECT_NAME} 
	main.cpp
	${ShaderFiles}
)

#target_link_libraries(../glfw/lib/glfw3_d)

