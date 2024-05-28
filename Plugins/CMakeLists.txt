# Copyright MediaZ AS. All Rights Reserved.

cmake_minimum_required(VERSION 3.24.2)
project("AI Plugins")
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


execute_process(COMMAND python  "${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.py" RESULT_VARIABLE COMPILE_SHADERS_RESULT)

if (NOT ${COMPILE_SHADERS_RESULT} EQUAL "0")
    message(FATAL_ERROR "Failed to compile shaders. Process returned ${COMPILE_SHADERS_RESULT}.")
endif()

add_subdirectory(nosTensors)
add_subdirectory(nosONNX)
add_subdirectory(nosInterop)
add_subdirectory(nosAIModelNodes)
add_subdirectory(nosTokenizer)
add_subdirectory(nosAI)
add_subdirectory(nosCUDA)
