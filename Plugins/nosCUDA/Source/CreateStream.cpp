#include <Nodos/PluginAPI.h>
#include <Builtins_generated.h>
#include <Nodos/PluginHelpers.hpp>
#include <AppService_generated.h> 
#include <AppEvents_generated.h>
#include <flatbuffers/flatbuffers.h>
#include "nosCUDASubsystem/Types_generated.h"
#include "nosCUDASubsystem/nosCUDASubsystem.h"

NOS_REGISTER_NAME(CreateStream);
NOS_REGISTER_NAME(Stream);

struct CreateStream : nos::NodeContext
{
	nosUUID NodeUUID = {}, StreamPinUUID = {};
	nosCUDAStream Stream = {};
	CreateStream(nos::fb::Node const* node) :NodeContext(node) {
		NodeUUID = *node->id();
		for (const auto& pin : *node->pins()) {
			if (NSN_Stream.Compare(pin->name()->c_str()) == 0) {
				StreamPinUUID = *pin->id();
			}
		}
		nosCUDA->CreateStream(&Stream);
		nosCUDAError err = nosCUDA->QueryStream(Stream);
		SetStreamPin(StreamPinUUID, NodeUUID, "Stream", reinterpret_cast<uint64_t>(Stream));
	}

	~CreateStream() {
		nosCUDA->DestroyStream(Stream);
	}

	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override {
	}

	void OnPinDisconnected(nos::Name pinName) override {
		
	}

	

	static nosBool CanConnectPin(void* ctx, nosName pinName, nosUUID connectedPinId, const nosBuffer* connectedPinData) {
		
		return NOS_TRUE;
	}

	static nosResult GetFunctions(size_t* count, nosName* names, nosPfnNodeFunctionExecute* fns)
	{
		*count = 0;
		if (!names || !fns)
			return NOS_RESULT_SUCCESS;
		return NOS_RESULT_SUCCESS;
	}
	inline void SetStreamPin(nosUUID& GenUUID, nosUUID& NodeUUID, std::string name, uint64_t streamHandle) {
		nos::sys::cuda::TCUDAStream stream;
		stream.handle = streamHandle;
		//If it is not empty
		flatbuffers::FlatBufferBuilder fbb;
		auto bufPin = nos::Buffer::From(stream);
		nosEngine.SetPinValueDirect(GenUUID, bufPin);
		return;
	}
};

void RegisterCreateStream(nosNodeFunctions* outFunctions) {
	outFunctions->CanConnectPin = CreateStream::CanConnectPin;
	NOS_BIND_NODE_CLASS(NSN_CreateStream, CreateStream, outFunctions);
}

