nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Include/nosCudaSubsystem" "cpp" "" nosCUDA_generated)

find_library(cuda.lib NAMES cuda.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(cudadevrt.lib NAMES cudadevrt.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(cudart.lib NAMES cudart.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(cudart_static.lib NAMES cudart_static.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)
find_library(OpenCL.lib NAMES OpenCL.lib PATHS ${CUDA_SDK_PATH}/lib/x64 REQUIRED NO_DEFAULT_PATH)

execute_process(COMMAND python "${CMAKE_CURRENT_LIST_DIR}/generate_cuda_kernels.py" -folder="${CMAKE_CURRENT_LIST_DIR}/Source" -nvcc="${CUDA_SDK_PATH}/bin/nvcc.exe" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

nos_add_subsystem("nosCudaSubsystem" "nosCUDA_generated;${NOS_SUBSYSTEM_SDK_TARGET};${CUDA_SDK_PATH}/lib/x64/cuda.lib;${CUDA_SDK_PATH}/lib/x64/cudadevrt.lib;${CUDA_SDK_PATH}/lib/x64/cudart.lib;${CUDA_SDK_PATH}/lib/x64/cudart_static.lib;${CUDA_SDK_PATH}/lib/x64/OpenCL.lib"

"${CMAKE_CURRENT_SOURCE_DIR}/Include;${CUDA_SDK_PATH}/include")

link_directories(${CUDA_SDK_PATH}/lib/x64/)
set_target_properties(nosCudaSubsystem PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_target_properties(nosCudaSubsystem PROPERTIES CUDA_ARCHITECTURES native)
set(CMAKE_VERBOSE_MAKEFILE ON)

target_compile_definitions(nosCudaSubsystem PRIVATE BUILDING_NOS_VULKAN_SUBSYSTEM)
