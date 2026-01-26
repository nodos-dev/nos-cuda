# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

# Check for environment variable & cmake variable
if(NOT DEFINED CUDA_SDK_PATH)
	if(NOT DEFINED ENV{CUDA_PATH})
		message(FATAL_ERROR "CUDA_PATH environment or cmake variable is not set.")
	endif()
	set(CUDA_SDK_PATH $ENV{CUDA_PATH})
endif()
nos_message(STATUS "CUDA Path: ${CUDA_SDK_PATH}")

find_library(cuda.lib NAMES cuda.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(cudadevrt.lib NAMES cudadevrt.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(cudart.lib NAMES cudart.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(cudart_static.lib NAMES cudart_static.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(OpenCL.lib NAMES OpenCL.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)

target_link_libraries(${NOS_PLUGIN_TARGET} PRIVATE cuda.lib cudadevrt.lib cudart.lib cudart_static.lib OpenCL.lib)
target_include_directories(${NOS_PLUGIN_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${CUDA_SDK_PATH}/include")
target_link_directories(${NOS_PLUGIN_TARGET} PRIVATE "${CUDA_SDK_PATH}/lib/x64/")

set_target_properties(${NOS_PLUGIN_TARGET} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_target_properties(${NOS_PLUGIN_TARGET} PROPERTIES CUDA_ARCHITECTURES native)

