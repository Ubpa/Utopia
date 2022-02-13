#include <Utopia/Render/DX12/ShaderCBMngr.h>

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

#include <Utopia/Render/RenderQueue.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;

ShaderCBMngr::ShaderCBMngr(ID3D12Device* device)
	: materialCBUploadVector(device)
	, commonCBUploadVector(device)
{
}

void ShaderCBMngr::NewFrame() {
	renderCtxDataMap.clear();
	materialCBUploadVector.Clear();
	commonCBUploadVector.Clear();
}

void ShaderCBMngr::RegisterRenderContext(const RenderContext& ctx,
	std::span<const CameraConstants* const> cameraConstantsSpan) {
	//////////////
	// CommonCB //
	//////////////

	RenderContextData renderCtxData;
	renderCtxData.cameraOffset = commonCBUploadVector.Size();
	renderCtxData.lightOffset = renderCtxData.cameraOffset
		+ cameraConstantsSpan.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants));
	renderCtxData.objectOffset = renderCtxData.lightOffset
		+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray));

	commonCBUploadVector.Resize(
		commonCBUploadVector.Size()
		+ cameraConstantsSpan.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants))
		+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray))
		+ ctx.entity2data.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants))
	);

	for (size_t i = 0; i < cameraConstantsSpan.size(); i++) {
		commonCBUploadVector.Set(
			renderCtxData.cameraOffset + i * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants)),
			cameraConstantsSpan[i],
			sizeof(CameraConstants)
		);
	}

	// light array
	commonCBUploadVector.Set(renderCtxData.lightOffset, &ctx.lightArray, sizeof(LightArray));

	// objects
	size_t offset = renderCtxData.objectOffset;
	for (auto& [idx, data] : ctx.entity2data) {
		ObjectConstants objectConstants;
		objectConstants.World = data.l2w;
		objectConstants.InvWorld = data.w2l;
		objectConstants.PrevWorld = data.prevl2w;
		renderCtxData.entity2offset.emplace(idx, offset);
		commonCBUploadVector.Set(offset, &objectConstants, sizeof(ObjectConstants));
		offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	}

	////////////////
	// MaterialCB //
	////////////////

	std::unordered_map<const Shader*, std::unordered_set<const Material*>> opaqueMaterialMap;
	for (const auto& opaque : ctx.renderQueue.GetOpaques())
		opaqueMaterialMap[opaque.material->shader.get()].insert(opaque.material.get());

	std::unordered_map<const Shader*, std::unordered_set<const Material*>> transparentMaterialMap;
	for (const auto& transparent : ctx.renderQueue.GetTransparents())
		transparentMaterialMap[transparent.material->shader.get()].insert(transparent.material.get());

	for (const auto& [shader, materials] : opaqueMaterialMap) {
		auto materialCBDesc = RegisterShaderMaterialCB(shader, materials);
		renderCtxData.materialCBDescMap.emplace(shader, std::move(materialCBDesc));
	}

	for (const auto& [shader, materials] : transparentMaterialMap) {
		auto materialCBDesc = RegisterShaderMaterialCB(shader, materials);
		renderCtxData.materialCBDescMap.emplace(shader, std::move(materialCBDesc));
	}

	/////////
	// Add //
	/////////

	renderCtxDataMap.emplace(ctx.ID, std::move(renderCtxData));
}

void ShaderCBMngr::SetGraphicsRoot_CBV_SRV(
	ID3D12GraphicsCommandList* cmdList,
	size_t ctxID,
	const Shader* shader,
	const Material& material,
	const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
	const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
)
{
	const auto& materialCBDesc = renderCtxDataMap.at(ctxID).materialCBDescMap.at(shader);

	size_t cbPos = materialCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ materialCBDesc.begin_offset + materialCBDesc.indexMap.at(material.GetInstanceID()) * materialCBDesc.materialCBSize;

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

				if (auto target = materialCBDesc.offsetMap.find(rsrcDesc.BindPoint); target != materialCBDesc.offsetMap.end())
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
						assert(std::holds_alternative<SharedVar<Texture2D>>(target->second.value));
						auto tex2d = std::get<SharedVar<Texture2D>>(target->second.value);
						handle = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*tex2d);
						break;
					}
					case D3D_SRV_DIMENSION_TEXTURECUBE: {
						assert(std::holds_alternative<SharedVar<TextureCube>>(target->second.value));
						auto texcube = std::get<SharedVar<TextureCube>>(target->second.value);
						handle = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*texcube);
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
		SetGraphicsRoot_Refl(GPURsrcMngrDX12::Instance().GetShaderRefl_vs(*material.shader, i));
		SetGraphicsRoot_Refl(GPURsrcMngrDX12::Instance().GetShaderRefl_ps(*material.shader, i));
	}
}

