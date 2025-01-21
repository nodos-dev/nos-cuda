#include <Nodos/PluginAPI.h>
#include <Builtins_generated.h>
#include <Nodos/Helpers.hpp>
#include <AppService_generated.h>
#include <AppEvents_generated.h>
#include "nosTensorSubsystem/nosTensorSubsystem.h"
#include "Nodos/PluginHelpers.hpp"

NOS_INIT();
NOS_VULKAN_INIT();
NOS_CUDA_INIT();
NOS_TENSOR_INIT();

NOS_BEGIN_IMPORT_DEPS()
	NOS_VULKAN_IMPORT()
	NOS_CUDA_IMPORT()
	NOS_TENSOR_IMPORT()
NOS_END_IMPORT_DEPS()

nosResult RegisterTextureToBuffer(nosNodeFunctions* outFunctions);
nosResult RegisterVulkanBufferToCUDABuffer(nosNodeFunctions* outFunctions);
nosResult RegisterTextureFormatConverter(nosNodeFunctions* outFunctions);
nosResult RegisterTextureFormatConverter(nosNodeFunctions* outFunctions);
nosResult RegisterLinearToSRGB(nosNodeFunctions* outFunctions);

struct InteropPluginFunctions : nos::PluginFunctions
{
	nosResult ExportNodeFunctions(size_t& outCount, nosNodeFunctions** outFunctions) override
	{
		outCount = (size_t)(4);
		if (!outFunctions)
			return NOS_RESULT_SUCCESS;

		NOS_RETURN_ON_FAILURE(RegisterTextureToBuffer(outFunctions[0]));
		NOS_RETURN_ON_FAILURE(RegisterVulkanBufferToCUDABuffer(outFunctions[1]));
		NOS_RETURN_ON_FAILURE(RegisterTextureFormatConverter(outFunctions[2]));
		NOS_RETURN_ON_FAILURE(RegisterLinearToSRGB(outFunctions[3]));
		return NOS_RESULT_SUCCESS;
	}
};

NOS_EXPORT_PLUGIN_FUNCTIONS(InteropPluginFunctions)