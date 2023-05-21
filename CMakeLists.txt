cmake_minimum_required(VERSION 3.25)
project(vkOcclusionTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(vkOcclusionTest main.cpp GraphicsPipeline.cpp GraphicsPipeline.h Buffer.cpp Buffer.h Instance.cpp Instance.h Swapchain.cpp Swapchain.h GlobalTypes.h FrameData.cpp FrameData.h Texture.cpp Texture.h HZBuffer.cpp HZBuffer.h ComputePipeline.cpp ComputePipeline.h Utils.cpp Utils.h Sampler.cpp Sampler.h)
target_link_libraries(vkOcclusionTest PRIVATE SDL2::SDL2 glm::glm Vulkan::Vulkan)
target_include_directories(vkOcclusionTest PRIVATE ${Stb_INCLUDE_DIR})
target_compile_definitions(vkOcclusionTest PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 GLM_FORCE_DEPTH_ZERO_TO_ONE=1)

set(vkOcclusion_SHADER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/main.vert ${CMAKE_CURRENT_SOURCE_DIR}/shaders/main.frag ${CMAKE_CURRENT_SOURCE_DIR}/shaders/downsample.comp ${CMAKE_CURRENT_SOURCE_DIR}/shaders/copy.comp)

foreach(SHADER IN LISTS vkOcclusion_SHADER_SOURCES)
	get_filename_component(SHADER_FILENAME ${SHADER} NAME)
	set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_FILENAME}.spv)
	add_custom_command(OUTPUT ${SHADER_OUTPUT}
			COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
			DEPENDS ${SHADER}
			COMMENT "Compiling SPIR-V Shader ${SHADER}")
	list(APPEND vkOcclusion_COMPILED_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(vkOcclusion_SHADERS ALL DEPENDS ${vkOcclusion_COMPILED_SHADERS})
add_dependencies(vkOcclusionTest vkOcclusion_SHADERS)