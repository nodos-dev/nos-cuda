#include <Nodos/PluginHelpers.hpp>
#include <nosVulkanSubsystem/Helpers.hpp>
#include <nosVulkanSubsystem/Types_generated.h>
#include <nosCUDASubsystem/nosCUDASubsystem.h>
#include <nosCUDASubsystem/Types_generated.h>
#include "InteropCommon.h"
#include "InteropNames.h"
#include "FloatToInt.comp.spv.dat"

std::pair<nos::Name, std::vector<uint8_t>> FloatToIntFormatShader;

struct TextureFormatConverter : nos::NodeContext
{

	BufferPin VulkanBufferPinProxy = {};
	nosResourceShareInfo InputTexture = {}, OutputTexture = {};
	nosUUID NodeUUID = {}, InputUUID = {}, OutputUUID = {}, FormatUUID = {};
	nos::sys::vulkan::Format OutputFormat = {};
	nosResourceShareInfo outBuf = {};
	nosResourceShareInfo inBuf = {};
	TextureFormatConverter(nosFbNode const* node) : NodeContext(node)
	{
		NodeUUID = *node->id();

		for (const auto& pin : *node->pins()) {
			if (NSN_Input.Compare(pin->name()->c_str()) == 0) {
				InputUUID = *pin->id();
			}
			else if (NSN_Output.Compare(pin->name()->c_str()) == 0) {
				OutputUUID = *pin->id();
			}
		}
		std::vector<std::string> Formats = {};
		auto FormatEnums = nos::sys::vulkan::EnumValuesFormat();
		for (int i = 0; i < 69; i++) {
			Formats.push_back(nos::sys::vulkan::EnumNameFormat(FormatEnums[i]));
		}
		CreateStringList(FormatUUID, NodeUUID, "OutputFormat", std::move(Formats));
	}

