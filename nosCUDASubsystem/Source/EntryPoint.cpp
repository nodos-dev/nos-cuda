// Copyright MediaZ Teknoloji A.S. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include "Services.h"

NOS_INIT();

NOS_BEGIN_IMPORT_DEPS()
NOS_END_IMPORT_DEPS()

namespace nos::cudass
{
extern "C"
{

NOSAPI_ATTR nosResult NOSAPI_CALL nosExportPlugin(nosPluginFunctions* pluginFunctions)
{
	nos::cudass::Initialize(0);
	pluginFunctions->OnRequestAPI = [](uint32_t minor, void** outPluginAPI) -> nosResult {
		(*outPluginAPI) = new nosCUDASubsystem;
		nos::cudass::Bind((nosCUDASubsystem*)(*outPluginAPI));
		return NOS_RESULT_SUCCESS;
	};
	// TODO: Garbage Collect on unload

	return NOS_RESULT_SUCCESS;
}
}

}
