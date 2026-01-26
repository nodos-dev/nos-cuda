#include <Nodos/PluginAPI.h>
#include <Builtins_generated.h>
#include <Nodos/Plugin.hpp>
#include <AppService_generated.h>
#include <AppEvents_generated.h>
#include "Nodos/Plugin.hpp"
#include <nosSysVulkan/nosVulkanSubsystem.h>
#include <nosSysCuda/nosCUDASubsystem.h>

NOS_INIT();
NOS_VULKAN_INIT();
NOS_CUDA_INIT();

NOS_BEGIN_IMPORT_DEPS()
	NOS_VULKAN_IMPORT()
	NOS_CUDA_IMPORT()
NOS_END_IMPORT_DEPS()

nosResult RegisterVulkanBufferToCUDABuffer(nosNodeFunctions* outFunctions);
nosResult RegisterLinearToSRGB(nosNodeFunctions* outFunctions);

struct InteropPluginFunctions : nos::PluginFunctions
{
	nosResult ExportNodeFunctions(size_t& outCount, nosNodeFunctions** outFunctions) override
	{
		outCount = (size_t)(2);
		if (!outFunctions)
			return NOS_RESULT_SUCCESS;

		NOS_RETURN_ON_FAILURE(RegisterVulkanBufferToCUDABuffer(outFunctions[0]));
		NOS_RETURN_ON_FAILURE(RegisterLinearToSRGB(outFunctions[1]));
		return NOS_RESULT_SUCCESS;
	}
};

NOS_EXPORT_PLUGIN_FUNCTIONS(InteropPluginFunctions)