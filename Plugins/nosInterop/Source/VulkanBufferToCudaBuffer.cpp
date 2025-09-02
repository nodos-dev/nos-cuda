#include <Nodos/Plugin.hpp>
#include <nosVulkanSubsystem/Helpers.hpp>
#include <nosVulkanSubsystem/Types_generated.h>
#include <nosCUDASubsystem/nosCUDASubsystem.h>
#include <nosCUDASubsystem/Types_generated.h>
#include "InteropCommon.h"
#include "InteropNames.h"

struct VulkanBufferToCUDABuffer : nos::NodeContext
{
	BufferPin VulkanBufferPinProxy = {};
	nosResourceShareInfo Buffer = {};
	nos::uuid InputBufferUUID = {}, OutputBufferUUID = {};
	nosCUDABufferInfo CUDABuffer = {};
	nosResult OnCreate(nosFbNodePtr node) override
	{
		for (const auto& pin : *node->pins()) {
			if (NSN_InputBuffer.Compare(pin->name()->c_str()) == 0) {
				InputBufferUUID = *pin->id();
			}
			else if (NSN_OutputBuffer.Compare(pin->name()->c_str()) == 0) {
				OutputBufferUUID = *pin->id();
			}
		}
		return NOS_RESULT_SUCCESS;
	}

	void OnPinValueChanged(nos::Name pinName, const nos::uuid& pinId, nosBuffer value) override
	{
		if (InputBufferUUID == pinId) {

			auto VulkanBuf = (nos::sys::vulkan::Buffer*)(value.Data);

			if (VulkanBuf->external_memory().handle() == CUDABuffer.CreateInfo.ImportedExternalHandle) {
				//Resource already imported
				return;
			}
			nosResult res = nosCUDA->ImportExternalMemoryAsCUDABuffer(VulkanBuf->external_memory().handle(), VulkanBuf->external_memory().allocation_size(),
				VulkanBuf->size_in_bytes(), 
				VulkanBuf->offset(), EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32, &CUDABuffer);
			if (res != NOS_RESULT_SUCCESS) {
				nosEngine.LogE("Import from Vulkan to CUDA failed!");
				return;
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

