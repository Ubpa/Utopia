#pragma once

#include <Utopia/Render/RenderQueue.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/DX12/IPipeline.h>

#include <Utopia/Core/SharedVar.h>
#include <Utopia/Core/ResourceMap.h>

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
	class ShaderCBMngr;

	static constexpr char StdPipeline_cbPerObject[] = "StdPipeline_cbPerObject";
	static constexpr char StdPipeline_cbPerCamera[] = "StdPipeline_cbPerCamera";
	static constexpr char StdPipeline_cbLightArray[] = "StdPipeline_cbLightArray";
	static constexpr char StdPipeline_srvIBL[] = "StdPipeline_IrradianceMap";
	static constexpr char StdPipeline_srvLTC[] = "StdPipeline_LTC0";

	class PipelineCommonResourceMngr {
	public:
		static PipelineCommonResourceMngr& GetInstance();
		void Init(ID3D12Device* device);
		void Release();

		std::shared_ptr<Material> GetErrorMaterial() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetDefaultSkyboxGpuHandle() const;
		bool IsCommonCB(std::string_view cbDescName) const;
		const UDX12::DescriptorHeapAllocation& GetDefaultIBLSrvDHA() const;

		std::shared_ptr<Texture2D> GetErrorTex2D() const;
		std::shared_ptr<Texture2D> GetWhiteTex2D() const;
		std::shared_ptr<Texture2D> GetBlackTex2D() const;
		std::shared_ptr<Texture2D> GetNormalTex2D() const;
		std::shared_ptr<Texture2D> GetBrdfLutTex2D() const;

		/** idx: 0 / 1 */
		std::shared_ptr<Texture2D> GetLtcTex2D(size_t idx) const;
		const UDX12::DescriptorHeapAllocation& GetLtcSrvDHA() const;

		std::shared_ptr<TextureCube> GetWhiteTexCube() const;
		std::shared_ptr<TextureCube> GetBlackTexCube() const;

	private:
		PipelineCommonResourceMngr();

		std::shared_ptr<Texture2D> errorTex2D;
		std::shared_ptr<Texture2D> whiteTex2D;
		std::shared_ptr<Texture2D> blackTex2D;
		std::shared_ptr<Texture2D> normalTex2D;
		std::shared_ptr<Texture2D> brdfLutTex2D;

		std::shared_ptr<Texture2D> ltcTex2Ds[2];
		UDX12::DescriptorHeapAllocation ltcSrvDHA;

		std::shared_ptr<TextureCube> whiteTexCube;
		std::shared_ptr<TextureCube> blackTexCube;

		std::shared_ptr<Material> errorMat;

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
		size_t ID = static_cast<size_t>(-1);

		struct EntityData {
			valf<16> l2w;
			valf<16> w2l;
			valf<16> prevl2w;
			std::shared_ptr<Mesh> mesh;
			std::pmr::vector<SharedVar<Material>> materials;
		};

		RenderQueue renderQueue;

		D3D12_GPU_DESCRIPTOR_HANDLE skyboxSrvGpuHandle;

		LightArray lightArray;

		std::unordered_map<size_t, EntityData> entity2data;
	};

	struct IBLData {
		~IBLData();

		void Init(ID3D12Device* device);

		D3D12_GPU_DESCRIPTOR_HANDLE lastSkybox{ 0 };
		UINT nextIdx{ static_cast<UINT>(-1) };

		static constexpr size_t IrradianceMapSize = 128;
		static constexpr size_t PreFilterMapSize = 512;
		static constexpr UINT PreFilterMapMipLevels = 5;

		Microsoft::WRL::ComPtr<ID3D12Resource> irradianceMapResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> prefilterMapResource;
		// irradiance map rtv : 0 ~ 5
		// prefilter map rtv  : 6 ~ 5 + 6 * PreFilterMapMipLevels
		UDX12::DescriptorHeapAllocation RTVsDH;
		// irradiance map srv : 0
		// prefilter map rtv  : 1
		// BRDF LUT           : 2
		UDX12::DescriptorHeapAllocation SRVDH;
	};

	RenderContext GenerateRenderContext(size_t ID, std::span<const UECS::World* const> worlds);

	void DrawObjects(
		const ShaderCBMngr& shaderCBMngr,
		const RenderContext& ctx,
		ID3D12GraphicsCommandList* cmdList,
		std::string_view lightMode,
		size_t rtNum,
		DXGI_FORMAT rtFormat,
		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress,
		D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle);
}
