// Copyright MediaZ Teknoloji A.S. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include "Services.h"
#include "Nodos/Name.hpp"
#include "nosSysCuda/Types_generated.h"

NOS_INIT();

NOS_BEGIN_IMPORT_DEPS()
NOS_END_IMPORT_DEPS()

namespace nos::sys::cuda
{
extern "C"
{

nosResult NOSAPI_CALL ExportObjectTypeFunctions(size_t* outCount, nosObjectTypeInfo** outList)
{
	if (outCount)
		*outCount = 2;
	if (outList)
	{
		auto stream =  outList[0];
		stream->TypeName = NSN_StreamTypeName;
		stream->Functions = {
			.Construct = type::ConstructStreamObject,
			.Release = type::ReleaseStreamObject,
		};
		stream->PinInterface = {
			.InitializeObject = type::InitializeStreamPinObject,
			.OnInputPinDisconnected = type::OnStreamInputPinDisconnected
		};
		auto buffer = outList[1];
		
	}
	return NOS_RESULT_SUCCESS;
}

NOSAPI_ATTR nosResult NOSAPI_CALL nosExportPlugin(nosPluginFunctions* pluginFunctions)
{
	Initialize(0);
	pluginFunctions->OnRequestAPI = [](uint32_t minor, void** outPluginAPI) -> nosResult {
		(*outPluginAPI) = new nosCudaSubsystem;
		Bind((nosCudaSubsystem*)(*outPluginAPI));
		return NOS_RESULT_SUCCESS;
	};
	// TODO: Garbage Collect on unload
	pluginFunctions->ExportObjectTypeInfos = ExportObjectTypeFunctions;

	return NOS_RESULT_SUCCESS;
}
}

}
