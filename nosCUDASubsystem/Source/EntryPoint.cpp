// Copyright MediaZ Teknoloji A.S. All Rights Reserved.
#include <Nodos/SubsystemAPI.h>
#include "Services.h"

NOS_INIT();

namespace nos::cudass
{
	extern "C"
	{

		NOSAPI_ATTR nosResult NOSAPI_CALL nosExportSubsystem(nosSubsystemFunctions* subsystemFunctions)
		{
			nos::cudass::Initialize(0);
			
			subsystemFunctions->OnRequest = [](uint32_t minor, void** outSubsystemCtx) -> nosResult {(*outSubsystemCtx) = new nosCUDASubsystem;
																									 nos::cudass::Bind((nosCUDASubsystem*)(*outSubsystemCtx));
																									return NOS_RESULT_SUCCESS; };

			subsystemFunctions->OnPreExecuteNode = [](nosNodeExecuteArgs* args) -> nosResult {return NOS_RESULT_SUCCESS; };
			subsystemFunctions->OnSendEngineMetrics = [](float timeSinceLastSent) -> void {return; };
			subsystemFunctions->OnRegisterOptions = [](nosModuleIdentifier module, nosBuffer blob)->nosResult {return NOS_RESULT_SUCCESS; };
			subsystemFunctions->OnMessageFromEditor = [](uint64_t editorId, nosBuffer blob) -> void {return; };
			return NOS_RESULT_SUCCESS;
		}

		NOSAPI_ATTR nosResult NOSAPI_CALL nosExportSubsystemTypeFunctions(size_t* outSize, nosSubsystemTypeFunctions** outList)
		{
			*outSize = 0;
			if (!outList)
				return NOS_RESULT_SUCCESS;
			return NOS_RESULT_SUCCESS;
		}

		NOSAPI_ATTR nosResult NOSAPI_CALL nosExportSubsystemNodeFunctions(size_t* outSize, nosSubsystemNodeFunctions** outList)
		{
			*outSize = 0;
			if (!outList)
				return NOS_RESULT_SUCCESS;
			return NOS_RESULT_SUCCESS;
		}

		NOSAPI_ATTR nosResult NOSAPI_CALL nosUnloadSubsystem()
		{
			//TODO: Garbage Collect?
			return NOS_RESULT_SUCCESS;
		}
	}

}