MaterialCBDesc ShaderCBMngr::RegisterShaderMaterialCB(
	const Shader* shader,
	const std::unordered_set<const Material*>& materials
) {
	MaterialCBDesc rst;
	rst.begin_offset = materialCBUploadVector.Size();

	auto CalculateSize = [&](ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
			auto cb = refl->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			ThrowIfFailed(cb->GetDesc(&cbDesc));

			if (PipelineCommonResourceMngr::GetInstance().IsCommonCB(cbDesc.Name))
				continue;

			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			refl->GetResourceBindingDescByName(cbDesc.Name, &rsrcDesc);

			auto target = rst.offsetMap.find(rsrcDesc.BindPoint);
			if (target != rst.offsetMap.end())
				continue;

			rst.offsetMap.emplace(std::pair{ rsrcDesc.BindPoint, rst.materialCBSize });
			rst.materialCBSize += UDX12::Util::CalcConstantBufferByteSize(cbDesc.Size);
		}
	};

	for (size_t i = 0; i < shader->passes.size(); i++) {
		CalculateSize(GPURsrcMngrDX12::Instance().GetShaderRefl_vs(*shader, i));
		CalculateSize(GPURsrcMngrDX12::Instance().GetShaderRefl_ps(*shader, i));
	}

	for (auto material : materials) {
		size_t idx = rst.indexMap.size();
		rst.indexMap[material->GetInstanceID()] = idx;
	}

	materialCBUploadVector.Resize(materialCBUploadVector.Size() + rst.materialCBSize * materials.size());

	auto UpdateShaderCBsForRefl = [&](std::set<size_t>& flags, const Material& material, ID3D12ShaderReflection* refl) {
		size_t index = rst.indexMap.at(material.GetInstanceID());

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

			if (flags.find(rsrcDesc.BindPoint) != flags.end())
				continue;

			flags.insert(rsrcDesc.BindPoint);

			size_t offset = rst.begin_offset + rst.materialCBSize * index + rst.offsetMap.at(rsrcDesc.BindPoint);

			for (UINT j = 0; j < cbDesc.Variables; j++) {
				auto var = cb->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC varDesc;
				ThrowIfFailed(var->GetDesc(&varDesc));

				auto target = material.properties.find(varDesc.Name);
				if (target == material.properties.end())
					continue;

				std::visit([&](const auto& value) {
					using Value = std::decay_t<decltype(value)>;
					if constexpr (std::is_same_v<Value, bool>) {
						auto v = static_cast<unsigned int>(value);
						assert(varDesc.Size == sizeof(unsigned int));
						materialCBUploadVector.Set(offset + varDesc.StartOffset, &v, sizeof(unsigned int));
					}
					else if constexpr (std::is_same_v<Value, SharedVar<Texture2D>> || std::is_same_v<Value, SharedVar<TextureCube>>)
						assert(false);
					else {
						assert(varDesc.Size == sizeof(Value));
						materialCBUploadVector.Set(offset + varDesc.StartOffset, &value, varDesc.Size);
					}
					}, target->second.value);
			}
		}
	};

	for (auto material : materials) {
		std::set<size_t> flags;
		for (size_t i = 0; i < shader->passes.size(); i++) {
			UpdateShaderCBsForRefl(flags, *material, GPURsrcMngrDX12::Instance().GetShaderRefl_vs(*shader, i));
			UpdateShaderCBsForRefl(flags, *material, GPURsrcMngrDX12::Instance().GetShaderRefl_ps(*shader, i));
		}
	}

	return rst;
}

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetCameraCBAddress(size_t ctxID, size_t cameraIdx) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).cameraOffset
		+ cameraIdx * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants));
}

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetLightCBAddress(size_t ctxID) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).lightOffset;
}

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetObjectCBAddress(size_t ctxID, size_t entityIdx) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).entity2offset.at(entityIdx);
}
