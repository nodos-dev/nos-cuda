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
	nos::uuid StreamPinUUID = {};
	nosCUDAStream Stream = {};
	CreateStream(nos::fb::Node const* node) :NodeContext(node) {
		for (const auto& pin : *node->pins()) {
			if (NSN_Stream.Compare(pin->name()->c_str()) == 0) {
				StreamPinUUID = *pin->id();
			}
		}
		nosCUDA->CreateStream(&Stream);
		nosCUDAError err = nosCUDA->QueryStream(Stream);
		SetStreamPin(StreamPinUUID, NodeId, "Stream", reinterpret_cast<uint64_t>(Stream));
	}

	~CreateStream() {
		nosCUDA->DestroyStream(Stream);
	}

	inline void SetStreamPin(nos::uuid const& GenUUID,
							 nos::uuid const& NodeUUID,
							 std::string name,
							 uint64_t streamHandle)
	{
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
	NOS_BIND_NODE_CLASS(NSN_CreateStream, CreateStream, outFunctions);
}

