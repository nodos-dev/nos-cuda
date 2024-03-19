
if (GENERATE_FLATBUFFERS_FILES)
    generate_flatbuffers("${TOOLS_DIR}/flatc" "${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${CMAKE_CURRENT_SOURCE_DIR}/__temp__" "cpp")
endif()

message("Will generate PTX headers, given file: ${CMAKE_CURRENT_LIST_DIR}/Source")
execute_process(COMMAND python "${CMAKE_CURRENT_LIST_DIR}/generate_cuda_kernels.py" -folder="${CMAKE_CURRENT_LIST_DIR}/Source" -nvcc="${NVCC_DIR}/nvcc.exe" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

add_folder_as_plugin_project("nosInterop" "" 
	"${CMAKE_CURRENT_SOURCE_DIR};${NOS_SYS_VULKAN_INCLUDE_FOLDER};${SUBSYSTEMS_SOURCE_DIR}/nosCUDASubsystem/Include;${SUBSYSTEMS_SOURCE_DIR}/nosTensorSubsystem/Include")

set_target_properties(nosInterop PROPERTIES OUTPUT_NAME "nosInterop")