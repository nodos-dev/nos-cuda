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
	nos::uuid OutputUUID = {};
	nosResult OnCreate(nosFbNodePtr node) override
	{
		for (const auto& pin : *node->pins()) {
			const char* currentPinName = pin->name()->c_str();
			if (NSN_Output.Compare(pin->name()->c_str()) == 0) {
				OutputUUID = *pin->id();
			}
		}
		return NOS_RESULT_SUCCESS;
	}
	void OnPinValueChanged(nos::Name pinName, const nos::uuid& pinId, nosBuffer value) override 
	{
		if (pinName == NSN_Input) {
			nosResourceShareInfo in = nos::vkss::DeserializeTextureInfo(value.Data);
			PrepareResources(in);
		}
	}

	nosResult ExecuteNode(nosNodeExecuteParams* params) override
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

		nosCmd cmdRunPass = nos::vkss::BeginCmd(NOS_NAME("Linear to SRGB Pass"), NodeId);
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
		nosVulkan->CreateResource(&Output, "LinearToSRGB Output");

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

	nosShaderInfo LinearToSRGBShaderInfo = {
		.ShaderName = NSN_LinearToSRGB_Shader,
		.Source = {.SpirvBlob = {.Data = LinearToSRGBShader.second.data(), .Size = LinearToSRGBShader.second.size()}}, // {.Stage = NOS_SHADER_STAGE_COMP, .GLSLSource = pathStr.c_str()},
		.AssociatedNodeClassName = NSN_LinearToSRGB,
	};
	nosResult ret = nosVulkan->RegisterShaders(1, &LinearToSRGBShaderInfo);
	if (NOS_RESULT_SUCCESS != ret)
		return ret;

	nosPassInfo pass = { .Key = NSN_LinearToSRGB_Pass, .Shader = NSN_LinearToSRGB_Shader, .MultiSample = 1 };
	return nosVulkan->RegisterPasses(1, &pass);
}

