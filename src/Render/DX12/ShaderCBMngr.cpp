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
	commonMaterialCBDescMap.clear();

	{
		auto material = PipelineCommonResourceMngr::GetInstance().GetErrorMaterial();
		auto materialCBDesc = RegisterMaterialCB(materialCBUploadVector, *material);
		commonMaterialCBDescMap.emplace(material->GetInstanceID(), std::move(materialCBDesc));
	}

	{
		auto material = PipelineCommonResourceMngr::GetInstance().GetDirectionalShadowMaterial();
		auto materialCBDesc = RegisterMaterialCB(materialCBUploadVector, *material);
		commonMaterialCBDescMap.emplace(material->GetInstanceID(), std::move(materialCBDesc));
	}
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
	renderCtxData.directionalShadowOffset = renderCtxData.lightOffset
		+ UDX12::Util::CalcConstantBufferByteSize(sizeof(DirectionalShadowConstants));
	renderCtxData.objectOffset = renderCtxData.directionalShadowOffset
		+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray));

	commonCBUploadVector.Resize(
		commonCBUploadVector.Size()
		+ cameraConstantsSpan.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants))
		+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray))
		+ UDX12::Util::CalcConstantBufferByteSize(sizeof(DirectionalShadowConstants))
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

	// directional shadow
	commonCBUploadVector.Set(renderCtxData.directionalShadowOffset, &ctx.directionalShadow, sizeof(DirectionalShadowConstants));

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

	for (const auto& obj : ctx.renderQueue.GetOpaques()) {
		auto materialCBDesc = RegisterMaterialCB(materialCBUploadVector, *obj.material);
		renderCtxData.materialCBDescMap.emplace(obj.material->GetInstanceID(), std::move(materialCBDesc));
	}

	for (const auto& obj : ctx.renderQueue.GetTransparents()) {
		auto materialCBDesc = RegisterMaterialCB(materialCBUploadVector, *obj.material);
		renderCtxData.materialCBDescMap.emplace(obj.material->GetInstanceID(), std::move(materialCBDesc));
	}

	/////////
	// Add //
	/////////

	renderCtxDataMap.emplace(ctx.ID, std::move(renderCtxData));
}

void ShaderCBMngr::SetGraphicsRoot_CBV_SRV(
	ID3D12GraphicsCommandList* cmdList,
	size_t ctxID,
	const Material& material,
	const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
	const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
) const
{
	const MaterialCBDesc* materialCBDescPtr;
	if (auto target = commonMaterialCBDescMap.find(material.GetInstanceID()); target != commonMaterialCBDescMap.end())
		materialCBDescPtr = &target->second;
	else
		materialCBDescPtr = &renderCtxDataMap.at(ctxID).materialCBDescMap.at(material.GetInstanceID());
	const auto& materialCBDesc = *materialCBDescPtr;

	size_t cbPos = materialCBUploadVector.GetResource()->GetGPUVirtualAddress() + materialCBDesc.beginOffset;

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

				if (auto target = materialCBDesc.registerIdx2LocalOffset.find(rsrcDesc.BindPoint); target != materialCBDesc.registerIdx2LocalOffset.end())
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

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetCameraCBAddress(size_t ctxID, size_t cameraIdx) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).cameraOffset
		+ cameraIdx * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants));
}

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetLightCBAddress(size_t ctxID) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).lightOffset;
}

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetDirectionalShadowCBAddress(size_t ctxID) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).directionalShadowOffset;
}

D3D12_GPU_VIRTUAL_ADDRESS ShaderCBMngr::GetObjectCBAddress(size_t ctxID, size_t entityIdx) const {
	return commonCBUploadVector.GetResource()->GetGPUVirtualAddress()
		+ renderCtxDataMap.at(ctxID).entity2offset.at(entityIdx);
}
