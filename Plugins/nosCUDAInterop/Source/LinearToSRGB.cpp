#include <Nodos/Plugin.hpp>
#include <nosSysVulkan/Helpers.hpp>
#include <nosSysVulkan/Types_generated.h>
#include "InteropCommon.h"
#include "InteropNames.h"
#include "LinearToSRGB.frag.spv.dat"
std::pair<nos::Name, std::vector<uint8_t>> LinearToSRGBShader;

struct LinearToSRGB : nos::NodeContext
{
	nos::ForeignObjectRef Output = {};
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
	void OnPinObjectChanged(nos::Name pinName, nos::uuid const& pinId, nosObjectId newHandle) override
	{
		if (pinName == NSN_Input)
		{
			if (auto info = nos::sys::vulkan::GetResourceInfo(newHandle))
			{
				PrepareResources(*info);
			}
		}
	}

	nosResult ExecuteNode(nos::NodeExecuteParams const& params) override
	{
		std::vector<nosShaderBinding> inputs;
		nos::ObjectRef in = params.GetPinObject(NSN_Input);
		inputs.emplace_back(nos::sys::vulkan::ShaderTextureBinding(NSN_Input, in, NOS_TEXTURE_FILTER_LINEAR));

		nosRunPassParams pass = {
			.Key = NSN_LinearToSRGB_Pass,
			.Bindings = inputs.data(),
			.BindingCount = (uint32_t)inputs.size(),
			.Output = Output,
			.Wireframe = false,
		};

		nosCmd cmdRunPass = nos::sys::vulkan::BeginCmd(NOS_NAME("Linear to SRGB Pass"), NodeId);
		nosGPUEvent eventHandle = {};
		nosCmdEndParams endParams = { .ForceSubmit = true, .OutGPUEventHandle = &eventHandle };
		nosVulkan->RunPass(cmdRunPass, &pass);
		nosVulkan->End(cmdRunPass, &endParams);
		nosVulkan->WaitGpuEvent(&eventHandle, UINT64_MAX);

		return NOS_RESULT_SUCCESS;
	}

	void PrepareResources(const nosResourceInfo& in)
	{
		if (Output) {
			auto outInfo = nos::sys::vulkan::GetResourceInfo(Output);
			if (outInfo && in.Texture.Width == outInfo->Texture.Width && in.Texture.Height == outInfo->Texture.Height && in.Texture.Format == outInfo->Texture.Format)
			{
				return;		
			}
		}

		Output = nos::sys::vulkan::CreateResource(in, "LinearToSRGB Output");
		if (Output)
			SetPinObject(OutputUUID, Output);
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

