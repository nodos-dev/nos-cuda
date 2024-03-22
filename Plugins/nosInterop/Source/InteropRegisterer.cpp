#include <Nodos/PluginAPI.h>
#include <Builtins_generated.h>
#include <Nodos/Helpers.hpp>
#include <AppService_generated.h>
#include <AppEvents_generated.h>
#include "nosTensorSubsystem/nosTensorSubsystem.h"

NOS_INIT();
NOS_VULKAN_INIT();
NOS_CUDA_INIT();
NOS_TENSOR_INIT();

NOS_REGISTER_NAME(VulkanBufferToCUDABuffer)
NOS_REGISTER_NAME(InputBuffer)
NOS_REGISTER_NAME(OutputBuffer);

NOS_REGISTER_NAME(TextureToBuffer)
NOS_REGISTER_NAME(TextureFormatConverter)
NOS_REGISTER_NAME(Input)
NOS_REGISTER_NAME(Output)
NOS_REGISTER_NAME(OutputFormat)
NOS_REGISTER_NAME(FloatToIntFormat)
NOS_REGISTER_NAME(FloatToIntFormat_Pass)

NOS_REGISTER_NAME(InputTexture)
NOS_REGISTER_NAME(outputType)
NOS_REGISTER_NAME(DST_TEXTURE_UINT32)
NOS_REGISTER_NAME(DST_TEXTURE_UINT16)
NOS_REGISTER_NAME(DST_TEXTURE_UINT8)
NOS_REGISTER_NAME(DST_TEXTURE_INT32)
NOS_REGISTER_NAME(DST_TEXTURE_INT16)
NOS_REGISTER_NAME(DST_TEXTURE_INT8)

NOS_REGISTER_NAME(LinearToSRGB);
NOS_REGISTER_NAME(LinearToSRGB_Shader);
NOS_REGISTER_NAME(LinearToSRGB_Pass);

nosResult RegisterTextureToBuffer(nosNodeFunctions* outFunctions);
nosResult RegisterVulkanBufferToCUDABuffer(nosNodeFunctions* outFunctions);
nosResult RegisterTextureFormatConverter(nosNodeFunctions* outFunctions);
nosResult RegisterTextureFormatConverter(nosNodeFunctions* outFunctions);
nosResult RegisterLinearToSRGB(nosNodeFunctions* outFunctions);

extern "C"
{
	NOSAPI_ATTR nosResult NOSAPI_CALL nosExportNodeFunctions(size_t* outCount, nosNodeFunctions** outFunctions)
	{
		*outCount = (size_t)(4);
		if (!outFunctions)
			return NOS_RESULT_SUCCESS;

		NOS_RETURN_ON_FAILURE(RequestVulkanSubsystem());
		NOS_RETURN_ON_FAILURE(RequestCUDASubsystem());
		NOS_RETURN_ON_FAILURE(RequestTensorSubsystem());

		NOS_RETURN_ON_FAILURE(RegisterTextureToBuffer(outFunctions[0]));
		NOS_RETURN_ON_FAILURE(RegisterVulkanBufferToCUDABuffer(outFunctions[1]));
		NOS_RETURN_ON_FAILURE(RegisterTextureFormatConverter(outFunctions[2]));
		NOS_RETURN_ON_FAILURE(RegisterLinearToSRGB(outFunctions[3]));
		return NOS_RESULT_SUCCESS;
	}
}