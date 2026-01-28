#pragma once
#include "nosSysCuda/nosCudaSubsystem.h"
#include "Common.hpp"
#include <Nodos/Plugin.hpp>
namespace nos::sys::cuda 
{
extern ResourceManager<nos::ObjectRef> ResManager;
extern uint32_t CurrentDevice;
extern void* PrimaryContext;
extern void* ActiveContext;
extern std::unordered_map<nos::Name, void*> IDContextMap;
}
