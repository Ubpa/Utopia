#include <Utopia/Render/DX12/PipelineBase.h>

#include <Utopia/Render/Material.h>
#include <Utopia/Render/Shader.h>

#include <Utopia/Render/DX12/RsrcMngrDX12.h>
#include <Utopia/Render/DX12/ShaderCBMngrDX12.h>

using namespace Ubpa::Utopia;

PipelineBase::ShaderCBDesc PipelineBase::UpdateShaderCBs(
	ShaderCBMngrDX12& shaderCBMngr,
	const Shader& shader,
	const std::unordered_set<const Material*>& materials,
	const std::set<std::string_view>& commonCBs
) {
	PipelineBase::ShaderCBDesc rst;

	auto CalculateSize = [&](ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
			auto cb = refl->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			ThrowIfFailed(cb->GetDesc(&cbDesc));

			if(commonCBs.find(cbDesc.Name) != commonCBs.end())
				continue;

			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			refl->GetResourceBindingDescByName(cbDesc.Name, &rsrcDesc);

			auto target = rst.offsetMap.find(rsrcDesc.BindPoint);
			if(target != rst.offsetMap.end())
				continue;

			rst.offsetMap.emplace_hint(target, std::pair{ rsrcDesc.BindPoint, rst.materialCBSize });
			rst.materialCBSize += UDX12::Util::CalcConstantBufferByteSize(cbDesc.Size);
		}
	};

	for (size_t i = 0; i < shader.passes.size(); i++) {
		CalculateSize(RsrcMngrDX12::Instance().GetShaderRefl_vs(shader, i));
		CalculateSize(RsrcMngrDX12::Instance().GetShaderRefl_ps(shader, i));
	}

	auto buffer = shaderCBMngr.GetBuffer(shader);
	buffer->FastReserve(rst.materialCBSize * materials.size());
	for (auto material : materials) {
		size_t idx = rst.indexMap.size();
		rst.indexMap[material->GetInstanceID()] = idx;
	}

	auto UpdateShaderCBsForRefl = [&](std::set<size_t>& flags, const Material* material, ID3D12ShaderReflection* refl) {
		size_t index = rst.indexMap.at(material->GetInstanceID());

		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
			auto cb = refl->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			ThrowIfFailed(cb->GetDesc(&cbDesc));

			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			refl->GetResourceBindingDescByName(cbDesc.Name, &rsrcDesc);

			if (rst.offsetMap.find(rsrcDesc.BindPoint) == rst.offsetMap.end())
				continue;

			if(flags.find(rsrcDesc.BindPoint) != flags.end())
				continue;
			
			flags.insert(rsrcDesc.BindPoint);

			size_t offset = rst.materialCBSize * index + rst.offsetMap.at(rsrcDesc.BindPoint);

			for (UINT j = 0; j < cbDesc.Variables; j++) {
				auto var = cb->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC varDesc;
				ThrowIfFailed(var->GetDesc(&varDesc));

				auto target = material->properties.find(varDesc.Name);
				if(target == material->properties.end())
					continue;

				std::visit([&](const auto& value) {
					using Value = std::decay_t<decltype(value)>;
					if constexpr (std::is_same_v<Value, bool>) {
						auto v = static_cast<unsigned int>(value);
						assert(varDesc.Size == sizeof(unsigned int));
						buffer->Set(offset + varDesc.StartOffset, &v, sizeof(unsigned int));
					}
					else if constexpr (std::is_same_v<Value, const Texture2D*> || std::is_same_v<Value, const TextureCube*>)
						assert(false);
					else {
						assert(varDesc.Size == sizeof(Value));
						buffer->Set(offset + varDesc.StartOffset, &value, varDesc.Size);
					}
				}, target->second);
			}
		}
	};

	for (auto material : materials) {
		std::set<size_t> flags;
		for (size_t i = 0; i < shader.passes.size(); i++) {
			UpdateShaderCBsForRefl(flags, material, RsrcMngrDX12::Instance().GetShaderRefl_vs(shader, i));
			UpdateShaderCBsForRefl(flags, material, RsrcMngrDX12::Instance().GetShaderRefl_ps(shader, i));
		}
	}

	return rst;
}

