#pragma once

#include <Utopia/Render/RenderQueue.h>
#include <Utopia/Render/DX12/IPipeline.h>

#include <Utopia/Core/SharedVar.h>

#include <UDX12/UDX12.h>

#include <UGM/UGM.hpp>

#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace Ubpa::Utopia {
	struct Material;
	struct Shader;
	struct RenderState;

	class PipelineCommonResourceMngr {
	public:
		static PipelineCommonResourceMngr& GetInstance();
		void Init(ID3D12Device* device);
		void Release();

		std::shared_ptr<Material> GetErrorMaterial() const;
		std::shared_ptr<Shader> GetSkyboxShader() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetDefaultSkyboxGpuHandle() const;
		bool IsCommonCB(std::string_view cbDescName) const;
		const UDX12::DescriptorHeapAllocation& GetDefaultIBLSrvDHA() const;

	private:
		PipelineCommonResourceMngr() = default;

		std::shared_ptr<Material> errorMat;
		std::shared_ptr<Shader> skyboxShader;
		D3D12_GPU_DESCRIPTOR_HANDLE defaultSkyboxGpuHandle;
		/**
		 * use black cube texture and balck texture 2d to generate the ibl resources
		 * 3 SRV
		 * - irradiance
		 * - prefilter
		 * - IBL lut
		 */
		UDX12::DescriptorHeapAllocation defaultIBLSrvDHA;

		std::set<std::string, std::less<>> commonCBs;
	};

	struct ShaderCBDesc {
		// material 0 cbs   -- offset
		//  material 0 cb 0
		//  material 0 cb 1
		//  ...
		// material 1 cbs   -- offset + materialCBSize
		// ...

		// global offset = begin_offset + indexMap[material] * materialCBSize + offsetMap[register index]
		size_t begin_offset;
		size_t materialCBSize{ 0 };
		std::map<size_t, size_t> offsetMap; // register index -> local offset
		std::unordered_map<size_t, size_t> indexMap; // material ID -> index
	};

	struct ObjectConstants {
		transformf World;
		transformf InvWorld;
		transformf PrevWorld;
	};

	struct CameraConstants {
		val<float, 16> View;

		val<float, 16> InvView;

		val<float, 16> Proj;

		val<float, 16> InvProj;

		val<float, 16> ViewProj;

		val<float, 16> UnjitteredViewProj;

		val<float, 16> InvViewProj;

		val<float, 16> PrevViewProj;

		pointf3 EyePosW;
		unsigned int FrameCount{ 0 };

		valf2 RenderTargetSize;
		valf2 InvRenderTargetSize;

		float NearZ;
		float FarZ;
		float TotalTime;
		float DeltaTime;

		valf2 Jitter;
		unsigned int padding0;
		unsigned int padding1;
	};

	struct ShaderLight {
		rgbf color;
		float range;
		vecf3 dir;
		float f0;
		pointf3 position;
		float f1;
		vecf3 horizontal;
		float f2;

		struct Spot {
			static constexpr auto pCosHalfInnerSpotAngle = &ShaderLight::f0;
			static constexpr auto pCosHalfOuterSpotAngle = &ShaderLight::f1;
		};
		struct Rect {
			static constexpr auto pWidth = &ShaderLight::f0;
			static constexpr auto pHeight = &ShaderLight::f1;
		};
		struct Disk {
			static constexpr auto pWidth = &ShaderLight::f0;
			static constexpr auto pHeight = &ShaderLight::f1;
		};
	};

	struct LightArray {
		static constexpr size_t size = 16;

		UINT diectionalLightNum{ 0 };
		UINT pointLightNum{ 0 };
		UINT spotLightNum{ 0 };
		UINT rectLightNum{ 0 };
		UINT diskLightNum{ 0 };
		const UINT _g_cbLightArray_pad0{ static_cast<UINT>(-1) };
		const UINT _g_cbLightArray_pad1{ static_cast<UINT>(-1) };
		const UINT _g_cbLightArray_pad2{ static_cast<UINT>(-1) };
		ShaderLight lights[size];
	};

	struct RenderContext {
		struct EntityData {
			valf<16> l2w;
			valf<16> w2l;
			valf<16> prevl2w;
			std::shared_ptr<Mesh> mesh;
			std::pmr::vector<SharedVar<Material>> materials;
		};

		RenderQueue renderQueue;

		std::unordered_map<const Shader*, ShaderCBDesc> shaderCBDescMap; // shader ID -> desc

		D3D12_GPU_DESCRIPTOR_HANDLE skyboxGpuHandle;

		LightArray lightArray;

		// common
		size_t cameraOffset;
		size_t lightOffset;
		size_t objectOffset;
		std::unordered_map<size_t, EntityData> entity2data;
		std::unordered_map<size_t, size_t> entity2offset;
	};

	ShaderCBDesc UpdateShaderCBs(
		UDX12::DynamicUploadVector& cb,
		const Shader& shader,
		const std::unordered_set<const Material*>& materials
	);

	void SetGraphicsRoot_CBV_SRV(
		ID3D12GraphicsCommandList* cmdList,
		UDX12::DynamicUploadVector& cb,
		const ShaderCBDesc& shaderCBDesc,
		const Material& material,
		const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
		const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
	);

	RenderContext GenerateRenderContext(
		std::span<const UECS::World* const> worlds,
		std::span<const CameraConstants* const> cameraConstantsSpan,
		UDX12::DynamicUploadVector& shaderCB,
		UDX12::DynamicUploadVector& commonShaderCB);
}
