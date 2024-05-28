
nos_add_plugin("nosCUDAPlugin" "${NOS_PLUGIN_SDK_TARGET}" 
	"${CMAKE_CURRENT_SOURCE_DIR};${SUBSYSTEMS_SOURCE_DIR}/nosCUDASubsystem/Include")

set_target_properties(nosCUDAPlugin PROPERTIES OUTPUT_NAME "nosCUDAPlugin")