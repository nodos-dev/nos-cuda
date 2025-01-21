#include "Globals.h"
#include "cuda.h"
namespace nos::cudass
{
	UtilsProxy::ResourceManagerProxy<nosCUDABufferInfo> ResManager;
	UtilsProxy::ResourceManagerProxy<nosCUDAStream> StreamManager;
	uint32_t CurrentDevice = 0;
	void* PrimaryContext = nullptr;
	void* ActiveContext = nullptr;
	std::unordered_map<nos::Name, void*> IDContextMap;
}