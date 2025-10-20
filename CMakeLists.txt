# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

cmake_minimum_required(VERSION 3.24.2)
project("NOSCUDA")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)

macro(get_targets targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_targets(${targets} ${subdir})
    endforeach()
    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(subtarget ${current_targets}) 
        if(TARGET ${subtarget})
            list(APPEND ${targets} ${subtarget})
        endif()
    endforeach()
endmacro()

macro(group_targets targets folder_name)
    foreach(target ${targets})
        get_target_property(FOLD ${target} FOLDER)
        if(${FOLD} STREQUAL "FOLD-NOTFOUND")
            set(FOLD_NAME "${folder_name}")
        else()
            set(FOLD_NAME "${folder_name}/${FOLD}")
        endif()
        set_target_properties(${target} PROPERTIES FOLDER ${FOLD_NAME})
    endforeach()
endmacro()

add_compile_definitions(NOS_DISABLE_DEPRECATED)
nos_find_plugin_sdk("40.0.0" NOS_PLUGIN_SDK_TARGET NOS_SDK_DIR)

# Check for environment variable & cmake variable
if(NOT DEFINED CUDA_SDK_PATH)
	if(NOT DEFINED ENV{CUDA_PATH})
		message(FATAL_ERROR "CUDA_PATH environment or cmake variable is not set.")
	endif()
	set(CUDA_SDK_PATH $ENV{CUDA_PATH})
endif()
message(STATUS "CUDA_PATH: ${CUDA_SDK_PATH}")

set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Tools/Windows)

add_subdirectory("nosCUDASubsystem" "${CMAKE_CURRENT_BINARY_DIR}/nosCUDASubsystem")
get_targets(SUBSYSTEM_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/nosCUDASubsystem")
group_targets("${SUBSYSTEM_TARGETS}" "NOS Subsystems")

add_subdirectory("Plugins" "${CMAKE_CURRENT_BINARY_DIR}/Plugins")
get_targets(PLUGIN_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/Plugins")
group_targets("${PLUGIN_TARGETS}" "NOS Plugins")