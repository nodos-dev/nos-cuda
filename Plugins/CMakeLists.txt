# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

cmake_minimum_required(VERSION 3.24.2)
project("Plugins")
# Dependencies
# ------------

unset(PYTHON_PATH CACHE)
find_program(PYTHON_PATH "python")
if(PYTHON_PATH STREQUAL "")
    message("Python not found. Looking for python3.")
    find_program(PYTHON_PATH "python3")
    if(PYTHON_PATH STREQUAL "")
        message(FATAL_ERROR "Could not find python")
    else()
        message("Found python3 ${PYTHON_PATH}")
    endif()
else()
    message("Found python ${PYTHON_PATH}")
endif()

nos_get_module("nos.sys.vulkan" "6.22" NOS_SYS_VULKAN_TARGET)
nos_get_module("nos.sys.cuda" "2.10" NOS_SYS_CUDA_TARGET)

execute_process(COMMAND python  "${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.py" RESULT_VARIABLE COMPILE_SHADERS_RESULT)

if (NOT ${COMPILE_SHADERS_RESULT} EQUAL "0")
message(FATAL_ERROR "Failed to compile shaders. Process returned ${COMPILE_SHADERS_RESULT}.")
endif()

add_subdirectory(nosInterop)
add_subdirectory(nosCUDA)
