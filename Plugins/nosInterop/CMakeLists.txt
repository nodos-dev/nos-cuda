# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Types" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "cpp" "" nosInterop_generated)

nos_add_plugin("nosInterop" "${NOS_SYS_CUDA_TARGET};${NOS_SYS_VULKAN_TARGET};${NOS_PLUGIN_SDK_TARGET};nosInterop_generated" 
	"${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(nosInterop PROPERTIES OUTPUT_NAME "nosInterop")