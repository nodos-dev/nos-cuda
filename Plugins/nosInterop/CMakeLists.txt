
nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "cpp" "" nosInterop_generated)

message("Will generate PTX headers, given file: ${CMAKE_CURRENT_LIST_DIR}/Source")
execute_process(COMMAND python "${CMAKE_CURRENT_LIST_DIR}/generate_cuda_kernels.py" -folder="${CMAKE_CURRENT_LIST_DIR}/Source" -nvcc="${NVCC_DIR}/nvcc.exe" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

nos_add_plugin("nosInterop" "${NOS_PLUGIN_SDK_TARGET};${NOS_SYS_VULKAN_TARGET};nosInterop_generated" 
	"${CMAKE_CURRENT_SOURCE_DIR};${SUBSYSTEMS_SOURCE_DIR}/nosCUDASubsystem/Include;${SUBSYSTEMS_SOURCE_DIR}/nosTensorSubsystem/Include")

set_target_properties(nosInterop PROPERTIES OUTPUT_NAME "nosInterop")