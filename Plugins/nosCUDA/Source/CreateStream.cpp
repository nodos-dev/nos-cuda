#include <Nodos/Plugin.hpp>
#include <nosSysCuda/nosCUDASubsystem.h>

namespace nos::cuda
{
NOS_REGISTER_NAME(CreateStream);
NOS_REGISTER_NAME(Stream);

struct CreateStream : nos::NodeContext
{
	nos::ObjectRef Stream = {};

	nosResult OnCreate(nosFbNodePtr node) override
	{
		Stream = {};
		nosCuda->CreateStream(&Stream.GetStorage());
		SetPinObject(NOS_NAME("Stream"), Stream);
		return NOS_RESULT_SUCCESS;
	}
};

void RegisterCreateStream(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NSN_CreateStream, CreateStream, outFunctions);
}
}