	~TextureFormatConverter() {
		nosVulkan->DestroyResource(&OutputTexture);
	}

	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override
	{
		if (InputUUID == pinId) {
			InputTexture = nos::vkss::DeserializeTextureInfo(value.Data);
			PrepareResources();
		}
		if (FormatUUID == pinId) {
			const char* SelectedFormat = (const char*)value.Data;
			auto FormatEnums = nos::sys::vulkan::EnumValuesFormat();
			nos::sys::vulkan::Format newFormat = {};
			for (int i = 0; i < 69; i++) {
				if (strcmp(nos::sys::vulkan::EnumNameFormat(FormatEnums[i]), SelectedFormat) == 0) {
					newFormat = FormatEnums[i];
					if (newFormat != OutputFormat) {
						OutputFormat = newFormat;
						PrepareResources();
					}
					break;
				}
			}
		}
	}

	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{

		auto pinIds = nos::GetPinIds(args);
		auto pinValues = nos::GetPinValues(args);
		InputTexture = nos::vkss::DeserializeTextureInfo(pinValues[NSN_Input]);
		auto Out = nos::vkss::DeserializeTextureInfo(pinValues[NSN_Output]);
		//TODO: Also should be able to convert from INT texture to FLOAT textures
		//TODO: Also should be able to convert between signed integer and unsigned integers
		//TODO: Editor view and AJA does not expects INTEGER formats hence both the editor and ajaOut view does not show correct image.
		if (!IsBlitCompatible(InputTexture.Info.Texture.Format, Out.Info.Texture.Format)) {
			struct OutputType { int outputType; };
			OutputType out = {};
			std::vector<nosShaderBinding> inputs;
			switch (Out.Info.Texture.Format) 
			{
				case NOS_FORMAT_R32G32B32A32_UINT:
				{
					out.outputType = 0;
					break;
				}
				case NOS_FORMAT_R16G16B16A16_UINT:
				{
					out.outputType = 1;
					break;
				}
				case NOS_FORMAT_R8G8B8A8_UINT:
				{
					out.outputType = 2;
					break;
				}
				case NOS_FORMAT_R32G32B32A32_SINT:
				{
					out.outputType = 3;
					break;
				}
				case NOS_FORMAT_R16G16B16A16_SINT:
				{
					out.outputType = 4;
					break;
				}
				default: 
				{
					out.outputType = -1;
					break;
				}
			}
			if (out.outputType != -1) {
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_InputTexture, InputTexture));
				inputs.emplace_back(nos::vkss::ShaderBinding<OutputType>(NSN_outputType, out));

				//Validation layer does not like this but we sure that only true desired texture will be used in shader
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_DST_TEXTURE_UINT32, Out));
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_DST_TEXTURE_UINT16, Out));
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_DST_TEXTURE_UINT8, Out));
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_DST_TEXTURE_INT32, Out));
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_DST_TEXTURE_INT16, Out));
				inputs.emplace_back(nos::vkss::ShaderBinding(NSN_DST_TEXTURE_INT8, Out));

				nosCmd cmdRunPass;
				nosVulkan->Begin("Texture Format Conversion:Float to Int", &cmdRunPass);
				nosRunComputePassParams pass = {};
				nosGPUEvent eventHandle = {};
				nosCmdEndParams endParams = { .ForceSubmit = true, .OutGPUEventHandle = &eventHandle };
				pass.Key = NSN_FloatToIntFormat_Pass;
				pass.DispatchSize = nosVec2u(InputTexture.Info.Texture.Width / 16, InputTexture.Info.Texture.Height / 16);
				pass.Bindings = inputs.data();
				pass.BindingCount = inputs.size();
				pass.Benchmark = 0;
				nosVulkan->RunComputePass(cmdRunPass, &pass);
				nosVulkan->End(cmdRunPass, &endParams);
				nosVulkan->WaitGpuEvent(&eventHandle, UINT64_MAX);
			}
		}
		else {
			nosCmd cmd = {};
			nosGPUEvent waitEvent = {};
			nosCmdEndParams endParams = { .ForceSubmit = true, .OutGPUEventHandle = &waitEvent };
			nosVulkan->Begin("TexToTex", &cmd);
			nosVulkan->Copy(cmd, &InputTexture, &Out, nullptr);
			nosVulkan->End(cmd, &endParams);
			nosVulkan->WaitGpuEvent(&waitEvent, UINT64_MAX);
		}



		outBuf.Info.Type = NOS_RESOURCE_TYPE_BUFFER;
		outBuf.Info.Buffer.Size = Out.Memory.Size;
		outBuf.Info.Buffer.Usage = nosBufferUsage(NOS_BUFFER_USAGE_TRANSFER_SRC | NOS_BUFFER_USAGE_TRANSFER_DST);

		inBuf.Info.Type = NOS_RESOURCE_TYPE_BUFFER;
		inBuf.Info.Buffer.Size = Out.Memory.Size;
		inBuf.Info.Buffer.Usage = nosBufferUsage(NOS_BUFFER_USAGE_TRANSFER_SRC | NOS_BUFFER_USAGE_TRANSFER_DST);
		//if(outBuf.Memory.Handle == NULL)
		//	nosVulkan->CreateResource(&outBuf);
		//if (inBuf.Memory.Handle == NULL)
		//	nosVulkan->CreateResource(&inBuf);

		//nosCmd cmd2 = {};
		//nosGPUEvent waitEvent2 = {};
		//nosCmdEndParams endParams2 = { .ForceSubmit = true, .OutGPUEventHandle = &waitEvent2 };
		//nosVulkan->Begin("TexToTex", &cmd2);
		//nosVulkan->Copy(cmd2, &InputTexture, &inBuf, nullptr);
		//nosVulkan->End(cmd2, &endParams2);
		//nosVulkan->WaitGpuEvent(&waitEvent2, UINT64_MAX);


		//uint8_t* cpuInput = nosVulkan->Map(&inBuf);

		//nosCmd cmd3 = {};
		//nosGPUEvent waitEvent3 = {};
		//nosCmdEndParams endParams3 = { .ForceSubmit = true, .OutGPUEventHandle = &waitEvent3 };
		//nosVulkan->Begin("TexToTex", &cmd3);
		//nosVulkan->Copy(cmd3, &Out, &outBuf, nullptr);
		//nosVulkan->End(cmd3, &endParams3);
		//nosVulkan->WaitGpuEvent(&waitEvent3, UINT64_MAX);
		//
		//uint8_t* cpuOut = nosVulkan->Map(&outBuf);
		
		//nosResourceShareInfo out = nos::vkss::DeserializeTextureInfo(pinValues[NSN_Output]);
		//nosCmd cmd2;
		//nosGPUEvent gpuevent2 = {};
		//nosCmdEndParams endParams2 = { .ForceSubmit = true, .OutGPUEventHandle = &gpuevent2 };
		//nosVulkan->Begin("NVVFX Upload", &cmd2);
		//nosVulkan->Copy(cmd2, &OutputTexture, &out, 0);
		//nosVulkan->End(cmd2, &endParams2);
		//nosVulkan->WaitGpuEvent(&gpuevent2, UINT64_MAX);

		return NOS_RESULT_SUCCESS;
	}

	void PrepareResources() {
		if(OutputTexture.Info.Texture.Width == InputTexture.Info.Texture.Width && OutputTexture.Info.Texture.Height == InputTexture.Info.Texture.Height
			&& OutputTexture.Info.Texture.Format == nosFormat((int)OutputFormat) ) {
			//No change
			return;
		}
		if (OutputTexture.Memory.Handle != NULL) {
			nosVulkan->DestroyResource(&OutputTexture);
		}

		OutputTexture.Info.Type = NOS_RESOURCE_TYPE_TEXTURE;
		OutputTexture.Info.Texture.FieldType = InputTexture.Info.Texture.FieldType;
		OutputTexture.Info.Texture.Filter = InputTexture.Info.Texture.Filter;
		OutputTexture.Info.Texture.Format = nosFormat((int)OutputFormat);
		OutputTexture.Info.Texture.Height = InputTexture.Info.Texture.Height;
		OutputTexture.Info.Texture.Usage = InputTexture.Info.Texture.Usage;
		OutputTexture.Info.Texture.Width = InputTexture.Info.Texture.Width;

		nosVulkan->CreateResource(&OutputTexture);
		UpdateOutputPin();
	}

	void UpdateOutputPin() {
		auto TTexture = nos::vkss::ConvertTextureInfo(OutputTexture);
		flatbuffers::FlatBufferBuilder fbb;
		auto TextureTable = nos::sys::vulkan::Texture::Pack(fbb, &TTexture);
		fbb.Finish(TextureTable);
		nosEngine.SetPinValueDirect(OutputUUID, { .Data = fbb.GetBufferPointer(), .Size = fbb.GetSize() });
	}
};

