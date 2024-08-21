#include <Nodos/PluginAPI.h>
#include <Builtins_generated.h>
#include <Nodos/PluginHelpers.hpp>
#include <AppService_generated.h>
#include <AppEvents_generated.h>
#include "nosCUDASubsystem/nosCUDASubsystem.h"

NOS_INIT();
NOS_CUDA_INIT();

NOS_BEGIN_IMPORT_DEPS()
	NOS_CUDA_IMPORT()
NOS_END_IMPORT_DEPS()

void RegisterCreateStream(nosNodeFunctions* outFunctions);

struct CUDANodesPluginFunctions : nos::PluginFunctions
{
	nosResult ExportNodeFunctions(size_t& outSize, nosNodeFunctions** outList) override
	{
		outSize = 1;
		if (!outList)
			return NOS_RESULT_SUCCESS;

		RegisterCreateStream(outList[0]);

		return NOS_RESULT_SUCCESS;
	}
};

NOS_EXPORT_PLUGIN_FUNCTIONS(CUDANodesPluginFunctions)