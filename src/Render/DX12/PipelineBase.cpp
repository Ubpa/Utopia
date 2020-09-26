#include <DustEngine/Render/DX12/PipelineBase.h>

#include <DustEngine/Render/Material.h>
#include <DustEngine/Render/Shader.h>

#include <DustEngine/Render/DX12/RsrcMngrDX12.h>

using namespace Ubpa::DustEngine;

void PipelineBase::SetGraphicsRootSRV(ID3D12GraphicsCommandList* cmdList, const Material* material) {
	auto SetSRV = [&](ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		auto GetSRVRootParamIndex = [&](size_t registerIndex) {
			for (size_t i = 0; i < material->shader->rootParameters.size(); i++) {
				const auto& param = material->shader->rootParameters[i];

				if (!std::holds_alternative<RootDescriptorTable>(param))
					continue;

				const auto& table = std::get<RootDescriptorTable>(param);
				if (table.size() != 1)
					continue;

				const auto& range = table.front();
				if (range.NumDescriptors != 1)
					continue;

				if (range.BaseShaderRegister == registerIndex)
					return i;
			}
			assert(false);
			return static_cast<size_t>(-1);
		};

		for (UINT i = 0; i < shaderDesc.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			ThrowIfFailed(refl->GetResourceBindingDesc(i, &rsrcDesc));
			if (rsrcDesc.Type != D3D_SIT_TEXTURE)
				continue;

			auto target = material->properties.find(rsrcDesc.Name);
			if (target == material->properties.end())
				continue;

			auto dim = rsrcDesc.Dimension;
			switch (dim)
			{
			case D3D_SRV_DIMENSION_TEXTURE2D: {
				assert(std::holds_alternative<const Texture2D*>(target->second));
				auto tex2d = std::get<const Texture2D*>(target->second);
				cmdList->SetGraphicsRootDescriptorTable(
					GetSRVRootParamIndex(rsrcDesc.BindPoint),
					RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(tex2d)
				);
				break;
			}
			case D3D_SRV_DIMENSION_TEXTURECUBE: {
				assert(std::holds_alternative<const TextureCube*>(target->second));
				auto texcube = std::get<const TextureCube*>(target->second);
				cmdList->SetGraphicsRootDescriptorTable(
					GetSRVRootParamIndex(rsrcDesc.BindPoint),
					RsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(texcube)
				);
				break;
			}
			default:
				assert("not support" && false);
				break;
			}
		}
	};

	for (size_t i = 0; i < material->shader->passes.size(); i++) {
		SetSRV(RsrcMngrDX12::Instance().GetShaderRefl_vs(material->shader, i));
		SetSRV(RsrcMngrDX12::Instance().GetShaderRefl_ps(material->shader, i));
	}
}