nosResult RegisterTextureFormatConverter(nosNodeFunctions* fn)
{
	NOS_BIND_NODE_CLASS(NSN_TextureFormatConverter, TextureFormatConverter, fn);

	FloatToIntFormatShader = { NSN_FloatToIntFormat, {std::begin(FloatToInt_comp_spv), std::end(FloatToInt_comp_spv)} };

	std::filesystem::path path = nosEngine.Context->RootFolderPath;
	path = path / ".." / "Source" / "FloatToInt.comp";
	auto pathStr = std::filesystem::canonical(path).string();
	nosShaderInfo FloatToIntShaderInfo = {
		.Key = NSN_FloatToIntFormat,
		.Source = {.SpirvBlob = {.Data = FloatToIntFormatShader.second.data(), .Size = FloatToIntFormatShader.second.size()}} // {.Stage = NOS_SHADER_STAGE_COMP, .GLSLSource = pathStr.c_str()},
	};
	nosResult ret = nosVulkan->RegisterShaders(1, &FloatToIntShaderInfo);
	if (NOS_RESULT_SUCCESS != ret)
		return ret;

	nosPassInfo pass = { .Key = NSN_FloatToIntFormat_Pass, .Shader = NSN_FloatToIntFormat, .MultiSample = 1 };
	return nosVulkan->RegisterPasses(1, &pass);
}

