#pragma once
#include "nosCUDASubsystem/nosCUDASubsystem.h"
#include "CUDASubsysCommon.h"
#include <Nodos/Helpers.hpp>
namespace nos::cudass 
{
	extern UtilsProxy::ResourceManagerProxy<nosCUDABufferInfo> ResManager;
	extern UtilsProxy::ResourceManagerProxy<nosCUDAStream> StreamManager;
	extern uint32_t CurrentDevice;
	extern void* PrimaryContext;
	extern void* ActiveContext;
	extern std::unordered_map<nos::Name, void*> IDContextMap;
}