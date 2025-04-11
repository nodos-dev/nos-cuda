# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

nos_add_plugin("nosCUDAPlugin" "${NOS_SYS_CUDA_TARGET};${NOS_PLUGIN_SDK_TARGET}" 
	"${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(nosCUDAPlugin PROPERTIES OUTPUT_NAME "nosCUDAPlugin")