#include <Nodos/PluginHelpers.hpp>
#include <nosVulkanSubsystem/Helpers.hpp>
#include <nosVulkanSubsystem/Types_generated.h>
#include "InteropCommon.h"
#include "InteropNames.h"
#include "LinearToSRGB.frag.spv.dat"
std::pair<nos::Name, std::vector<uint8_t>> LinearToSRGBShader;

struct LinearToSRGB : nos::NodeContext
{
	BufferPin BufferPinProxy = {};
	nosResourceShareInfo Input = {}, Output = {};
	nosUUID NodeUUID = {}, InputUUID = {}, OutputUUID = {};
	LinearToSRGB(nosFbNode const* node) : NodeContext(node)
	{
		NodeUUID = *node->id();

		for (const auto& pin : *node->pins()) {
			const char* currentPinName = pin->name()->c_str();
			if (NSN_Input.Compare(pin->name()->c_str()) == 0) {
				InputUUID = *pin->id();
			}
			else if (NSN_Output.Compare(pin->name()->c_str()) == 0) {
				OutputUUID = *pin->id();
			}
		}
	}
	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override 
	{
		if (pinId == InputUUID) {
			nosResourceShareInfo in = nos::vkss::DeserializeTextureInfo(value.Data);
			PrepareResources(in);
		}
	}

	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{
		std::vector<nosShaderBinding> inputs;
		inputs.emplace_back(nos::vkss::ShaderBinding(NSN_Input, Input));

		nosRunPassParams pass = {
			.Key = NSN_LinearToSRGB_Pass,
			.Bindings = inputs.data(),
			.BindingCount = (uint32_t)inputs.size(),
			.Output = Output,
			.Wireframe = false,
		};

		nosCmd cmdRunPass;
		nosVulkan->Begin("Linear to SRGB Pass", &cmdRunPass);
		nosGPUEvent eventHandle = {};
		nosCmdEndParams endParams = { .ForceSubmit = true, .OutGPUEventHandle = &eventHandle };
		nosVulkan->RunPass(cmdRunPass, &pass);
		nosVulkan->End(cmdRunPass, &endParams);
		nosVulkan->WaitGpuEvent(&eventHandle, UINT64_MAX);

		return NOS_RESULT_SUCCESS;
	}

	void PrepareResources(nosResourceShareInfo& in) {
		uint64_t currentSize = in.Memory.Size;
			//static_cast<uint64_t>(GetComponentBytesFromVulkanFormat(in.Info.Texture.Format)) *
			//static_cast<uint64_t>(GetComponentNumFromVulkanFormat(in.Info.Texture.Format)) * in.Info.Texture.Width * in.Info.Texture.Height;
		if (in.Info.Texture.Width == Input.Info.Texture.Width && in.Info.Texture.Height == Input.Info.Texture.Height && in.Info.Texture.Format == Input.Info.Texture.Format) {
			Input = in;
			return;
		}

		if (Output.Memory.Handle != NULL) {
			nosVulkan->DestroyResource(&Output);
		}
		Output = in;
		Output.Memory = nosMemoryInfo{};
		nosVulkan->CreateResource(&Output);

		auto TTexture = nos::vkss::ConvertTextureInfo(Output);
		flatbuffers::FlatBufferBuilder fbb;
		auto TextureTable = nos::sys::vulkan::Texture::Pack(fbb, &TTexture);
		fbb.Finish(TextureTable);

		nosEngine.SetPinValueDirect(OutputUUID, { .Data = fbb.GetBufferPointer(), .Size = fbb.GetSize() });
		
		Input = in;

		return;
	}

};

nosResult RegisterLinearToSRGB(nosNodeFunctions* fn)
{
	NOS_BIND_NODE_CLASS(NSN_LinearToSRGB, LinearToSRGB, fn);

	LinearToSRGBShader = { NSN_FloatToIntFormat, {std::begin(LinearToSRGB_frag_spv), std::end(LinearToSRGB_frag_spv)} };

	std::filesystem::path path = nosEngine.Context->RootFolderPath;
	path = path / ".." / "Source" / "LinearToSRGB.frag";
	auto pathStr = std::filesystem::canonical(path).string();
	nosShaderInfo LinearToSRGBShaderInfo = {
		.Key = NSN_LinearToSRGB_Shader,
		.Source = {.SpirvBlob = {.Data = LinearToSRGBShader.second.data(), .Size = LinearToSRGBShader.second.size()}} // {.Stage = NOS_SHADER_STAGE_COMP, .GLSLSource = pathStr.c_str()},
	};
	nosResult ret = nosVulkan->RegisterShaders(1, &LinearToSRGBShaderInfo);
	if (NOS_RESULT_SUCCESS != ret)
		return ret;

	nosPassInfo pass = { .Key = NSN_LinearToSRGB_Pass, .Shader = NSN_LinearToSRGB_Shader, .MultiSample = 1 };
	return nosVulkan->RegisterPasses(1, &pass);
}

