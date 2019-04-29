cmake_minimum_required(VERSION 3.10)

project(LearnVulkan)
set(CMAKE_BUILD_TYPE Debug)

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

include_directories(
../glfw/include 
../glm 
../VulkanSDK/1.1.101.0/Include
../stb
)

LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

link_libraries(../glfw/lib/glfw3 ../VulkanSDK/1.1.101.0/Lib32/vulkan-1)

list(APPEND
	ShaderFiles
	resource/shaders/vertex.vert
	resource/shaders/pixel.frag
)

list(APPEND
	Textures
	resource/Textures/texture.jpg
)

source_group(res\\shaders FILES ${ShaderFiles})
source_group(res\\texutures FILES ${Textures})

add_subdirectory(resource)

add_executable(${PROJECT_NAME} 
	main.cpp
	${ShaderFiles}
	${Textures}
)

#target_link_libraries(../glfw/lib/glfw3_d)