void PipelineBase::SetGraphicsRoot_CBV_SRV(
	ID3D12GraphicsCommandList* cmdList,
	ShaderCBMngrDX12& shaderCBMngr,
	const ShaderCBDesc& shaderCBDescconst,
	const Material& material,
	const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
	const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
) {
	auto buffer = shaderCBMngr.GetBuffer(*material.shader);
	size_t cbPos = buffer->GetResource()->GetGPUVirtualAddress()
		+ shaderCBDescconst.indexMap.at(material.GetInstanceID()) * shaderCBDescconst.materialCBSize;

	auto SetGraphicsRoot_Refl = [&](ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		auto GetSRVRootParamIndex = [&](UINT registerIndex) {
			for (size_t i = 0; i < material.shader->rootParameters.size(); i++) {
				const auto& param = material.shader->rootParameters[i];

				bool flag = std::visit(
					[=](const auto& param) {
						using Type = std::decay_t<decltype(param)>;
						if constexpr (std::is_same_v<Type, RootDescriptorTable>) {
							const RootDescriptorTable& table = param;
							if (table.size() != 1)
								return false;

							const auto& range = table.front();
							assert(range.NumDescriptors > 0);

							return range.BaseShaderRegister == registerIndex;
						}
						else
							return false;
					},
					param
				);

				if (flag)
					return (UINT)i;
			}
			// assert(false);
			return static_cast<UINT>(-1); // inner SRV
		};

		auto GetCBVRootParamIndex = [&](UINT registerIndex) {
			for (size_t i = 0; i < material.shader->rootParameters.size(); i++) {
				const auto& param = material.shader->rootParameters[i];

				bool flag = std::visit(
					[=](const auto& param) {
						using Type = std::decay_t<decltype(param)>;
						if constexpr (std::is_same_v<Type, RootDescriptor>) {
							const RootDescriptor& descriptor = param;
							if (descriptor.DescriptorType != RootDescriptorType::CBV)
								return false;

							return descriptor.ShaderRegister == registerIndex;
						}
						else
							return false;
					},
					param
				);
				
				if (flag)
					return (UINT)i;
			}
			assert(false);
			return static_cast<UINT>(-1);
		};

		for (UINT i = 0; i < shaderDesc.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			ThrowIfFailed(refl->GetResourceBindingDesc(i, &rsrcDesc));

			switch (rsrcDesc.Type)
			{
			case D3D_SIT_CBUFFER:
			{
				UINT idx = GetCBVRootParamIndex(rsrcDesc.BindPoint);
				D3D12_GPU_VIRTUAL_ADDRESS adress;

				if (auto target = shaderCBDescconst.offsetMap.find(rsrcDesc.BindPoint); target != shaderCBDescconst.offsetMap.end())
					adress = cbPos + target->second;
				else if (auto target = commonCBs.find(rsrcDesc.Name); target != commonCBs.end())
					adress = target->second;
				else {
					assert(false);
					break;
				}
				cmdList->SetGraphicsRootConstantBufferView(idx, adress);
				break;
			}
			case D3D_SIT_TEXTURE:
			{
				UINT rootParamIndex = GetSRVRootParamIndex(rsrcDesc.BindPoint);
				if (rootParamIndex == static_cast<size_t>(-1))
					break;

				D3D12_GPU_DESCRIPTOR_HANDLE handle;
				handle.ptr = 0;

				if (auto target = material.properties.find(rsrcDesc.Name); target != material.properties.end()) {
					auto dim = rsrcDesc.Dimension;
					switch (dim)
					{
					case D3D_SRV_DIMENSION_TEXTURE2D: {
						assert(std::holds_alternative<std::shared_ptr<const Texture2D>>(target->second));
						auto tex2d = std::get<std::shared_ptr<const Texture2D>>(target->second);
						handle = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*tex2d);
						break;
					}
					case D3D_SRV_DIMENSION_TEXTURECUBE: {
						assert(std::holds_alternative<std::shared_ptr<const TextureCube>>(target->second));
						auto texcube = std::get<std::shared_ptr<const TextureCube>>(target->second);
						handle = RsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*texcube);
						break;
					}
					default:
						assert("not support" && false);
						break;
					}
				}
				else if (auto target = commonSRVs.find(rsrcDesc.Name); target != commonSRVs.end())
					handle = target->second;
				else
					break;
				
				if (handle.ptr != 0)
					cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, handle);

				break;
			}
			default:
				break;
			}
		}
	};

	for (size_t i = 0; i < material.shader->passes.size(); i++) {
		SetGraphicsRoot_Refl(RsrcMngrDX12::Instance().GetShaderRefl_vs(*material.shader, i));
		SetGraphicsRoot_Refl(RsrcMngrDX12::Instance().GetShaderRefl_ps(*material.shader, i));
	}
}

void PipelineBase::SetPSODescForRenderState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const RenderState& renderState) {
	desc.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(renderState.fillMode);
	desc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(renderState.cullMode);
	desc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(renderState.zTest);
	desc.DepthStencilState.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(renderState.zWrite);

	for (size_t i = 0; i < 8; i++) {
		if (renderState.blendStates[i].enable) {
			desc.BlendState.RenderTarget[i].BlendEnable = TRUE;
			desc.BlendState.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(renderState.blendStates[i].src);
			desc.BlendState.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(renderState.blendStates[i].dest);
			desc.BlendState.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(renderState.blendStates[i].op);
			desc.BlendState.RenderTarget[i].SrcBlendAlpha = static_cast<D3D12_BLEND>(renderState.blendStates[i].srcAlpha);
			desc.BlendState.RenderTarget[i].DestBlendAlpha = static_cast<D3D12_BLEND>(renderState.blendStates[i].destAlpha);
			desc.BlendState.RenderTarget[i].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(renderState.blendStates[i].opAlpha);
		}
	}
	
	if (renderState.stencilState.enable) {
		desc.DepthStencilState.StencilEnable = TRUE;
		desc.DepthStencilState.StencilReadMask = renderState.stencilState.readMask;
		desc.DepthStencilState.StencilWriteMask = renderState.stencilState.writeMask;
		D3D12_DEPTH_STENCILOP_DESC stencilDesc;
		stencilDesc.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(renderState.stencilState.depthFailOp);
		stencilDesc.StencilFailOp = static_cast<D3D12_STENCIL_OP>(renderState.stencilState.failOp);
		stencilDesc.StencilPassOp = static_cast<D3D12_STENCIL_OP>(renderState.stencilState.passOp);
		stencilDesc.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(renderState.stencilState.func);
		desc.DepthStencilState.FrontFace = stencilDesc;
		desc.DepthStencilState.BackFace = stencilDesc;
	}
	for (size_t i = 0; i < 8; i++)
		desc.BlendState.RenderTarget[i].RenderTargetWriteMask = renderState.colorMask[i];
}
