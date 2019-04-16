cmake_minimum_required(VERSION 3.10)

set(shaderFiles "vert.spv" "frag.spv")

set(shadersPath ${PROJECT_SOURCE_DIR}/shaders)

set(shaderCompile ${PROJECT_SOURCE_DIR}/../VulkanSDK/1.1.101.0/Bin32/glslangValidator.exe)

add_custom_target(CompileShaderAndCopy ALL DEPENDS ${shaderFiles})

add_custom_command(TARGET CompileShaderAndCopy
	POST_BUILD

	COMMAND echo Compile Shader...
	COMMAND ${shaderCompile} -V ${shadersPath}/pixel.frag -o ${shadersPath}/frag.spv
	COMMAND ${shaderCompile} -V ${shadersPath}/vertex.vert -o ${shadersPath}/vert.spv
	COMMAND echo Copy Shader
	COMMAND cd ${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/
	COMMAND del /s /q Shaders
	COMMAND rd Shaders
	COMMAND mkdir Shaders
	#COMMAND	xcopy /s /c /q /r /y "${shadersPath}/*.spv" "Shaders/"
	COMMAND	${CMAKE_COMMAND} -E copy  ${shadersPath}/vert.spv ${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/Shaders/
	COMMAND	${CMAKE_COMMAND} -E copy  ${shadersPath}/frag.spv ${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/Shaders/
)