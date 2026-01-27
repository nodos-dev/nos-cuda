#include <Nodos/Plugin.hpp>
#include <nosSysVulkan/Helpers.hpp>
#include <nosSysVulkan/Types_generated.h>
#include <nosSysCuda/nosCUDASubsystem.h>
#include <nosSysCuda/Types_generated.h>
#include "InteropCommon.h"
#include "InteropNames.h"

struct VulkanBufferToCUDABuffer : nos::NodeContext
{
	BufferPin VulkanBufferPinProxy = {};
	nosCudaBufferInfo CUDABuffer = {};

	void OnPinObjectChanged(nos::Name pinName, nos::uuid const& pinId, nosObjectId newHandle) override
	{
		if (pinName == NSN_Input)
		{
			nosResourceInfo resourceInfo = {};
			nosExternalMemoryInfo extMemInfo = {};
			if (NOS_RESULT_SUCCESS == nosVulkan->GetResourceInfo(newHandle, &resourceInfo, &extMemInfo))
			{
				if (extMemInfo.Handle == CUDABuffer.CreateInfo.ImportedExternalHandle)
				{
					// Resource is already imported
					return;
				}
				nosResult res = nosCuda->ImportExternalMemoryAsCudaBuffer(extMemInfo.Handle, extMemInfo.AllocationSize, 
					resourceInfo.Buffer.Size, extMemInfo.Offset, NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32, &CUDABuffer);
				if (res != NOS_RESULT_SUCCESS)
				{
					nosEngine.LogE("Import from Vulkan to CUDA failed!");
				}
			}
		}
	}

	nosResult ExecuteNode(nosNodeExecuteParams* params) override
	{
		auto pinIds = nos::GetPinIds(params);
		auto pinValues = nos::GetPinValues(params);
		auto VulkanBuf = (nos::sys::vulkan::Buffer*)(pinValues[NSN_InputBuffer]);

		UpdateOutputPin(VulkanBuf);

		return NOS_RESULT_SUCCESS;
	}

	void UpdateOutputPin(const nos::sys::vulkan::Buffer* VulkanBuf) {

		if (VulkanBufferPinProxy.Address == VulkanBuf->handle()) {
			return;
		}
		VulkanBufferPinProxy.Address = VulkanBuf->handle();

		nos::sys::cuda::Buffer buffer;
		buffer.mutate_element_type((nos::sys::cuda::BufferElementType)VulkanBuf->element_type());
		buffer.mutate_handle(CUDABuffer.Address);
		buffer.mutate_size_in_bytes(VulkanBuf->size_in_bytes());
		buffer.mutate_offset(VulkanBuf->offset());
		buffer.mutable_external_memory().mutate_allocation_size(CUDABuffer.CreateInfo.AllocationSize);
		buffer.mutable_external_memory().mutate_handle(CUDABuffer.ShareInfo.ShareableHandle);
		//buffer.mutable_external_memory().mutate_handle_type(CUDABuffer.ShareInfo.HandleType);
		buffer.mutable_external_memory().mutate_pid(getpid());
		

		auto bufPin = nos::Buffer::From(buffer);
		nosEngine.SetPinValueDirect(OutputBufferUUID, bufPin);
		return;
	}

};

nosResult RegisterVulkanBufferToCUDABuffer(nosNodeFunctions* fn)
{
	NOS_BIND_NODE_CLASS(NSN_VulkanBufferToCUDABuffer, VulkanBufferToCUDABuffer, fn);
	return NOS_RESULT_SUCCESS;
}

