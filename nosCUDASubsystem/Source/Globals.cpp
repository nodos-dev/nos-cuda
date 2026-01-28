#include "Globals.h"
#include "cuda.h"
#include "Common.hpp"

namespace nos::sys::cuda
{
ResourceManager<nos::ObjectRef> ResManager;
uint32_t CurrentDevice = 0;
void* PrimaryContext = nullptr;
void* ActiveContext = nullptr;
std::unordered_map<nos::Name, void*> IDContextMap;
}
