#include <Nodos/Plugin.hpp>
#include <nosSysVulkan/Helpers.hpp>
#include <nosSysVulkan/Types_generated.h>
#include <nosSysCuda/Helpers.hpp>
#include <nosSysCuda/Types_generated.h>
#include "InteropCommon.h"
#include "InteropNames.h"

namespace nos::cuda::interop
{
static std::optional<nosCudaExternalMemoryHandleType> MapExternalHandleType(nosExternalMemoryHandleType handleType)
{
	switch (handleType)
	{
	case NOS_EXTERNAL_MEMORY_HANDLE_TYPE_WIN32:
		return NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32;
	case NOS_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE:
		return NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12RESOURCE;
	case NOS_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE:
		return NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11RESOURCE;
	default:
		return std::nullopt;
	}
}

struct VulkanBufferToCUDABuffer : nos::NodeContext
{
	nos::ObjectRef OutputBuffer{};
	uint64_t LastExternalHandle = 0;

	void OnPinObjectChanged(nos::Name pinName, nos::uuid const& pinId, nosObjectId newHandle) override
	{
		if (pinName != NSN_InputBuffer)
			return;

		auto resourceInfo = nos::sys::vulkan::GetResourceInfo(newHandle);
		auto extMemInfo = nos::sys::vulkan::GetExternalMemoryInfo(newHandle);
		if (!resourceInfo || !extMemInfo)
			return;
		if (resourceInfo->Type != NOS_RESOURCE_TYPE_BUFFER)
			return;

		if (extMemInfo->Handle == LastExternalHandle && OutputBuffer)
			return;

		auto mappedHandleType = MapExternalHandleType(extMemInfo->HandleType);
		if (!mappedHandleType)
		{
			nosEngine.LogE("Unsupported external memory handle type for Vulkan->CUDA buffer import.");
			return;
		}

		nosCudaBufferImportInfo importInfo{};
		importInfo.AllocationSize = resourceInfo->Buffer.Size;
		importInfo.Offset = extMemInfo->Offset;
		importInfo.ExternalMemoryHandle = extMemInfo->Handle;
		importInfo.ExternalMemoryHandleType = *mappedHandleType;

		nos::ObjectRef newCudaBuffer{};
		nosResult res = nosCuda->ImportExternalMemoryAsCudaBuffer(&importInfo,
		                                                         extMemInfo->AllocationSize,
		                                                         &newCudaBuffer.GetStorage());
		if (res != NOS_RESULT_SUCCESS)
		{
			nosEngine.LogE("Import from Vulkan to CUDA failed.");
			return;
		}

		OutputBuffer = std::move(newCudaBuffer);
		LastExternalHandle = extMemInfo->Handle;
		SetPinObject(NSN_OutputBuffer, OutputBuffer);
	}
};

nosResult RegisterVulkanBufferToCUDABuffer(nosNodeFunctions* fn)
{
	NOS_BIND_NODE_CLASS(NSN_VulkanBufferToCUDABuffer, VulkanBufferToCUDABuffer, fn);
	return NOS_RESULT_SUCCESS;
}
} // namespace nos::cuda::interop
