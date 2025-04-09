
nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "cpp" "" nosInterop_generated)

nos_add_plugin("nosInterop" "nosAIPluginsCommon;${NOS_SYS_VULKAN_TARGET};${NOS_PLUGIN_SDK_TARGET};nosInterop_generated" 
	"${CMAKE_CURRENT_SOURCE_DIR};${SUBSYSTEMS_SOURCE_DIR}/nosCUDASubsystem/Include;${SUBSYSTEMS_SOURCE_DIR}/nosTensorSubsystem/Include")

set_target_properties(nosInterop PROPERTIES OUTPUT_NAME "nosInterop")