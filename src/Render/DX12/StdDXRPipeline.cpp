#include <Utopia/Render/DX12/StdDXRPipeline.h>

#include "../_deps/LTCTex.h"

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
#include <Utopia/Render/DX12/MeshLayoutMngr.h>
#include <Utopia/Render/DX12/ShaderCBMngrDX12.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/RenderQueue.h>

#include <Utopia/Core/AssetMngr.h>

#include <Utopia/Core/Image.h>
#include <Utopia/Render/Components/Camera.h>
#include <Utopia/Render/Components/MeshFilter.h>
#include <Utopia/Render/Components/MeshRenderer.h>
#include <Utopia/Render/Components/Skybox.h>
#include <Utopia/Render/Components/Light.h>
#include <Utopia/Core/GameTimer.h>

#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/Translation.h>
#include <Utopia/Core/Components/WorldToLocal.h>

#include <UECS/UECS.hpp>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_win32.h>
#include <_deps/imgui/imgui_impl_dx12.h>

#include <UDX12/FrameResourceMngr.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;
using namespace Ubpa;

#pragma region DXR

static const D3D12_HEAP_PROPERTIES kUploadHeapProps = {
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};
static const D3D12_HEAP_PROPERTIES kDefaultHeapProps = {
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};
Microsoft::WRL::ComPtr<ID3D12Resource> createBuffer(
	ID3D12Device* pDevice,
	uint64_t size,
	D3D12_RESOURCE_FLAGS flags,
	D3D12_RESOURCE_STATES initState,
	const D3D12_HEAP_PROPERTIES& heapProps)
{
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = flags;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = size;

	Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
	ThrowIfFailed(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
	return pBuffer;
}
struct RootSignatureDesc {
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};
RootSignatureDesc createRayGenRootDesc()
{
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(4);
	std::size_t descIdx = 0;
	// gOutput (2)
	desc.range[descIdx].BaseShaderRegister = 0; // u0 rt result, u1 rt u result
	desc.range[descIdx].NumDescriptors = 2;
	desc.range[descIdx].RegisterSpace = 0;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// gbuffers (3)
	desc.range[descIdx].BaseShaderRegister = 1; // t1,t2,t3
	desc.range[descIdx].NumDescriptors = 3;
	desc.range[descIdx].RegisterSpace = 0;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// depth stencil
	desc.range[descIdx].BaseShaderRegister = 4; // t4
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = 0;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// BRDF lut
	desc.range[descIdx].BaseShaderRegister = 5; // t5
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = 0;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	assert(descIdx == desc.range.size());

	desc.rootParams.resize(5);
	std::size_t rootParamIdx = 0;
	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data();
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 1;
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 2;
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 3;
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	desc.rootParams[rootParamIdx].Descriptor.RegisterSpace = 0;
	desc.rootParams[rootParamIdx].Descriptor.ShaderRegister = 0;
	rootParamIdx++;

	assert(rootParamIdx == desc.rootParams.size());
	
	// Create the desc
	desc.desc.NumParameters = (UINT)desc.rootParams.size();
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}
RootSignatureDesc createIndirectMissShaderRootDesc() {
	constexpr UINT space = 1;

	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(1);
	std::size_t descIdx = 0;
	// cubemap
	desc.range[descIdx].BaseShaderRegister = 0; // t0
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	desc.rootParams.resize(1);
	std::size_t rootParamIdx = 0;
	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data();
	rootParamIdx++;

	// Create the desc
	desc.desc.NumParameters = (UINT)desc.rootParams.size();
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}
RootSignatureDesc createIndirectRootDesc()
{
	constexpr UINT space = 2;
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(5);
	std::size_t descIdx = 0;
	// {vb, ib}
	desc.range[descIdx].BaseShaderRegister = 0; // t0, t1
	desc.range[descIdx].NumDescriptors = 2;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// albedo
	desc.range[descIdx].BaseShaderRegister = 2; // t2
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// roughness
	desc.range[descIdx].BaseShaderRegister = 3; // t3
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// metalness
	desc.range[descIdx].BaseShaderRegister = 4; // t4
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// normal
	desc.range[descIdx].BaseShaderRegister = 5; // t5
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	// {vb, ib}
	// albedo, roughness, metalness, normal
	desc.rootParams.resize(5);
	std::size_t rootParamIdx = 0;
	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1; // [t0...t1]
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data(); // t0 vb, t1 ib
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 1; // t2 albedo
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 2; // t3 roughness
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 3; // t4 metalness
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data() + 4; // t5 normal
	rootParamIdx++;

	// Create the desc
	desc.desc.NumParameters = (UINT)desc.rootParams.size();
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}
RootSignatureDesc creatGlobalRootDesc()
{
	constexpr UINT space = 0;
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(1);
	std::size_t descIdx = 0;
	// rtScene
	desc.range[descIdx].BaseShaderRegister = 0; // t0
	desc.range[descIdx].NumDescriptors = 1;
	desc.range[descIdx].RegisterSpace = space;
	desc.range[descIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[descIdx].OffsetInDescriptorsFromTableStart = 0;
	descIdx++;

	assert(descIdx == desc.range.size());

	desc.rootParams.resize(2);
	std::size_t rootParamIdx = 0;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[rootParamIdx].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[rootParamIdx].DescriptorTable.pDescriptorRanges = desc.range.data();
	rootParamIdx++;

	desc.rootParams[rootParamIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	desc.rootParams[rootParamIdx].Descriptor.ShaderRegister = 1;
	desc.rootParams[rootParamIdx].Descriptor.RegisterSpace = space;
	rootParamIdx++;

	assert(rootParamIdx == desc.rootParams.size());

	// Create the desc
	desc.desc.NumParameters = (UINT)desc.rootParams.size();
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	return desc;
}
struct DxilLibrary {
	DxilLibrary(Microsoft::WRL::ComPtr<ID3DBlob> pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
	{
		stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stateSubobject.pDesc = &dxilLibDesc;

		dxilLibDesc = {};
		exportDesc.resize(entryPointCount);
		exportName.resize(entryPointCount);
		if (pBlob)
		{
			dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
			dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
			dxilLibDesc.NumExports = entryPointCount;
			dxilLibDesc.pExports = exportDesc.data();

			for (uint32_t i = 0; i < entryPointCount; i++)
			{
				exportName[i] = entryPoint[i];
				exportDesc[i].Name = exportName[i].c_str();
				exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[i].ExportToRename = nullptr;
			}
		}
	};

	DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	D3D12_STATE_SUBOBJECT stateSubobject{};
	Microsoft::WRL::ComPtr<ID3DBlob> pShaderBlob;
	std::vector<D3D12_EXPORT_DESC> exportDesc;
	std::vector<std::wstring> exportName;
};
static const WCHAR* kRayGenShader = L"RayGen";
static const WCHAR* kShadowMissShader = L"ShadowMiss";
static const WCHAR* kShadowHitGroup = L"ShadowHitGroup";
static const WCHAR* kIndirectAHS = L"IndirectAHS";
static const WCHAR* kIndirectCHS = L"IndirectCHS";
static const WCHAR* kIndirectMissShader = L"IndirectMiss";
static const WCHAR* kIndirectHitGroup = L"IndirectHitGroup";
DxilLibrary createDxilLibrary() {
	// Compile the shader
	auto hlsl = AssetMngr::Instance().LoadAsset<HLSLFile>(LR"(shaders\BasicRayTracing.rt.hlsl)");
	Microsoft::WRL::ComPtr<ID3DBlob> pDxilLib = UDX12::Util::CompileLibrary(hlsl->GetText().data(), (UINT32)hlsl->GetText().size(), LR"(shaders\BasicRayTracing.rt.hlsl)");
	const WCHAR* entryPoints[] = { kRayGenShader, kShadowMissShader, kIndirectAHS, kIndirectCHS, kIndirectMissShader };
	return DxilLibrary(pDxilLib, entryPoints, 5);
}
struct HitGroup {
	HitGroup(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name) : exportName(name)
	{
		desc = {};
		desc.AnyHitShaderImport = ahsExport;
		desc.ClosestHitShaderImport = chsExport;
		desc.HitGroupExport = exportName.c_str();

		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObject.pDesc = &desc;
	}

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc;
	D3D12_STATE_SUBOBJECT subObject;
};
struct ExportAssociation {
	ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
	{
		association.NumExports = exportCount;
		association.pExports = exportNames;
		association.pSubobjectToAssociate = pSubobjectToAssociate;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobject.pDesc = &association;
	}

	D3D12_STATE_SUBOBJECT subobject = {};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
};
struct LocalRootSignature
{
	LocalRootSignature(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = UDX12::Device{ Microsoft::WRL::ComPtr<ID3D12Device>(pDevice) }.CreateRootSignature(desc);
		pInterface = pRootSig.Get();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	}
	Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};
struct GlobalRootSignature {
	GlobalRootSignature(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = UDX12::Device{ Microsoft::WRL::ComPtr<ID3D12Device>(pDevice) }.CreateRootSignature(desc);
		pInterface = pRootSig.Get();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	}
	Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};
struct ShaderConfig {
	ShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
	{
		shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
		shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		subobject.pDesc = &shaderConfig;
	}

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};
struct PipelineConfig {
	PipelineConfig(uint32_t maxTraceRecursionDepth)
	{
		config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		subobject.pDesc = &config;
	}

	D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};
Microsoft::WRL::ComPtr<ID3D12StateObject> createRtPipelineState(ID3D12Device5* device) {
	// Need 15 subobjects:
	//  1 for the DXIL library
	//  1 for hit-group (chs)
	//  2 for RayGen root-signature (root-signature and the subobject association)
    //  2 for chs hit-program root-signature (root-signature and the subobject association)
	//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
	//  1 for pipeline config
	//  1 for the global root signature
	//  1 for hit-group (indirectCHS)
	//  2 for indirect chs root signature ({vb, ib}, albedo, metalness, roughness, normal, constants) and association
	//  2 for indirect miss shader root signature (cubemap) and association
	std::array<D3D12_STATE_SUBOBJECT, 15> subobjects;
	uint32_t index = 0;

	// [0]
	// Create the DXIL library: rayGen, miss, chs, indirectCHS
	DxilLibrary dxilLib = createDxilLibrary();
	subobjects[index++] = dxilLib.stateSubobject;

	// [1]
	// {default intersection, default any hit, chs}
	// name: HitGroup
	HitGroup hitGroup(nullptr, nullptr, kShadowHitGroup);
	subobjects[index++] = hitGroup.subObject;

	// [2]
	// Create the ray-gen root-signature and association
	uint32_t rgsRootIndex = index;
	LocalRootSignature rgsRootSignature(device, createRayGenRootDesc().desc);
	subobjects[index++] = rgsRootSignature.subobject;

	// [3]
	// bind: ray-gen shader <-> root signature
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject;

	// [4]
	// Create the miss- and hit-programs root-signature
	uint32_t emptyRootSignatureIndex = index;
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature emptyRootSignature(device, emptyDesc);
	subobjects[index++] = emptyRootSignature.subobject;

	// [5]
	// bind: miss <-> empty root-signature
	const WCHAR* emptyRootSignatureExports[] = { kShadowMissShader, kShadowHitGroup };
	ExportAssociation missHitRootAssociation(emptyRootSignatureExports, 2, &(subobjects[emptyRootSignatureIndex]));
	subobjects[index++] = missHitRootAssociation.subobject;

	// [6]
	// Bind the payload size to the programs
	uint32_t shaderConfigIndex = index;
	// float3 color + uint randSeed + uint depth
	ShaderConfig shaderConfig(sizeof(float) * 2 /* barycentric coordinates */, sizeof(float) * 3 + sizeof(UINT) * 2);
	subobjects[index++] = shaderConfig.subobject;

	// [7]
	const WCHAR* shaderExports[] = { kShadowMissShader, kRayGenShader, kIndirectAHS, kIndirectCHS, kIndirectMissShader };
	ExportAssociation configAssociation(shaderExports, 5, &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject;

	// [8]
	// Create the pipeline config
	PipelineConfig config(D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH);
	subobjects[index++] = config.subobject;

	// [9]
	// Create the global root signature (6 static samplers)
	auto samplers = GPURsrcMngrDX12::Instance().GetStaticSamplers();
	auto grsdesc = creatGlobalRootDesc();
	grsdesc.desc.NumStaticSamplers = (UINT)samplers.size();
	grsdesc.desc.pStaticSamplers = samplers.data();
	GlobalRootSignature root(device, grsdesc.desc);
	subobjects[index++] = root.subobject;

	// [10]
	// hit group (indirectAHS, indirectCHS)
	HitGroup indirectHitGroup(kIndirectAHS, kIndirectCHS, kIndirectHitGroup);
	subobjects[index++] = indirectHitGroup.subObject;

	// [11]
	// indirect ahs/chs root signature
	uint32_t indirectRootSignatureIndex = index;
	LocalRootSignature indirectRootSig(device, createIndirectRootDesc().desc);
	subobjects[index++] = indirectRootSig.subobject;

	// [12]
	// association: indirect ahs/chs root sig <-> indirect ahs/chs
	const WCHAR* indirectExports[] = { kIndirectAHS, kIndirectCHS };
	ExportAssociation indirectRootAssociation(indirectExports, 2, &(subobjects[indirectRootSignatureIndex]));
	subobjects[index++] = indirectRootAssociation.subobject;

	// [13]
	// Create the indirect miss shader local root signature
	uint32_t indirectMissShaderLocalRootSignatureIndex = index;
	LocalRootSignature indirectMissShaderLocalRootSignature(device, createIndirectMissShaderRootDesc().desc);
	subobjects[index++] = indirectMissShaderLocalRootSignature.subobject;

	// [14]
	// bind: indirect miss shader <-> indirect miss shader local root signature
	ExportAssociation indirectMissShaderEntryLocalRootSignatureAssociation(&kIndirectMissShader, 1, &(subobjects[indirectMissShaderLocalRootSignatureIndex]));
	subobjects[index++] = indirectMissShaderEntryLocalRootSignatureAssociation.subobject;

	// Create the state
	assert(index == subobjects.size());
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index;
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	
	Microsoft::WRL::ComPtr<ID3D12StateObject> pPipelineState;
	ThrowIfFailed(device->CreateStateObject(&desc, IID_PPV_ARGS(&pPipelineState)));
	return pPipelineState;
}
// raygenTable: output uav, tlas srv
Microsoft::WRL::ComPtr<ID3D12Resource> createShaderTable(
	ID3D12Device5* device,
	ID3D12StateObject* pipelineState,
	D3D12_GPU_DESCRIPTOR_HANDLE raygenTable,
	std::size_t shaderTableEntrySize)
{
	/** The shader-table layout is as follows:
		Entry 0 - Ray-gen program
		Entry 1 - Shadow Miss program
		Entry 2 - Indirect Miss program
		All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
		The ray-gen program requires the largest entry - sizeof(program identifier) + 8 bytes for a descriptor-table.
		The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	*/

	// Calculate the size and create the buffer
	uint32_t shaderTableSize = uint32_t(shaderTableEntrySize * 3);

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	Microsoft::WRL::ComPtr<ID3D12Resource> pShaderTable = createBuffer(device,
		shaderTableSize,
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps);

	// Map the buffer
	uint8_t* pData;
	ThrowIfFailed(pShaderTable->Map(0, nullptr, (void**)&pData));

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	pipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

	// Entry 0 - ray-gen shader ID and descriptor data
	memcpy(pData, pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = raygenTable.ptr;

	// Entry 1 - shadow miss shader
	memcpy(pData + shaderTableEntrySize, pRtsoProps->GetShaderIdentifier(kShadowMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - indirect miss shader
	memcpy(pData + 2 * shaderTableEntrySize, pRtsoProps->GetShaderIdentifier(kIndirectMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Unmap
	pShaderTable->Unmap(0, nullptr);

	return pShaderTable;
}
#pragma endregion

struct StdDXRPipeline::Impl {
	Impl(InitDesc initDesc) :
		initDesc{ initDesc },
		frameRsrcMngr{ initDesc.numFrame, initDesc.device },
		fg{ "Standard Pipeline" },
		crossFrameShaderCBMngr{ initDesc.device }
	{
		ThrowIfFailed(initDesc.device->QueryInterface(IID_PPV_ARGS(&dxr_device)));
		rtpso = createRtPipelineState(dxr_device.Get());
		// Calculate the size and create the buffer
		// id
		// [ray gen]
		// - table: result uav, result U uav, tlas srv
		// - table: gbuffers (3)
		// - table: depth
		// - table: brdf lut
		// - cbv : camera
		// [indirect miss]
		// - table : skybox
		// [indirect chs]
		// - table: vb, ib
		// - table: albedo
		// - table: roughness
		// - table: metalness
		// - table: normal
		// // - cbv: meterial constants
		auto maxalignment = std::max(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

		mHitGroupTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		mHitGroupTableEntrySize += 8 * 5;
		mHitGroupTableEntrySize = (mHitGroupTableEntrySize + maxalignment - 1) & ~(maxalignment - 1);

		mRayGenAndMissShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		mRayGenAndMissShaderTableEntrySize += 8 * 5;
		mRayGenAndMissShaderTableEntrySize = (mRayGenAndMissShaderTableEntrySize + maxalignment - 1) & ~(maxalignment - 1);

		auto samplers = GPURsrcMngrDX12::Instance().GetStaticSamplers();
		auto grsdesc = creatGlobalRootDesc();
		grsdesc.desc.NumStaticSamplers = (UINT)samplers.size();
		grsdesc.desc.pStaticSamplers = samplers.data();
		mpGlobalRootSig = UDX12::Device{ Microsoft::WRL::ComPtr<ID3D12Device>(initDesc.device) }.CreateRootSignature(grsdesc.desc);
		BuildTextures();
		BuildFrameResources();
		BuildShaders();
	}

	~Impl();

	struct ObjectConstants {
		transformf World;
		transformf InvWorld;
	};
	struct CameraConstants {
		transformf View;

		transformf InvView;

		transformf Proj;

		transformf InvProj;

		transformf ViewProj;

		transformf InvViewProj;

		transformf PrevViewProj;

		pointf3 EyePosW;
		std::uint32_t FrameCount{ 0 };

		valf2 RenderTargetSize;
		valf2 InvRenderTargetSize;

		float NearZ;
		float FarZ;
		float TotalTime;
		float DeltaTime;

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

	struct QuadPositionLs {
		valf4 positionL4x;
		valf4 positionL4y;
		valf4 positionL4z;
	};

	struct MipInfo {
		float roughness;
		float resolution;
	};

	struct IBLData {
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
	UDX12::DescriptorHeapAllocation defaultIBLSRVDH; // 3

	struct RenderContext {
		RenderQueue renderQueue;

		bool cameraChange;
		CameraConstants cameraConstants;

		std::unordered_map<const Shader*, PipelineBase::ShaderCBDesc> shaderCBDescMap; // shader ID -> desc

		D3D12_GPU_DESCRIPTOR_HANDLE skybox;
		LightArray lights;

		// common
		size_t cameraOffset = 0;
		size_t lightOffset = cameraOffset + UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants));
		size_t objectOffset = lightOffset + UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray));
		struct EntityData {
			valf<16> l2w;
			valf<16> w2l;
			std::shared_ptr<Mesh> mesh;
			std::pmr::vector<SharedVar<Material>> materials;
		};
		std::unordered_map<size_t, EntityData> entity2data;
		std::unordered_map<size_t, size_t> entity2offset;
	};

	const InitDesc initDesc;

	static constexpr char StdDXRPipeline_cbPerObject[] = "StdPipeline_cbPerObject";
	static constexpr char StdDXRPipeline_cbPerCamera[] = "StdPipeline_cbPerCamera";
	static constexpr char StdDXRPipeline_cbLightArray[] = "StdPipeline_cbLightArray";
	static constexpr char StdDXRPipeline_srvIBL[] = "StdPipeline_IrradianceMap";
	static constexpr char StdDXRPipeline_srvLTC[] = "StdPipeline_LTC0";

	const std::set<std::string_view> commonCBs{
		StdDXRPipeline_cbPerObject,
		StdDXRPipeline_cbPerCamera,
		StdDXRPipeline_cbLightArray
	};

	Texture2D ltcTexes[2];
	UDX12::DescriptorHeapAllocation ltcHandles; // 2

	RenderContext renderContext;
	D3D12_GPU_DESCRIPTOR_HANDLE defaultSkybox;
	std::shared_ptr<Material> errorMat;
	std::shared_ptr<Texture2D> errorTex;
	std::shared_ptr<Texture2D> dnormalTex;
	std::shared_ptr<Texture2D> blackTex;
	std::shared_ptr<Texture2D> whiteTex;

	UDX12::FrameResourceMngr frameRsrcMngr;

	UDX12::FG::Executor fgExecutor;
	UFG::Compiler fgCompiler;
	UFG::FrameGraph fg;

	std::shared_ptr<Shader> deferLightingShader;
	std::shared_ptr<Shader> skyboxShader;
	std::shared_ptr<Shader> postprocessShader;
	std::shared_ptr<Shader> irradianceShader;
	std::shared_ptr<Shader> prefilterShader;
	std::shared_ptr<Shader> gaussBlurXShader;
	std::shared_ptr<Shader> gaussBlurYShader;
	std::shared_ptr<Shader> atrousShader;
	std::shared_ptr<Shader> rtWithBRDFLUTShader;
	std::shared_ptr<Shader> rtReprojectShader;
	std::shared_ptr<Shader> taaShader;
	std::shared_ptr<Shader> filterMomentsShader;

	ShaderCBMngrDX12 crossFrameShaderCBMngr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// DXR
	Microsoft::WRL::ComPtr<ID3D12Device5> dxr_device;
	Microsoft::WRL::ComPtr<ID3D12StateObject> rtpso;
	std::size_t mRayGenAndMissShaderTableEntrySize;
	std::size_t mHitGroupTableEntrySize;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	Shader placeholder_rtshader; // a place holder for cbuffer
	struct RTConfig {
		int gFrameCnt;
	};
	struct TLASData {
		TLASData(ID3D12Device5* device, ID3D12StateObject* rtpso, std::size_t entrySize) : instanceDescs(device), hitGroupTable(device)  {
			raygenDescriptors = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(2); // u0 rt result, u1 rt u result
			shaderTable = createShaderTable(device, rtpso, raygenDescriptors.GetGpuHandle(), entrySize);
		}
		~TLASData() {
			UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(raygenDescriptors));
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> pResult;
		Microsoft::WRL::ComPtr<ID3D12Resource> pScratch;
		Microsoft::WRL::ComPtr<ID3D12Resource> pUpdateScratch;
		UDX12::DynamicUploadBuffer instanceDescs;
		UDX12::DescriptorHeapAllocation raygenDescriptors; // uav
		Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable; // raygen, shadow miss, indirect miss
		UDX12::DynamicUploadBuffer hitGroupTable;
		void Reserve(ID3D12Device* device, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& new_info, std::size_t new_cnt_instances);
	};

	size_t lastWidth = 0;
	size_t lastHeight = 0;

	void BuildTextures();
	void BuildFrameResources();
	void BuildShaders();

	void UpdateRenderContext(const std::vector<const UECS::World*>& worlds, size_t width, size_t height, const CameraData& cameraData);
	void UpdateShaderCBs();
	void Render(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData, ID3D12Resource* rtb);
	void DrawObjects(ID3D12GraphicsCommandList*, std::string_view lightMode, size_t rtNum, DXGI_FORMAT rtFormat);
	void Resize(size_t width, size_t height);
};

void StdDXRPipeline::Impl::TLASData::Reserve(ID3D12Device* device,
	const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& new_info,
	std::size_t new_cnt_instances)
{
	const auto default_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	if (new_info.ResultDataMaxSizeInBytes != 0 && !pResult || pResult->GetDesc().Width < new_info.ResultDataMaxSizeInBytes) {
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(new_info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		device->CreateCommittedResource(
			&default_heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&pResult));
	}
	if (new_info.ScratchDataSizeInBytes != 0 && (!pScratch || pScratch->GetDesc().Width < new_info.ScratchDataSizeInBytes)) {
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(new_info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		device->CreateCommittedResource(
			&default_heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&pScratch));
	}
	if (new_info.UpdateScratchDataSizeInBytes != 0 && (!pUpdateScratch || pUpdateScratch->GetDesc().Width < new_info.UpdateScratchDataSizeInBytes)) {
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(new_info.UpdateScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		device->CreateCommittedResource(
			&default_heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&pUpdateScratch));
	}
	instanceDescs.FastReserve(std::max<size_t>(1,new_cnt_instances) * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
}

StdDXRPipeline::Impl::~Impl() {
	for (auto& fr : frameRsrcMngr.GetFrameResources()) {
		auto data = fr->GetResource<std::shared_ptr<IBLData>>("IBL data");
		UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(data->RTVsDH));
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(data->SRVDH));
	}
	UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(defaultIBLSRVDH));
	GPURsrcMngrDX12::Instance().UnregisterTexture2D(ltcTexes[0].GetInstanceID());
	GPURsrcMngrDX12::Instance().UnregisterTexture2D(ltcTexes[1].GetInstanceID());
	UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(ltcHandles));
}

void StdDXRPipeline::Impl::BuildTextures() {
	ltcTexes[0].image = Image(LTCTex::SIZE, LTCTex::SIZE, 4, LTCTex::data1);
	ltcTexes[1].image = Image(LTCTex::SIZE, LTCTex::SIZE, 4, LTCTex::data2);
	GPURsrcMngrDX12::Instance().RegisterTexture2D(ltcTexes[0]);
	GPURsrcMngrDX12::Instance().RegisterTexture2D(ltcTexes[1]);
	ltcHandles = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(2);
	auto ltc0 = GPURsrcMngrDX12::Instance().GetTexture2DResource(ltcTexes[0]);
	auto ltc1 = GPURsrcMngrDX12::Instance().GetTexture2DResource(ltcTexes[1]);
	const auto ltc0SRVDesc = UDX12::Desc::SRV::Tex2D(ltc0->GetDesc().Format);
	const auto ltc1SRVDesc = UDX12::Desc::SRV::Tex2D(ltc1->GetDesc().Format);
	initDesc.device->CreateShaderResourceView(
		ltc0,
		&ltc0SRVDesc,
		ltcHandles.GetCpuHandle(static_cast<uint32_t>(0))
	);
	initDesc.device->CreateShaderResourceView(
		ltc1,
		&ltc1SRVDesc,
		ltcHandles.GetCpuHandle(static_cast<uint32_t>(1))
	);

	auto blackTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\black.png)");
	auto blackRsrc = GPURsrcMngrDX12::Instance().GetTexture2DResource(*blackTex2D);
	// TODO bugs!
	//auto skyboxBlack = AssetMngr::Instance().LoadAsset<Material>(LR"(_internal\materials\skyBlack.mat)");
	//auto blackTexCube = std::get<SharedVar<TextureCube>>(tmp->properties.find("gSkybox")->second.value);
	//auto tmp = std::make_shared<Material>();
	//tmp->properties.emplace("gSkybox", blackTexCube);
	auto blackTexCube = SharedVar<TextureCube>(AssetMngr::Instance().LoadAsset<TextureCube>(LR"(_internal\textures\blackCube.png)"));
	auto blackTexCubeRsrc = GPURsrcMngrDX12::Instance().GetTextureCubeResource(*blackTexCube);
	defaultSkybox = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*blackTexCube);

	errorMat = AssetMngr::Instance().LoadAsset<Material>(LR"(_internal\materials\error.mat)");
	errorTex = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\error.png)");
	dnormalTex = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\normal.png)");
	blackTex = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\black.png)");
	whiteTex = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\white.png)");

	defaultIBLSRVDH = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(3);
	auto cubeDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto lutDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	initDesc.device->CreateShaderResourceView(blackTexCubeRsrc, &cubeDesc, defaultIBLSRVDH.GetCpuHandle(0));
	initDesc.device->CreateShaderResourceView(blackTexCubeRsrc, &cubeDesc, defaultIBLSRVDH.GetCpuHandle(1));
	initDesc.device->CreateShaderResourceView(blackRsrc, &lutDesc, defaultIBLSRVDH.GetCpuHandle(2));
}

void StdDXRPipeline::Impl::BuildFrameResources() {
	for (const auto& fr : frameRsrcMngr.GetFrameResources()) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(initDesc.device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));

		fr->RegisterResource("CommandAllocator", std::move(allocator));

		fr->RegisterResource("ShaderCBMngrDX12", std::make_shared<ShaderCBMngrDX12>(initDesc.device));

		auto fgRsrcMngr = std::make_shared<UDX12::FG::RsrcMngr>();
		fr->RegisterResource("FrameGraphRsrcMngr", fgRsrcMngr);

		D3D12_CLEAR_VALUE clearColor;
		clearColor.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		clearColor.Color[0] = 0.f;
		clearColor.Color[1] = 0.f;
		clearColor.Color[2] = 0.f;
		clearColor.Color[3] = 1.f;
		auto iblData = std::make_shared<IBLData>();
		iblData->RTVsDH = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(6 * (1 + IBLData::PreFilterMapMipLevels));
		iblData->SRVDH = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(3);
		{ // irradiance
			auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(
				IBLData::IrradianceMapSize, IBLData::IrradianceMapSize,
				1, DXGI_FORMAT_R32G32B32A32_FLOAT
			);
			const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			initDesc.device->CreateCommittedResource(
				&defaultHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&rsrcDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clearColor,
				IID_PPV_ARGS(&iblData->irradianceMapResource)
			);
			for (UINT i = 0; i < 6; i++) {
				auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i);
				initDesc.device->CreateRenderTargetView(iblData->irradianceMapResource.Get(), &rtvDesc, iblData->RTVsDH.GetCpuHandle(i));
			}
			auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
			initDesc.device->CreateShaderResourceView(iblData->irradianceMapResource.Get(), &srvDesc, iblData->SRVDH.GetCpuHandle(0));
		}
		{ // prefilter
			auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(
				IBLData::PreFilterMapSize, IBLData::PreFilterMapSize,
				IBLData::PreFilterMapMipLevels, DXGI_FORMAT_R32G32B32A32_FLOAT
			);
			const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			initDesc.device->CreateCommittedResource(
				&defaultHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&rsrcDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clearColor,
				IID_PPV_ARGS(&iblData->prefilterMapResource)
			);
			for (UINT mip = 0; mip < IBLData::PreFilterMapMipLevels; mip++) {
				for (UINT i = 0; i < 6; i++) {
					auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i, mip);
					initDesc.device->CreateRenderTargetView(
						iblData->prefilterMapResource.Get(),
						&rtvDesc,
						iblData->RTVsDH.GetCpuHandle(6 * (1 + mip) + i)
					);
				}
			}
			auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, IBLData::PreFilterMapMipLevels);
			initDesc.device->CreateShaderResourceView(iblData->prefilterMapResource.Get(), &srvDesc, iblData->SRVDH.GetCpuHandle(1));
		}
		{// BRDF LUT
			auto brdfLUTTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\BRDFLUT.png)");
			auto brdfLUTTex2DRsrc = GPURsrcMngrDX12::Instance().GetTexture2DResource(*brdfLUTTex2D);
			auto desc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32_FLOAT);
			initDesc.device->CreateShaderResourceView(brdfLUTTex2DRsrc, &desc, iblData->SRVDH.GetCpuHandle(2));
		}
		fr->RegisterResource("IBL data", iblData);

		fr->RegisterResource("TLASData", std::make_shared<TLASData>(dxr_device.Get(), rtpso.Get(), mRayGenAndMissShaderTableEntrySize));

		fr->RegisterResource("taaPrevRsrc", Microsoft::WRL::ComPtr<ID3D12Resource>{});

		// create resuource in resize
		fr->RegisterResource("rtSRsrc", Microsoft::WRL::ComPtr<ID3D12Resource>{});
		fr->RegisterResource("rtURsrc", Microsoft::WRL::ComPtr<ID3D12Resource>{});
		fr->RegisterResource("rtColorMoment", Microsoft::WRL::ComPtr<ID3D12Resource>{});
		fr->RegisterResource("rtHistoryLength", Microsoft::WRL::ComPtr<ID3D12Resource>{});
		fr->RegisterResource("rtLinearZ", Microsoft::WRL::ComPtr<ID3D12Resource>{});
		fr->RegisterResource("RTConfig", RTConfig{ -1 });
	}
}

void StdDXRPipeline::Impl::BuildShaders() {
	deferLightingShader = ShaderMngr::Instance().Get("StdPipeline/Defer Lighting");
	skyboxShader = ShaderMngr::Instance().Get("StdPipeline/Skybox");
	postprocessShader = ShaderMngr::Instance().Get("StdPipeline/Post Process");
	irradianceShader = ShaderMngr::Instance().Get("StdPipeline/Irradiance");
	prefilterShader = ShaderMngr::Instance().Get("StdPipeline/PreFilter");
	gaussBlurXShader = ShaderMngr::Instance().Get("StdPipeline/GaussBlur_x");
	gaussBlurYShader = ShaderMngr::Instance().Get("StdPipeline/GaussBlur_y");
	atrousShader = ShaderMngr::Instance().Get("StdPipeline/ATrous");
	rtWithBRDFLUTShader = ShaderMngr::Instance().Get("StdPipeline/RTwithBRDFLUT");
	rtReprojectShader = ShaderMngr::Instance().Get("StdPipeline/RTRepeoject");
	taaShader = ShaderMngr::Instance().Get("StdPipeline/TAA");
	filterMomentsShader = ShaderMngr::Instance().Get("StdPipeline/FilterMoments");

	vecf3 origin[6] = {
		{ 1,-1, 1}, // +x right
		{-1,-1,-1}, // -x left
		{-1, 1, 1}, // +y top
		{-1,-1,-1}, // -y buttom
		{-1,-1, 1}, // +z front
		{ 1,-1,-1}, // -z back
	};

	vecf3 right[6] = {
		{ 0, 0,-2}, // +x
		{ 0, 0, 2}, // -x
		{ 2, 0, 0}, // +y
		{ 2, 0, 0}, // -y
		{ 2, 0, 0}, // +z
		{-2, 0, 0}, // -z
	};

	vecf3 up[6] = {
		{ 0, 2, 0}, // +x
		{ 0, 2, 0}, // -x
		{ 0, 0,-2}, // +y
		{ 0, 0, 2}, // -y
		{ 0, 2, 0}, // +z
		{ 0, 2, 0}, // -z
	};

	auto buffer = crossFrameShaderCBMngr.GetCommonBuffer();
	constexpr auto quadPositionsSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
	constexpr auto mipInfoSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
	buffer->FastReserve(6 * quadPositionsSize + IBLData::PreFilterMapMipLevels * mipInfoSize);
	for (size_t i = 0; i < 6; i++) {
		QuadPositionLs positionLs;
		auto p0 = origin[i];
		auto p1 = origin[i] + right[i];
		auto p2 = origin[i] + right[i] + up[i];
		auto p3 = origin[i] + up[i];
		positionLs.positionL4x = { p0[0], p1[0], p2[0], p3[0] };
		positionLs.positionL4y = { p0[1], p1[1], p2[1], p3[1] };
		positionLs.positionL4z = { p0[2], p1[2], p2[2], p3[2] };
		buffer->Set(i * quadPositionsSize, &positionLs, sizeof(QuadPositionLs));
	}
	size_t size = IBLData::PreFilterMapSize;
	for (UINT i = 0; i < IBLData::PreFilterMapMipLevels; i++) {
		MipInfo info;
		info.roughness = i / float(IBLData::PreFilterMapMipLevels - 1);
		info.resolution = (float)size;

		buffer->Set(6 * quadPositionsSize + i * mipInfoSize, &info, sizeof(MipInfo));
		size /= 2;
	}
}

void StdDXRPipeline::Impl::UpdateRenderContext(
	const std::vector<const UECS::World*>& worlds,
	size_t width, size_t height,
	const CameraData& cameraData
) {
	renderContext.renderQueue.Clear();
	renderContext.entity2data.clear();
	renderContext.entity2offset.clear();
	renderContext.shaderCBDescMap.clear();

	if (cameraData.entity.Valid()) { // camera
		CameraConstants newcameraConstants;
		auto cmptCamera = cameraData.world.entityMngr.ReadComponent<Camera>(cameraData.entity);
		auto cmptW2L = cameraData.world.entityMngr.ReadComponent<WorldToLocal>(cameraData.entity);
		auto cmptTranslation = cameraData.world.entityMngr.ReadComponent<Translation>(cameraData.entity);

		newcameraConstants.View = cmptW2L->value;
		newcameraConstants.InvView = newcameraConstants.View.inverse();
		newcameraConstants.Proj = cmptCamera->prjectionMatrix;
		newcameraConstants.InvProj = newcameraConstants.Proj.inverse();
		newcameraConstants.ViewProj = newcameraConstants.Proj * newcameraConstants.View;
		newcameraConstants.InvViewProj = newcameraConstants.InvView * newcameraConstants.InvProj;
		newcameraConstants.PrevViewProj = renderContext.cameraConstants.ViewProj;
		newcameraConstants.EyePosW = cmptTranslation->value.as<pointf3>();
		newcameraConstants.RenderTargetSize = { width, height };
		newcameraConstants.InvRenderTargetSize = { 1.0f / width, 1.0f / height };

		newcameraConstants.NearZ = cmptCamera->clippingPlaneMin;
		newcameraConstants.FarZ = cmptCamera->clippingPlaneMax;
		newcameraConstants.TotalTime = GameTimer::Instance().TotalTime();
		newcameraConstants.DeltaTime = GameTimer::Instance().DeltaTime();

		newcameraConstants.FrameCount = renderContext.cameraConstants.FrameCount + 1;

		renderContext.cameraChange = renderContext.cameraConstants.ViewProj != newcameraConstants.ViewProj
			|| renderContext.cameraConstants.RenderTargetSize != newcameraConstants.RenderTargetSize;

		renderContext.cameraConstants = newcameraConstants;
	}

	{ // object
		ArchetypeFilter filter;
		filter.all = {
			AccessTypeID_of<Latest<MeshFilter>>,
			AccessTypeID_of<Latest<MeshRenderer>>,
			AccessTypeID_of<Latest<LocalToWorld>>,
		};
		for (auto world : worlds) {
			world->RunChunkJob(
				[&](ChunkView chunk) {
					auto meshFilters = chunk->GetCmptArray<MeshFilter>();
					auto meshRenderers = chunk->GetCmptArray<MeshRenderer>();
					auto L2Ws = chunk->GetCmptArray<LocalToWorld>();
					auto W2Ls = chunk->GetCmptArray<WorldToLocal>();
					auto entities = chunk->GetEntityArray();

					size_t N = chunk->EntityNum();

					for (size_t i = 0; i < N; i++) {
						auto meshFilter = meshFilters[i];
						auto meshRenderer = meshRenderers[i];

						if (!meshFilter.mesh)
							return;

						RenderObject obj;
						obj.mesh = meshFilter.mesh;
						obj.entity = entities[i];

						size_t M = obj.mesh->GetSubMeshes().size();// std::min(meshRenderer.materials.size(), obj.mesh->GetSubMeshes().size());

						if (M == 0)
							continue;

						bool isDraw = false;

						for (size_t j = 0; j < M; j++) {
							auto material = j < meshRenderer.materials.size() ? meshRenderer.materials[j] : nullptr;
							if (!material || !material->shader)
								obj.material = errorMat;
							else {
								if (material->shader->passes.empty())
									continue;

								obj.material = material;
							}

							obj.translation = L2Ws[i].value.decompose_translation();
							obj.submeshIdx = j;
							for (size_t k = 0; k < obj.material->shader->passes.size(); k++) {
								obj.passIdx = k;
								renderContext.renderQueue.Add(obj);
							}
							isDraw = true;
						}

						if (!isDraw)
							continue;

						auto target = renderContext.entity2data.find(obj.entity.index);
						if (target != renderContext.entity2data.end())
							continue;
						RenderContext::EntityData data;
						data.l2w = L2Ws[i].value;
						data.w2l = !W2Ls.empty() ? W2Ls[i].value : L2Ws[i].value.inverse();
						data.mesh = meshFilter.mesh;
						data.materials = meshRenderer.materials;
						renderContext.entity2data.emplace(obj.entity.index, std::move(data));
					}
				},
				filter,
				false
			);
		}
		renderContext.renderQueue.Sort(renderContext.cameraConstants.EyePosW);
	}

	{ // light
		std::array<ShaderLight, LightArray::size> dirLights;
		std::array<ShaderLight, LightArray::size> pointLights;
		std::array<ShaderLight, LightArray::size> spotLights;
		std::array<ShaderLight, LightArray::size> rectLights;
		std::array<ShaderLight, LightArray::size> diskLights;
		renderContext.lights.diectionalLightNum = 0;
		renderContext.lights.pointLightNum = 0;
		renderContext.lights.spotLightNum = 0;
		renderContext.lights.rectLightNum = 0;
		renderContext.lights.diskLightNum = 0;

		UECS::ArchetypeFilter filter;
		filter.all = { UECS::AccessTypeID_of<UECS::Latest<LocalToWorld>> };
		for (auto world : worlds) {
			world->RunEntityJob(
				[&](const Light* light) {
					switch (light->mode)
					{
					case Light::Mode::Directional:
						renderContext.lights.diectionalLightNum++;
						break;
					case Light::Mode::Point:
						renderContext.lights.pointLightNum++;
						break;
					case Light::Mode::Spot:
						renderContext.lights.spotLightNum++;
						break;
					case Light::Mode::Rect:
						renderContext.lights.rectLightNum++;
						break;
					case Light::Mode::Disk:
						renderContext.lights.diskLightNum++;
						break;
					default:
						assert("not support" && false);
						break;
					}
				},
				false,
					filter
					);
		}

		size_t offset_diectionalLight = 0;
		size_t offset_pointLight = offset_diectionalLight + renderContext.lights.diectionalLightNum;
		size_t offset_spotLight = offset_pointLight + renderContext.lights.pointLightNum;
		size_t offset_rectLight = offset_spotLight + renderContext.lights.spotLightNum;
		size_t offset_diskLight = offset_rectLight + renderContext.lights.rectLightNum;
		size_t cur_diectionalLight = 0;
		size_t cur_pointLight = 0;
		size_t cur_spotLight = 0;
		size_t cur_rectLight = 0;
		size_t cur_diskLight = 0;
		for (auto world : worlds) {
			world->RunEntityJob(
				[&](const Light* light, const LocalToWorld* l2w) {
					switch (light->mode)
					{
					case Light::Mode::Directional:
						renderContext.lights.lights[cur_diectionalLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_diectionalLight].dir = (l2w->value * vecf3{ 0,0,1 }).safe_normalize();
						cur_diectionalLight++;
						break;
					case Light::Mode::Point:
						renderContext.lights.lights[cur_pointLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_pointLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_pointLight].range = light->range;
						cur_pointLight++;
						break;
					case Light::Mode::Spot:
						renderContext.lights.lights[cur_spotLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_spotLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_spotLight].dir = (l2w->value * vecf3{ 0,1,0 }).safe_normalize();
						renderContext.lights.lights[cur_spotLight].range = light->range;
						renderContext.lights.lights[cur_spotLight].*
							ShaderLight::Spot::pCosHalfInnerSpotAngle = std::cos(to_radian(light->innerSpotAngle) / 2.f);
						renderContext.lights.lights[cur_spotLight].*
							ShaderLight::Spot::pCosHalfOuterSpotAngle = std::cos(to_radian(light->outerSpotAngle) / 2.f);
						cur_spotLight++;
						break;
					case Light::Mode::Rect:
						renderContext.lights.lights[cur_rectLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_rectLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_rectLight].dir = (l2w->value * vecf3{ 0,1,0 }).safe_normalize();
						renderContext.lights.lights[cur_rectLight].horizontal = (l2w->value * vecf3{ 1,0,0 }).safe_normalize();
						renderContext.lights.lights[cur_rectLight].range = light->range;
						renderContext.lights.lights[cur_rectLight].*
							ShaderLight::Rect::pWidth = light->width;
						renderContext.lights.lights[cur_rectLight].*
							ShaderLight::Rect::pHeight = light->height;
						cur_rectLight++;
						break;
					case Light::Mode::Disk:
						renderContext.lights.lights[cur_diskLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_diskLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_diskLight].dir = (l2w->value * vecf3{ 0,1,0 }).safe_normalize();
						renderContext.lights.lights[cur_diskLight].horizontal = (l2w->value * vecf3{ 1,0,0 }).safe_normalize();
						renderContext.lights.lights[cur_diskLight].range = light->range;
						renderContext.lights.lights[cur_diskLight].*
							ShaderLight::Disk::pWidth = light->width;
						renderContext.lights.lights[cur_diskLight].*
							ShaderLight::Disk::pHeight = light->height;
						cur_diskLight++;
						break;
					default:
						break;
					}
				},
				false
			);
		}
	}

	// use first skybox in the world vector
	renderContext.skybox = defaultSkybox;
	for (auto world : worlds) {
		if (auto ptr = world->entityMngr.ReadSingleton<Skybox>(); ptr && ptr->material && ptr->material->shader.get() == skyboxShader.get()) {
			auto target = ptr->material->properties.find("gSkybox");
			if (target != ptr->material->properties.end()
				&& std::holds_alternative<SharedVar<TextureCube>>(target->second.value)
				) {
				auto texcube = std::get<SharedVar<TextureCube>>(target->second.value);
				renderContext.skybox = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*texcube);
				break;
			}
		}
	}
}

void StdDXRPipeline::Impl::UpdateShaderCBs() {
	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");

	{ // camera, lights, objects
		auto buffer = shaderCBMngr->GetCommonBuffer();
		buffer->FastReserve(
			UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants))
			+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray))
			+ renderContext.entity2data.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants))
		);

		buffer->Set(renderContext.cameraOffset, &renderContext.cameraConstants, sizeof(CameraConstants));

		// light array
		buffer->Set(renderContext.lightOffset, &renderContext.lights, sizeof(LightArray));

		// objects
		size_t offset = renderContext.objectOffset;
		for (auto& [idx, data] : renderContext.entity2data) {
			ObjectConstants objectConstants;
			objectConstants.World = data.l2w;
			objectConstants.InvWorld = data.w2l;
			renderContext.entity2offset[idx] = offset;
			buffer->Set(offset, &objectConstants, sizeof(ObjectConstants));
			offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		}
	}

	std::unordered_map<const Shader*, std::unordered_set<const Material*>> opaqueMaterialMap;
	for (const auto& opaque : renderContext.renderQueue.GetOpaques())
		opaqueMaterialMap[opaque.material->shader.get()].insert(opaque.material.get());

	std::unordered_map<const Shader*, std::unordered_set<const Material*>> transparentMaterialMap;
	for (const auto& transparent : renderContext.renderQueue.GetTransparents())
		transparentMaterialMap[transparent.material->shader.get()].insert(transparent.material.get());

	for (const auto& [shader, materials] : opaqueMaterialMap) {
		renderContext.shaderCBDescMap[shader] =
			PipelineBase::UpdateShaderCBs(*shaderCBMngr, *shader, materials, commonCBs);
	}

	for (const auto& [shader, materials] : transparentMaterialMap) {
		renderContext.shaderCBDescMap[shader] =
			PipelineBase::UpdateShaderCBs(*shaderCBMngr, *shader, materials, commonCBs);
	}
}

void StdDXRPipeline::Impl::Render(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData, ID3D12Resource* rtb) {
	assert(rtb);

	size_t width = rtb->GetDesc().Width;
	size_t height = rtb->GetDesc().Height;
	CD3DX12_VIEWPORT viewport(0.f, 0.f, width, height);
	D3D12_RECT scissorRect(0.f, 0.f, width, height);

	// collect some cpu data
	UpdateRenderContext(worlds, width, height, cameraData);

	// Cycle through the circular frame resource array.
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	frameRsrcMngr.BeginFrame();

	// cpu -> gpu
	UpdateShaderCBs();

	auto cmdAlloc = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
	cmdAlloc->Reset();

	auto tlasBuffers = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<TLASData>>("TLASData");
	{ // tlas buffers reverse
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		inputs.NumDescs = (UINT)renderContext.entity2data.size(); // upper bound
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		inputs.InstanceDescs = tlasBuffers->instanceDescs.GetResource() ? tlasBuffers->instanceDescs.GetResource()->GetGPUVirtualAddress() : 0;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		dxr_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
		tlasBuffers->Reserve(initDesc.device, info, renderContext.renderQueue.GetOpaques().size());
	}

	fg.Clear();
	auto fgRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");
	fgRsrcMngr->NewFrame();

	Resize(width, height);

	fgExecutor.NewFrame();

	auto rtSRsrc = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12Resource>>("rtSRsrc");
	auto rtURsrc = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12Resource>>("rtURsrc");
	auto rtColorMoment = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12Resource>>("rtColorMoment");
	auto rtHistoryLength = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12Resource>>("rtHistoryLength");
	auto rtLinearZ = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12Resource>>("rtLinearZ");
	assert(rtSRsrc);
	assert(rtURsrc);
	assert(rtColorMoment);
	assert(rtHistoryLength);
	assert(rtLinearZ);
	auto taaPrevRsrc = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12Resource>>("taaPrevRsrc");

	auto gbuffer0 = fg.RegisterResourceNode("GBuffer0");
	auto gbuffer1 = fg.RegisterResourceNode("GBuffer1");
	auto gbuffer2 = fg.RegisterResourceNode("GBuffer2");
	auto linearZ = fg.RegisterResourceNode("LinearZ");
	auto motion = fg.RegisterResourceNode("Motion");
	auto deferLightedRT = fg.RegisterResourceNode("Defer Lighted");
	auto deferLightedSkyRT = fg.RegisterResourceNode("Defer Lighted with Sky");
	auto sceneRT = fg.RegisterResourceNode("Scene");
	auto presentedRT = fg.RegisterResourceNode("Present");
	auto deferDS = fg.RegisterResourceNode("Defer Depth Stencil");
	auto forwardDS = fg.RegisterResourceNode("Forward Depth Stencil");
	auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
	auto prefilterMap = fg.RegisterResourceNode("PreFilter Map");
	auto TLAS = fg.RegisterResourceNode("TLAS");
	auto historyLength = fg.RegisterResourceNode("History Length");
	auto prevRTResult = fg.RegisterResourceNode("Prev RT result");
	auto prevRTUResult = fg.RegisterResourceNode("Prev RT U result");
	auto prevColorMoment = fg.RegisterResourceNode("Prev Color Moment");
	auto prevLinearZ = fg.RegisterResourceNode("Prev LinearZ");
	auto rtResult = fg.RegisterResourceNode("RT result");
	auto rtUResult = fg.RegisterResourceNode("RT U result");
	auto accRTResult = fg.RegisterResourceNode("Acc RT result");
	auto accRTUResult = fg.RegisterResourceNode("Acc RT U result");
	auto fAccRTResult = fg.RegisterResourceNode("Filtered Acc RT result");
	auto fAccRTUResult = fg.RegisterResourceNode("Filtered Acc RT U result");
	auto accRTResult_copyrt = fg.RegisterResourceNode("Acc RT result Copy Target");
	auto accRTUResult_copyrt = fg.RegisterResourceNode("Acc RT U result Copy Target");
	auto colorMoment_copyrt = fg.RegisterResourceNode("Color Moment Copy Target");
	auto linearZ_copyrt = fg.RegisterResourceNode("LinearZ Copy Target");
	auto colorMoment = fg.RegisterResourceNode("Color Moment");
	auto taaPrevResult = fg.RegisterResourceNode("TAA Prev Result");
	auto taaResult = fg.RegisterResourceNode("TAA Result");
	auto taaResult_copyrt = fg.RegisterResourceNode("TAA Result Copy Target");
	constexpr std::size_t ATrousN = 5;
	std::array<std::size_t, ATrousN> rtATrousDenoiseResults;
	for (std::size_t i = 0; i < ATrousN; i++)
		rtATrousDenoiseResults[i] = fg.RegisterResourceNode("RT ATrous denoised result " + std::to_string(i));
	std::array<std::size_t, ATrousN> rtUATrousDenoiseResults;
	for (std::size_t i = 0; i < ATrousN; i++)
		rtUATrousDenoiseResults[i] = fg.RegisterResourceNode("RT U ATrous denoised result " + std::to_string(i));
	auto rtWithBRDFLUT = fg.RegisterResourceNode("RT with BRDFLUT");
	auto rtDenoisedResult = fg.RegisterResourceNode("RT Denoised Result");
	auto rtUDenoisedResult = fg.RegisterResourceNode("RT U Denoised Result");

	auto gbPass = fg.RegisterPassNode(
		"GBuffer Pass",
		{},
		{ gbuffer0,gbuffer1,gbuffer2,linearZ,motion,deferDS }
	);
	auto tlasPass = fg.RegisterPassNode(
		"TLAS Pass",
		{},
		{ TLAS }
	);
	auto rtPass = fg.RegisterPassNode(
		"RT Pass",
		{ gbuffer0,gbuffer1,gbuffer2,deferDS,TLAS },
		{ rtResult, rtUResult }
	);
	fg.RegisterMoveNode(accRTResult, rtResult);
	fg.RegisterMoveNode(accRTUResult, rtUResult);
	auto reprojectPass = fg.RegisterPassNode(
		"RTReproject Pass",
		{ prevRTResult, prevRTUResult, prevColorMoment, motion, prevLinearZ, linearZ },
		{ accRTResult, accRTUResult, colorMoment, historyLength }
	);
	auto filterMomentsPass = fg.RegisterPassNode(
		"Filter Moments Pass",
		{ accRTResult, accRTUResult, colorMoment, historyLength, gbuffer1, linearZ },
		{ fAccRTResult, fAccRTUResult }
	);
	auto iblPass = fg.RegisterPassNode(
		"IBL",
		{ },
		{ irradianceMap, prefilterMap }
	);
	auto deferLightingPass = fg.RegisterPassNode(
		"Defer Lighting",
		{ gbuffer0,gbuffer1,gbuffer2,deferDS,irradianceMap,prefilterMap },
		{ deferLightedRT }
	);
	std::array<std::size_t, ATrousN> rtATrousDenoisePasses;
	for (std::size_t i = 0; i < ATrousN; i++) {
		std::size_t input_node_S = i == 0 ? fAccRTResult : rtATrousDenoiseResults[i - 1];
		std::size_t input_node_U = i == 0 ? fAccRTUResult : rtUATrousDenoiseResults[i - 1];
		rtATrousDenoisePasses[i] = fg.RegisterPassNode(
			"RT ATrous Denoise Pass" + std::to_string(i),
			{ input_node_S, input_node_U, gbuffer1, linearZ },
			{ rtATrousDenoiseResults[i], rtUATrousDenoiseResults[i] }
		);
	}
	fg.RegisterMoveNode(linearZ_copyrt, prevLinearZ);
	fg.RegisterMoveNode(accRTResult_copyrt, prevRTResult);
	fg.RegisterMoveNode(accRTUResult_copyrt, prevRTUResult);
	fg.RegisterMoveNode(colorMoment_copyrt, prevColorMoment);
	auto copyPass = fg.RegisterPassNode(
		"RT Copy Pass",
		{ linearZ, rtATrousDenoiseResults.front(), rtUATrousDenoiseResults.front(), colorMoment },
		{ linearZ_copyrt, accRTResult_copyrt, accRTUResult_copyrt, colorMoment_copyrt }
	);
	fg.RegisterMoveNode(rtDenoisedResult, rtATrousDenoiseResults.back());
	fg.RegisterMoveNode(rtUDenoisedResult, rtUATrousDenoiseResults.back());
	auto rtWithBRDFLUTPass = fg.RegisterPassNode(
		"RT with BRDFLUT Pass",
		{ gbuffer0,gbuffer1,gbuffer2,deferDS,rtDenoisedResult,rtUDenoisedResult },
		{ rtWithBRDFLUT }
	);
	fg.RegisterMoveNode(deferLightedSkyRT, rtWithBRDFLUT);
	auto skyboxPass = fg.RegisterPassNode(
		"Skybox",
		{ deferDS },
		{ deferLightedSkyRT }
	);
	fg.RegisterMoveNode(forwardDS, deferDS);
	fg.RegisterMoveNode(sceneRT, deferLightedSkyRT);
	auto forwardPass = fg.RegisterPassNode(
		"Forward",
		{ irradianceMap, prefilterMap },
		{ forwardDS, sceneRT }
	);
	auto taaPass = fg.RegisterPassNode(
		"TAA Pass",
		{ taaPrevResult, sceneRT, motion },
		{ taaResult }
	);
	fg.RegisterMoveNode(taaResult_copyrt, taaPrevResult);
	auto taaCopyPass = fg.RegisterPassNode(
		"TAA Copy Pass",
		{ taaResult },
		{ taaResult_copyrt }
	);
	auto postprocessPass = fg.RegisterPassNode(
		"Post Process",
		{ taaResult },
		{ presentedRT }
	);

	D3D12_RESOURCE_DESC dsDesc = UDX12::Desc::RSRC::Basic(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		width,
		(UINT)height,
		DXGI_FORMAT_R24G8_TYPELESS,
		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);
	D3D12_CLEAR_VALUE dsClear;
	dsClear.Format = dsFormat;
	dsClear.DepthStencil.Depth = 1.0f;
	dsClear.DepthStencil.Stencil = 0;

	auto srvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto dsSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	auto dsvDesc = UDX12::Desc::DSV::Basic(dsFormat);
	auto rt2dDesc = UDX12::Desc::RSRC::RT2D(width, (UINT)height, DXGI_FORMAT_R32G32B32A32_FLOAT);
	const UDX12::FG::RsrcImplDesc_RTV_Null rtvNull;

	D3D12_RESOURCE_DESC rtResultDesc = {};
	{ // fill rtResultType
		rtResultDesc.DepthOrArraySize = 1;
		rtResultDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		rtResultDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		rtResultDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		rtResultDesc.Width = width;
		rtResultDesc.Height = (UINT)height;
		rtResultDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		rtResultDesc.MipLevels = 1;
		rtResultDesc.SampleDesc.Count = 1;
	}
	D3D12_UNORDERED_ACCESS_VIEW_DESC tex2dUAVDesc = {};
	{
		tex2dUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc = {};
	{ // fill tlasSrvDesc
		tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		tlasSrvDesc.RaytracingAccelerationStructure.Location = tlasBuffers->pResult->GetGPUVirtualAddress();
	}

	auto iblData = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<IBLData>>("IBL data");
	D3D12_CLEAR_VALUE gbuffer_clearvalue = {};
	gbuffer_clearvalue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gbuffer_clearvalue.Color[0] = 0;
	gbuffer_clearvalue.Color[1] = 0;
	gbuffer_clearvalue.Color[2] = 0;
	gbuffer_clearvalue.Color[3] = 0;

	for (std::size_t i = 0; i < ATrousN; i++) {
		fgRsrcMngr->RegisterTemporalRsrcAutoClear(rtATrousDenoiseResults[i], rt2dDesc);
		fgRsrcMngr->RegisterTemporalRsrcAutoClear(rtUATrousDenoiseResults[i], rt2dDesc);

		// rts, rtu
		fgRsrcMngr->RegisterRsrcTable({
			{fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[0], srvDesc},
			{fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[1], srvDesc}
		});

		fgRsrcMngr->RegisterPassRsrc(rtATrousDenoisePasses[i],
			fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[0],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc);
		fgRsrcMngr->RegisterPassRsrc(rtATrousDenoisePasses[i],
			fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[1],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc);

		fgRsrcMngr->RegisterPassRsrc(rtATrousDenoisePasses[i], gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc);
		fgRsrcMngr->RegisterPassRsrc(rtATrousDenoisePasses[i], linearZ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc);

		fgRsrcMngr->RegisterPassRsrc(rtATrousDenoisePasses[i],
			fg.GetPassNodes()[rtATrousDenoisePasses[i]].Outputs()[0],
			D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull);
		fgRsrcMngr->RegisterPassRsrc(rtATrousDenoisePasses[i],
			fg.GetPassNodes()[rtATrousDenoisePasses[i]].Outputs()[1],
			D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull);
	}

	(*fgRsrcMngr)
		.RegisterTemporalRsrc(gbuffer0, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(gbuffer1, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(gbuffer2, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(linearZ, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(motion, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(deferDS, dsDesc, dsClear)
		.RegisterTemporalRsrcAutoClear(deferLightedRT, rt2dDesc)
		.RegisterTemporalRsrc(rtResult, rtResultDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(rtUResult, rtResultDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(fAccRTResult, rtResultDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(fAccRTUResult, rtResultDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(colorMoment, rtResultDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrcAutoClear(rtWithBRDFLUT, rt2dDesc)
		.RegisterTemporalRsrcAutoClear(taaResult, rt2dDesc)

		.RegisterImportedRsrc(prevLinearZ, { rtLinearZ.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prevRTResult, { rtSRsrc.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prevRTUResult, { rtURsrc.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prevColorMoment, { rtColorMoment.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(historyLength, { rtHistoryLength.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS })

		.RegisterImportedRsrc(presentedRT, { rtb, D3D12_RESOURCE_STATE_PRESENT })
		.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(TLAS, { nullptr /*special for TLAS*/, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE })
		.RegisterImportedRsrc(taaPrevResult, { taaPrevRsrc.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })

		.RegisterPassRsrc(gbPass, gbuffer0, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, gbuffer1, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, gbuffer2, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, linearZ, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, motion, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, deferDS, D3D12_RESOURCE_STATE_DEPTH_WRITE, dsvDesc)

		.RegisterPassRsrcState(iblPass, irradianceMap, D3D12_RESOURCE_STATE_RENDER_TARGET)
		.RegisterPassRsrcState(iblPass, prefilterMap, D3D12_RESOURCE_STATE_RENDER_TARGET)

		.RegisterRsrcTable({
			{gbuffer0,srvDesc},
			{gbuffer1,srvDesc},
			{gbuffer2,srvDesc}
		})
		.RegisterPassRsrc(deferLightingPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(deferLightingPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(deferLightingPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrcState(deferLightingPass, deferDS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ)
		.RegisterPassRsrcImplDesc(deferLightingPass, deferDS, dsvDesc)
		.RegisterPassRsrcImplDesc(deferLightingPass, deferDS, dsSrvDesc)
		.RegisterPassRsrcState(deferLightingPass, irradianceMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrcState(deferLightingPass, prefilterMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrc(deferLightingPass, deferLightedRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrc(skyboxPass, deferDS, D3D12_RESOURCE_STATE_DEPTH_READ, dsvDesc)
		.RegisterPassRsrc(skyboxPass, deferLightedSkyRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrcState(tlasPass, TLAS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)

		.RegisterRsrcHandle(rtResult, tex2dUAVDesc, tlasBuffers->raygenDescriptors.GetCpuHandle(0), tlasBuffers->raygenDescriptors.GetGpuHandle(0), false)
		.RegisterRsrcHandle(rtUResult, tex2dUAVDesc, tlasBuffers->raygenDescriptors.GetCpuHandle(1), tlasBuffers->raygenDescriptors.GetGpuHandle(1), false)

		.RegisterPassRsrc(rtPass, rtResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(rtPass, rtUResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(rtPass, TLAS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, tlasSrvDesc)
		.RegisterPassRsrc(rtPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtPass, deferDS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ, dsSrvDesc)

		.RegisterRsrcTable({
			{ prevRTResult, srvDesc },
			{ prevRTUResult, srvDesc },
			{ prevColorMoment, srvDesc },
			{ motion, srvDesc },
			{ prevLinearZ, srvDesc },
			{ linearZ, srvDesc },
		})
		.RegisterRsrcTable({
			{ accRTResult, tex2dUAVDesc },
			{ accRTUResult, tex2dUAVDesc },
			{ colorMoment, tex2dUAVDesc },
			{ historyLength, tex2dUAVDesc },
		})
		.RegisterPassRsrc(reprojectPass, prevLinearZ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(reprojectPass, linearZ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(reprojectPass, prevRTResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(reprojectPass, prevRTUResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(reprojectPass, prevColorMoment, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(reprojectPass, motion, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(reprojectPass, accRTResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(reprojectPass, accRTUResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(reprojectPass, colorMoment, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(reprojectPass, historyLength, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		
		.RegisterRsrcTable({
			{ accRTResult, srvDesc },
			{ accRTUResult, srvDesc },
			{ colorMoment, srvDesc },
			{ historyLength, UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32_UINT) },
		})
		.RegisterPassRsrc(filterMomentsPass, accRTResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, accRTUResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, colorMoment, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, historyLength, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32_UINT))
		.RegisterPassRsrc(filterMomentsPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, linearZ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, fAccRTResult, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(filterMomentsPass, fAccRTUResult, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrcState(copyPass, linearZ, D3D12_RESOURCE_STATE_COPY_SOURCE)
		.RegisterPassRsrcState(copyPass, rtATrousDenoiseResults.front(), D3D12_RESOURCE_STATE_COPY_SOURCE)
		.RegisterPassRsrcState(copyPass, rtUATrousDenoiseResults.front(), D3D12_RESOURCE_STATE_COPY_SOURCE)
		.RegisterPassRsrcState(copyPass, colorMoment, D3D12_RESOURCE_STATE_COPY_SOURCE)
		.RegisterPassRsrcState(copyPass, linearZ_copyrt, D3D12_RESOURCE_STATE_COPY_DEST)
		.RegisterPassRsrcState(copyPass, accRTResult_copyrt, D3D12_RESOURCE_STATE_COPY_DEST)
		.RegisterPassRsrcState(copyPass, accRTUResult_copyrt, D3D12_RESOURCE_STATE_COPY_DEST)
		.RegisterPassRsrcState(copyPass, colorMoment_copyrt, D3D12_RESOURCE_STATE_COPY_DEST)

		.RegisterRsrcTable({
			{rtDenoisedResult,srvDesc},
			{rtUDenoisedResult,srvDesc}
		})

		.RegisterPassRsrc(rtWithBRDFLUTPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, deferDS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ, dsSrvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, rtDenoisedResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, rtUDenoisedResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, rtWithBRDFLUT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrcState(forwardPass, irradianceMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrcState(forwardPass, prefilterMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrc(forwardPass, forwardDS, D3D12_RESOURCE_STATE_DEPTH_WRITE, dsvDesc)
		.RegisterPassRsrc(forwardPass, sceneRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
			
		.RegisterRsrcTable({
			{ taaPrevResult,srvDesc },
			{ sceneRT,srvDesc },
		})
		.RegisterPassRsrc(taaPass, taaPrevResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(taaPass, sceneRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(taaPass, motion, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(taaPass, taaResult, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrcState(taaCopyPass, taaResult, D3D12_RESOURCE_STATE_COPY_SOURCE)
		.RegisterPassRsrcState(taaCopyPass, taaResult_copyrt, D3D12_RESOURCE_STATE_COPY_DEST)

		.RegisterPassRsrc(postprocessPass, taaResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(postprocessPass, presentedRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		;

	fgExecutor.RegisterPassFunc(
		gbPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto gb0 = rsrcs.at(gbuffer0);
			auto gb1 = rsrcs.at(gbuffer1);
			auto gb2 = rsrcs.at(gbuffer2);
			auto lZ = rsrcs.at(linearZ);
			auto mt = rsrcs.at(motion);
			auto ds = rsrcs.at(deferDS);

			std::array rtHandles{
				gb0.info.null_info_rtv.cpuHandle,
				gb1.info.null_info_rtv.cpuHandle,
				gb2.info.null_info_rtv.cpuHandle,
				lZ.info.null_info_rtv.cpuHandle,
				mt.info.null_info_rtv.cpuHandle,
			};
			auto dsHandle = ds.info.desc2info_dsv.at(dsvDesc).cpuHandle;
			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rtHandles[0], gbuffer_clearvalue.Color, 0, nullptr);
			cmdList->ClearRenderTargetView(rtHandles[1], gbuffer_clearvalue.Color, 0, nullptr);
			cmdList->ClearRenderTargetView(rtHandles[2], gbuffer_clearvalue.Color, 0, nullptr);
			cmdList->ClearRenderTargetView(rtHandles[3], gbuffer_clearvalue.Color, 0, nullptr);
			cmdList->ClearRenderTargetView(rtHandles[4], gbuffer_clearvalue.Color, 0, nullptr);
			cmdList->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets((UINT)rtHandles.size(), rtHandles.data(), false, &dsHandle);

			DrawObjects(cmdList, "RTDeferred", 5, DXGI_FORMAT_R32G32B32A32_FLOAT);
		}
	);

	fgExecutor.RegisterPassFunc(
		iblPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& /*rsrcs*/) {
			if (iblData->lastSkybox.ptr == renderContext.skybox.ptr) {
				if (iblData->nextIdx >= 6 * (1 + IBLData::PreFilterMapMipLevels))
					return;
			}
			else {
				if (renderContext.skybox.ptr == defaultSkybox.ptr) {
					iblData->lastSkybox.ptr = defaultSkybox.ptr;
					return;
				}
				iblData->lastSkybox = renderContext.skybox;
				iblData->nextIdx = 0;
			}

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			if (iblData->nextIdx < 6) { // irradiance
				cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*irradianceShader));
				cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
					*irradianceShader,
					0,
					static_cast<size_t>(-1),
					1,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					DXGI_FORMAT_UNKNOWN)
				);

				D3D12_VIEWPORT viewport;
				viewport.MinDepth = 0.f;
				viewport.MaxDepth = 1.f;
				viewport.TopLeftX = 0.f;
				viewport.TopLeftY = 0.f;
				viewport.Width = Impl::IBLData::IrradianceMapSize;
				viewport.Height = Impl::IBLData::IrradianceMapSize;
				D3D12_RECT rect = { 0,0,Impl::IBLData::IrradianceMapSize,Impl::IBLData::IrradianceMapSize };
				cmdList->RSSetViewports(1, &viewport);
				cmdList->RSSetScissorRects(1, &rect);

				auto buffer = crossFrameShaderCBMngr.GetCommonBuffer();

				cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);
				//for (UINT i = 0; i < 6; i++) {
				UINT i = static_cast<UINT>(iblData->nextIdx);
				// Specify the buffers we are going to render to.
				const auto iblRTVHandle = iblData->RTVsDH.GetCpuHandle(i);
				cmdList->OMSetRenderTargets(1, &iblRTVHandle, false, nullptr);
				auto address = buffer->GetResource()->GetGPUVirtualAddress()
					+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
				cmdList->SetGraphicsRootConstantBufferView(1, address);

				cmdList->IASetVertexBuffers(0, 0, nullptr);
				cmdList->IASetIndexBuffer(nullptr);
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				cmdList->DrawInstanced(6, 1, 0, 0);
				//}
			}
			else { // prefilter
				cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*prefilterShader));
				cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
					*prefilterShader,
					0,
					static_cast<size_t>(-1),
					1,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					DXGI_FORMAT_UNKNOWN)
				);

				auto buffer = crossFrameShaderCBMngr.GetCommonBuffer();

				cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);
				//size_t size = Impl::IBLData::PreFilterMapSize;
				//for (UINT mip = 0; mip < Impl::IBLData::PreFilterMapMipLevels; mip++) {
				UINT mip = static_cast<UINT>((iblData->nextIdx - 6) / 6);
				size_t size = Impl::IBLData::PreFilterMapSize >> mip;
				auto mipinfo = buffer->GetResource()->GetGPUVirtualAddress()
					+ 6 * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs))
					+ mip * UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
				cmdList->SetGraphicsRootConstantBufferView(2, mipinfo);

				D3D12_VIEWPORT viewport;
				viewport.MinDepth = 0.f;
				viewport.MaxDepth = 1.f;
				viewport.TopLeftX = 0.f;
				viewport.TopLeftY = 0.f;
				viewport.Width = (float)size;
				viewport.Height = (float)size;
				D3D12_RECT rect = { 0,0,(LONG)size,(LONG)size };
				cmdList->RSSetViewports(1, &viewport);
				cmdList->RSSetScissorRects(1, &rect);

				//for (UINT i = 0; i < 6; i++) {
				UINT i = iblData->nextIdx % 6;
				auto positionLs = buffer->GetResource()->GetGPUVirtualAddress()
					+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
				cmdList->SetGraphicsRootConstantBufferView(1, positionLs);

				// Specify the buffers we are going to render to.
				const auto iblRTVHandle = iblData->RTVsDH.GetCpuHandle(6 * (1 + mip) + i);
				cmdList->OMSetRenderTargets(1, &iblRTVHandle, false, nullptr);

				cmdList->IASetVertexBuffers(0, 0, nullptr);
				cmdList->IASetIndexBuffer(nullptr);
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				cmdList->DrawInstanced(6, 1, 0, 0);
				//}

				size /= 2;
				//}
			}

			iblData->nextIdx++;
		}
	);

	fgExecutor.RegisterPassFunc(
		deferLightingPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto gb0 = rsrcs.at(gbuffer0); // table {gb0, gb1, gb2}
			//auto gb1 = rsrcs.at(gbuffer1);
			//auto gb2 = rsrcs.at(gbuffer2);
			auto ds = rsrcs.find(deferDS)->second;

			auto rt = rsrcs.at(deferLightedRT);

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, &ds.info.desc2info_dsv.at(dsvDesc).cpuHandle);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*deferLightingShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*deferLightingShader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT)
			);

			cmdList->SetGraphicsRootDescriptorTable(0, gb0.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, ds.info.desc2info_srv.at(dsSrvDesc).gpuHandle);

			// irradiance, prefilter, BRDF LUT
			if (renderContext.skybox.ptr == defaultSkybox.ptr)
				cmdList->SetGraphicsRootDescriptorTable(2, defaultIBLSRVDH.GetGpuHandle());
			else
				cmdList->SetGraphicsRootDescriptorTable(2, iblData->SRVDH.GetGpuHandle());

			cmdList->SetGraphicsRootDescriptorTable(3, ltcHandles.GetGpuHandle());

			auto cbLights = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12")
				->GetCommonBuffer()->GetResource()->GetGPUVirtualAddress() + renderContext.lightOffset;
			cmdList->SetGraphicsRootConstantBufferView(4, cbLights);

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12")
				->GetCommonBuffer()->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(5, cbPerCamera->GetGPUVirtualAddress());

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		skyboxPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			if (renderContext.skybox.ptr == defaultSkybox.ptr)
				return;

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*skyboxShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*skyboxShader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT)
			);

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto rt = rsrcs.find(deferLightedSkyRT)->second;
			auto ds = rsrcs.find(deferDS)->second;

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, &ds.info.desc2info_dsv.at(dsvDesc).cpuHandle);

			cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12")
				->GetCommonBuffer()->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(1, cbPerCamera->GetGPUVirtualAddress());

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(36, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		forwardPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto rt = rsrcs.find(sceneRT)->second;
			auto ds = rsrcs.find(forwardDS)->second;

			auto dsHandle = ds.info.desc2info_dsv.at(dsvDesc).cpuHandle;

			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, &dsHandle);

			DrawObjects(cmdList, "Forward", 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
		}
	);

	size_t hitgroupsize = 1;
	fgExecutor.RegisterPassFunc(
		tlasPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxr_cmdlist;
			ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&dxr_cmdlist)));
			size_t cnt = 0;

			std::vector<std::size_t> entities;
			size_t submeshCnt = 0;
			for (const auto& [id, data] : renderContext.entity2data) {
				// check
				{ // 2 step
					// 1. material valid

					bool valid = true;
					size_t N = std::min(data.mesh->GetSubMeshes().size(), data.materials.size());
					for (size_t i = 0; i < N; i++) {
						if (data.materials[i] && data.materials[i]->shader->name != "StdPipeline/Geometry") {
							valid = false;
							break;
						}
					}
					if (!valid)
						continue;

					// 2. mesh valid
					if (!GPURsrcMngrDX12::Instance().GetMeshBLAS(*data.mesh))
						continue;
				}

				D3D12_RAYTRACING_INSTANCE_DESC desc;
				desc.InstanceID = cnt;
				desc.InstanceContributionToHitGroupIndex = submeshCnt;
				desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				auto l2wT = transformf(renderContext.entity2data.at(id).l2w).transpose();
				memcpy(desc.Transform, l2wT.data(), sizeof(desc.Transform));
				desc.AccelerationStructure = GPURsrcMngrDX12::Instance().GetMeshBLAS(*data.mesh)->GetGPUVirtualAddress();
				desc.InstanceMask = 0xFF;
				tlasBuffers->instanceDescs.Set((cnt++) * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), &desc);

				entities.push_back(id);
				submeshCnt += data.mesh->GetSubMeshes().size();
			}

			// fill shader table
			{
				Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> pRtsoProps;
				rtpso->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

				hitgroupsize = 1 + submeshCnt;
				tlasBuffers->hitGroupTable.FastReserve(mHitGroupTableEntrySize * hitgroupsize);
				tlasBuffers->hitGroupTable.Set(0, pRtsoProps->GetShaderIdentifier(kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				size_t tableIdx = 0;

				struct RootTable {
					UINT64 buffer_table;
					UINT64 albedo_table;
					UINT64 roughness_table;
					UINT64 metalness_table;
					UINT64 normal_table;
				};
				for (const auto& id : entities) {
					const auto& data = renderContext.entity2data[id];
					for (size_t i = 0; i < data.mesh->GetSubMeshes().size(); i++) {
						tlasBuffers->hitGroupTable.Set(mHitGroupTableEntrySize* (1 + tableIdx),
							pRtsoProps->GetShaderIdentifier(kIndirectHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
						if (i < data.materials.size() && data.materials[i]) {
							RootTable rootTable{
							.buffer_table = GPURsrcMngrDX12::Instance().GetMeshBufferTableGpuHandle(*data.mesh, i).ptr,
							.albedo_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gAlbedoMap").value)).ptr,
							.roughness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gRoughnessMap").value)).ptr,
							.metalness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gMetalnessMap").value)).ptr,
							.normal_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gNormalMap").value)).ptr
							};
							tlasBuffers->hitGroupTable.Set(mHitGroupTableEntrySize* (1 + (tableIdx++)) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &rootTable);
						}
						else { // error material
							RootTable rootTable{
								.buffer_table = GPURsrcMngrDX12::Instance().GetMeshBufferTableGpuHandle(*data.mesh, i).ptr,
								.albedo_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*errorTex).ptr,
								.roughness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*whiteTex).ptr,
								.metalness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*blackTex).ptr,
								.normal_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*dnormalTex).ptr
							};
							tlasBuffers->hitGroupTable.Set(mHitGroupTableEntrySize* (1 + (tableIdx++)) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &rootTable);
						}
					}
				}
				assert(hitgroupsize == tableIdx + 1);
			}

			if (entities.size() == 0)
				return;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
			inputs.NumDescs = (UINT)entities.size();
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			inputs.InstanceDescs = tlasBuffers->instanceDescs.GetResource()->GetGPUVirtualAddress();
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
			asDesc.Inputs = inputs;
			asDesc.DestAccelerationStructureData = tlasBuffers->pResult->GetGPUVirtualAddress();
			asDesc.ScratchAccelerationStructureData = tlasBuffers->pScratch->GetGPUVirtualAddress();

			dxr_cmdlist->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
		}
	);

	fgExecutor.RegisterPassFunc(
		rtPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxr_cmdlist;
			ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&dxr_cmdlist)));

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			auto tlas = rsrcs.at(TLAS);
			auto gb0 = rsrcs.at(gbuffer0); // table {gb0, gb1, gb2}
			//auto gb1 = rsrcs.at(gbuffer1);
			//auto gb2 = rsrcs.at(gbuffer2);
			auto ds = rsrcs.find(deferDS)->second;

			D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
			raytraceDesc.Width = (UINT)width;
			raytraceDesc.Height = (UINT)height;
			raytraceDesc.Depth = (UINT)1;

			auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");
			auto commonBuffer = shaderCBMngr->GetCommonBuffer();
			auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.cameraOffset;

			auto cbLights = commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.lightOffset;

			auto* cbuffer = shaderCBMngr->GetBuffer(placeholder_rtshader);
			cbuffer->FastReserve(UDX12::Util::CalcConstantBufferByteSize(sizeof(RTConfig)));
			RTConfig& rtconfig = frameRsrcMngr.GetCurrentFrameResource()->GetResource<RTConfig>("RTConfig");
			if (renderContext.cameraChange)
				rtconfig.gFrameCnt = 0;
			else
				rtconfig.gFrameCnt++;
			cbuffer->Set(0, &rtconfig);

			{ // fill shader table every frame
				uint8_t* pData;
				ThrowIfFailed(tlasBuffers->shaderTable->Map(0, nullptr, (void**)&pData));

				// Entry 0
				////////////

				// rt uav, rtU uav
				// *(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 0)
				
				// gbuffers
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) = gb0.info.desc2info_srv.at(srvDesc).gpuHandle.ptr;

				// depth
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 16) = ds.info.desc2info_srv.at(dsSrvDesc).gpuHandle.ptr;

				// brdf lut
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 24) = iblData->SRVDH.GetGpuHandle(2).ptr;

				// camera
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 32) = cameraCBAdress;

				// Entry 2
				////////////

				*(uint64_t*)(pData + 2 * mRayGenAndMissShaderTableEntrySize + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = renderContext.skybox.ptr;
				
				tlasBuffers->shaderTable->Unmap(0, nullptr);
			}

			// RayGen is the first entry in the shader-table
			raytraceDesc.RayGenerationShaderRecord.StartAddress = tlasBuffers->shaderTable->GetGPUVirtualAddress() + 0 * mRayGenAndMissShaderTableEntrySize;
			raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mRayGenAndMissShaderTableEntrySize;

			// Miss is the second entry in the shader-table
			size_t missOffset = 1 * mRayGenAndMissShaderTableEntrySize;
			raytraceDesc.MissShaderTable.StartAddress = tlasBuffers->shaderTable->GetGPUVirtualAddress() + missOffset;
			raytraceDesc.MissShaderTable.StrideInBytes = mRayGenAndMissShaderTableEntrySize;
			raytraceDesc.MissShaderTable.SizeInBytes = 2 * mRayGenAndMissShaderTableEntrySize;   // 2 miss shader entry

			// Hit is the third entry in the shader-table
			raytraceDesc.HitGroupTable.StartAddress = tlasBuffers->hitGroupTable.GetResource()->GetGPUVirtualAddress();
			raytraceDesc.HitGroupTable.StrideInBytes = mHitGroupTableEntrySize;
			raytraceDesc.HitGroupTable.SizeInBytes = mHitGroupTableEntrySize * hitgroupsize;

			// Bind the root signature
			dxr_cmdlist->SetComputeRootSignature(mpGlobalRootSig.Get());
			
			// Dispatch
			dxr_cmdlist->SetPipelineState1(rtpso.Get());
			dxr_cmdlist->SetComputeRootDescriptorTable(0, tlas.info.desc2info_srv.at(tlasSrvDesc).gpuHandle);
			dxr_cmdlist->SetComputeRootConstantBufferView(1, cbLights);
			dxr_cmdlist->DispatchRays(&raytraceDesc);
		}
	);

	fgExecutor.RegisterPassFunc(
		reprojectPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*rtReprojectShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*rtReprojectShader,
				0,
				static_cast<size_t>(-1),
				0,
				DXGI_FORMAT_UNKNOWN,
				DXGI_FORMAT_UNKNOWN)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto in_table = rsrcs.at(prevRTResult); // 6
			auto out_table = rsrcs.at(accRTResult); // 4

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(0, nullptr, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, in_table.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, out_table.info.desc2info_uav.at(tex2dUAVDesc).gpuHandle);
			auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");
			auto commonBuffer = shaderCBMngr->GetCommonBuffer();
			auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.cameraOffset;
			cmdList->SetGraphicsRootConstantBufferView(2, cameraCBAdress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		filterMomentsPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*filterMomentsShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*filterMomentsShader,
				0,
				static_cast<size_t>(-1),
				2,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				DXGI_FORMAT_UNKNOWN)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			// {rtS, rtU, cm, hl}
			auto rtS = rsrcs.at(accRTResult);
			// auto rtU = rsrcs.at(accRTUResult);
			// auto cm = rsrcs.at(colorMoment);
			// auto hl = rsrcs.at(historyLength);

			auto gb1 = rsrcs.at(gbuffer1);
			auto lz = rsrcs.at(linearZ);

			auto rt0 = rsrcs.at(fAccRTResult);
			auto rt1 = rsrcs.at(fAccRTUResult);

			// Clear the render texture and depth buffer.
			//cmdList->ClearRenderTargetView(rt0.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);
			//cmdList->ClearRenderTargetView(rt1.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			D3D12_CPU_DESCRIPTOR_HANDLE rts[2] = { rt0.info.null_info_rtv.cpuHandle, rt1.info.null_info_rtv.cpuHandle };
			cmdList->OMSetRenderTargets(2, rts, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, rtS.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, gb1.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(2, lz.info.desc2info_srv.at(srvDesc).gpuHandle);

			auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");

			auto commonBuffer = shaderCBMngr->GetCommonBuffer();
			auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.cameraOffset;
			cmdList->SetGraphicsRootConstantBufferView(3, cameraCBAdress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		copyPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			std::pair<std::size_t, std::size_t> inouts[] = {
				{linearZ,linearZ_copyrt},
				{rtATrousDenoiseResults.front(),accRTResult_copyrt},
				{rtUATrousDenoiseResults.front(),accRTUResult_copyrt},
				{colorMoment,colorMoment_copyrt},
			};
			for (const auto& [in_id, out_id] : inouts) {
				auto in_rsrc = rsrcs.at(in_id).resource;
				auto out_rsrc = rsrcs.at(out_id).resource;
				cmdList->CopyResource(out_rsrc, in_rsrc);
			}
		}
	);

	{ // denoise passes
		auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
			->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");
		auto* configBuffer = shaderCBMngr->GetBuffer(*atrousShader);
		struct ATrousConfig {
			int gKernelStep;
		};
		configBuffer->FastReserve(ATrousN* UDX12::Util::CalcConstantBufferByteSize(sizeof(ATrousConfig)));
		for (std::size_t i = 0; i < ATrousN; i++) {
			fgExecutor.RegisterPassFunc(
				rtATrousDenoisePasses[i],
				[&, i](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
					cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*atrousShader));
					cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
						*atrousShader,
						0,
						static_cast<size_t>(-1),
						2,
						DXGI_FORMAT_R32G32B32A32_FLOAT,
						DXGI_FORMAT_UNKNOWN)
					);

					auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
					cmdList->SetDescriptorHeaps(1, &heap);
					cmdList->RSSetViewports(1, &viewport);
					cmdList->RSSetScissorRects(1, &scissorRect);

					// {rtS, rtU}
					auto rtS = rsrcs.at(fg.GetPassNodes()[rtATrousDenoisePasses[i]].Outputs()[0]);
					auto rtU = rsrcs.at(fg.GetPassNodes()[rtATrousDenoisePasses[i]].Outputs()[1]);
					auto imgS = rsrcs.at(fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[0]);
					auto imgU = rsrcs.at(fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[1]);
					auto gb1 = rsrcs.at(gbuffer1);
					auto lz = rsrcs.at(linearZ);

					// Clear the render texture and depth buffer.
					cmdList->ClearRenderTargetView(rtS.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);
					cmdList->ClearRenderTargetView(rtU.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

					// Specify the buffers we are going to render to.
					D3D12_CPU_DESCRIPTOR_HANDLE rts[2] = { rtS.info.null_info_rtv.cpuHandle, rtU.info.null_info_rtv.cpuHandle };
					cmdList->OMSetRenderTargets(2, rts, false, nullptr);

					cmdList->SetGraphicsRootDescriptorTable(0, imgS.info.desc2info_srv.at(srvDesc).gpuHandle);
					cmdList->SetGraphicsRootDescriptorTable(1, gb1.info.desc2info_srv.at(srvDesc).gpuHandle);
					cmdList->SetGraphicsRootDescriptorTable(2, lz.info.desc2info_srv.at(srvDesc).gpuHandle);

					auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
						->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");

					auto commonBuffer = shaderCBMngr->GetCommonBuffer();
					auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
						+ renderContext.cameraOffset;
					cmdList->SetGraphicsRootConstantBufferView(3, cameraCBAdress);

					auto* configBuffer = shaderCBMngr->GetBuffer(*atrousShader);
					std::size_t offset = i * UDX12::Util::CalcConstantBufferByteSize(sizeof(ATrousConfig));
					{
						ATrousConfig config{ int(i + 1) };
						configBuffer->Set(offset, &config);
					}
					cmdList->SetGraphicsRootConstantBufferView(4, configBuffer->GetResource()->GetGPUVirtualAddress() + offset);

					cmdList->IASetVertexBuffers(0, 0, nullptr);
					cmdList->IASetIndexBuffer(nullptr);
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(6, 1, 0, 0);
				}
			);
		}
	}

	fgExecutor.RegisterPassFunc(
		rtWithBRDFLUTPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto gb0 = rsrcs.at(gbuffer0); // table {gb0, gb1, gb2}
			//auto gb1 = rsrcs.at(gbuffer1);
			//auto gb2 = rsrcs.at(gbuffer2);
			auto ds = rsrcs.at(deferDS);

			auto rtDR = rsrcs.at(rtDenoisedResult); // table {rtDR, rtUDR}
			//auto rtUDR = rsrcs.at(rtUDenoisedResult);
			auto rt = rsrcs.at(rtWithBRDFLUT);

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*rtWithBRDFLUTShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*rtWithBRDFLUTShader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				DXGI_FORMAT_UNKNOWN)
			);

			cmdList->SetGraphicsRootDescriptorTable(0, rtDR.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, gb0.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(2, ds.info.desc2info_srv.at(dsSrvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(3, iblData->SRVDH.GetGpuHandle(2));
			auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");
			auto commonBuffer = shaderCBMngr->GetCommonBuffer();
			auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.cameraOffset;
			cmdList->SetGraphicsRootConstantBufferView(4, cameraCBAdress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		taaPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*taaShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*taaShader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				DXGI_FORMAT_UNKNOWN)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto taa_in = rsrcs.at(taaPrevResult); // table {prev, curr}
			auto taa_mt = rsrcs.at(motion);
			auto taa_out = rsrcs.at(taaResult);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(taa_out.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &taa_out.info.null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, taa_in.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, taa_mt.info.desc2info_srv.at(srvDesc).gpuHandle);
			auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");
			auto commonBuffer = shaderCBMngr->GetCommonBuffer();
			auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.cameraOffset;
			cmdList->SetGraphicsRootConstantBufferView(2, cameraCBAdress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		taaCopyPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto in_rsrc = rsrcs.at(taaResult).resource;
			auto out_rsrc = rsrcs.at(taaResult_copyrt).resource;
			cmdList->CopyResource(out_rsrc, in_rsrc);
		}
	);

	fgExecutor.RegisterPassFunc(
		postprocessPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*postprocessShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*postprocessShader,
				0,
				static_cast<size_t>(-1),
				1,
				rtb->GetDesc().Format,
				DXGI_FORMAT_UNKNOWN)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto rt = rsrcs.find(presentedRT)->second;
			auto img = rsrcs.find(taaResult)->second;

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, img.info.desc2info_srv.at(srvDesc).gpuHandle);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	static bool flag{ false };
	if (!flag) {
		OutputDebugStringA(fg.ToGraphvizGraph().Dump().c_str());
		flag = true;
	}

	auto [success, crst] = fgCompiler.Compile(fg);
	fgExecutor.Execute(
		initDesc.device,
		initDesc.cmdQueue,
		cmdAlloc.Get(),
		crst,
		*fgRsrcMngr
	);

	frameRsrcMngr.EndFrame(initDesc.cmdQueue);
}

void StdDXRPipeline::Impl::DrawObjects(ID3D12GraphicsCommandList* cmdList, std::string_view lightMode, size_t rtNum, DXGI_FORMAT rtFormat) {
	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<ShaderCBMngrDX12>>("ShaderCBMngrDX12");

	D3D12_GPU_DESCRIPTOR_HANDLE ibl;
	if (renderContext.skybox.ptr == defaultSkybox.ptr)
		ibl = defaultIBLSRVDH.GetGpuHandle();
	else {
		auto iblData = frameRsrcMngr.GetCurrentFrameResource()
			->GetResource<std::shared_ptr<IBLData>>("IBL data");
		ibl = iblData->SRVDH.GetGpuHandle();
	}

	auto commonBuffer = shaderCBMngr->GetCommonBuffer();
	auto cameraCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
		+ renderContext.cameraOffset;

	std::shared_ptr<const Shader> shader;

	auto Draw = [&](const RenderObject& obj) {
		const auto& pass = obj.material->shader->passes[obj.passIdx];

		if (auto target = pass.tags.find("LightMode"); target == pass.tags.end() || target->second != lightMode)
			return;

		if (shader.get() != obj.material->shader.get()) {
			shader = obj.material->shader;
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*shader));
		}

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
			commonBuffer->GetResource()->GetGPUVirtualAddress()
			+ renderContext.entity2offset.at(obj.entity.index);

		const auto& shaderCBDesc = renderContext.shaderCBDescMap.at(shader.get());

		auto lightCBAdress = commonBuffer->GetResource()->GetGPUVirtualAddress()
			+ renderContext.lightOffset;
		StdDXRPipeline::SetGraphicsRoot_CBV_SRV(cmdList, *shaderCBMngr, shaderCBDesc, *obj.material,
			{
				{StdDXRPipeline_cbPerObject, objCBAddress},
				{StdDXRPipeline_cbPerCamera, cameraCBAdress},
				{StdDXRPipeline_cbLightArray, lightCBAdress}
			},
			{
				{StdDXRPipeline_srvIBL, ibl},
				{StdDXRPipeline_srvLTC, ltcHandles.GetGpuHandle()}
			}
			);

		auto& meshGPUBuffer = GPURsrcMngrDX12::Instance().GetMeshGPUBuffer(*obj.mesh);
		const auto& submesh = obj.mesh->GetSubMeshes().at(obj.submeshIdx);
		const auto meshVBView = meshGPUBuffer.VertexBufferView();
		const auto meshIBView = meshGPUBuffer.IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &meshVBView);
		cmdList->IASetIndexBuffer(&meshIBView);
		// submesh.topology
		D3D12_PRIMITIVE_TOPOLOGY d3d12Topology;
		switch (submesh.topology)
		{
		case Ubpa::Utopia::MeshTopology::Triangles:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case Ubpa::Utopia::MeshTopology::Lines:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case Ubpa::Utopia::MeshTopology::LineStrip:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case Ubpa::Utopia::MeshTopology::Points:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		default:
			assert(false);
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			break;
		}
		cmdList->IASetPrimitiveTopology(d3d12Topology);

		if (shader->passes[obj.passIdx].renderState.stencilState.enable)
			cmdList->OMSetStencilRef(shader->passes[obj.passIdx].renderState.stencilState.ref);
		cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
			*shader,
			obj.passIdx,
			MeshLayoutMngr::Instance().GetMeshLayoutID(*obj.mesh),
			rtNum,
			rtFormat
		));
		cmdList->DrawIndexedInstanced((UINT)submesh.indexCount, 1, (UINT)submesh.indexStart, (INT)submesh.baseVertex, 0);
	};

	for (const auto& obj : renderContext.renderQueue.GetOpaques())
		Draw(obj);

	for (const auto& obj : renderContext.renderQueue.GetTransparents())
		Draw(obj);
}

StdDXRPipeline::StdDXRPipeline(InitDesc initDesc) :
	PipelineBase{ initDesc },
	pImpl{ new Impl{ initDesc } } {}

StdDXRPipeline::~StdDXRPipeline() { delete pImpl; }

void StdDXRPipeline::Render(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData, ID3D12Resource* rt) {
	pImpl->Render(worlds, cameraData, rt);
}

void StdDXRPipeline::Impl::Resize(size_t width, size_t height) {
	if (lastWidth == width && lastHeight == height)
		return;
	lastWidth = width;
	lastHeight = height;

	D3D12_RESOURCE_DESC rtResultDesc = {};
	{ // fill rtResultType
		rtResultDesc.DepthOrArraySize = 1;
		rtResultDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		rtResultDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		rtResultDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		rtResultDesc.Width = width;
		rtResultDesc.Height = (UINT)height;
		rtResultDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		rtResultDesc.MipLevels = 1;
		rtResultDesc.SampleDesc.Count = 1;
	}
	D3D12_RESOURCE_DESC historyLengthDesc = {};
	{ // fill historyLengthDesc
		historyLengthDesc.DepthOrArraySize = 1;
		historyLengthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		historyLengthDesc.Format = DXGI_FORMAT_R32_UINT;
		historyLengthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		historyLengthDesc.Width = width;
		historyLengthDesc.Height = (UINT)height;
		historyLengthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		historyLengthDesc.MipLevels = 1;
		historyLengthDesc.SampleDesc.Count = 1;
	}
	auto rt2dDesc = UDX12::Desc::RSRC::RT2D(width, (UINT)height, DXGI_FORMAT_R32G32B32A32_FLOAT);

	Microsoft::WRL::ComPtr<ID3D12Resource> rtURsrc;
	Microsoft::WRL::ComPtr<ID3D12Resource> rtSRsrc;
	Microsoft::WRL::ComPtr<ID3D12Resource> rtColorMoment;
	Microsoft::WRL::ComPtr<ID3D12Resource> rtHistoryLength;
	D3D12_CLEAR_VALUE clearvalue = {};
	clearvalue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearvalue.Color[0] = 0;
	clearvalue.Color[1] = 0;
	clearvalue.Color[2] = 0;
	clearvalue.Color[3] = 1;
	D3D12_CLEAR_VALUE clearvalue_cm = {};
	clearvalue_cm.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearvalue_cm.Color[0] = 0;
	clearvalue_cm.Color[1] = 0;
	clearvalue_cm.Color[2] = 0;
	clearvalue_cm.Color[3] = 0;
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rtResultDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearvalue,
		IID_PPV_ARGS(&rtSRsrc)
	);
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rtResultDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearvalue,
		IID_PPV_ARGS(&rtURsrc)
	);
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rtResultDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearvalue_cm,
		IID_PPV_ARGS(&rtColorMoment)
	);
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&historyLengthDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&rtHistoryLength)
	);
	Microsoft::WRL::ComPtr<ID3D12Resource> rtLinearZ;
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rt2dDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearvalue_cm,
		IID_PPV_ARGS(&rtLinearZ)
	);
	Microsoft::WRL::ComPtr<ID3D12Resource> taaPrevRsrc;
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rt2dDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearvalue,
		IID_PPV_ARGS(&taaPrevRsrc)
	);

	for (auto& frsrc : frameRsrcMngr.GetFrameResources()) {
		frsrc->DelayUpdateResource(
			"FrameGraphRsrcMngr",
			[](std::shared_ptr<UDX12::FG::RsrcMngr> rsrcMngr) {
				rsrcMngr->Clear();
			}
		);
		frsrc->DelayUpdateResource(
			"rtSRsrc",
			[rtSRsrc](Microsoft::WRL::ComPtr<ID3D12Resource>& rsrc) {
				rsrc = rtSRsrc;
			}
		);
		frsrc->DelayUpdateResource(
			"rtURsrc",
			[rtURsrc](Microsoft::WRL::ComPtr<ID3D12Resource>& rsrc) {
				rsrc = rtURsrc;
			}
		);
		frsrc->DelayUpdateResource(
			"rtColorMoment",
			[rtColorMoment](Microsoft::WRL::ComPtr<ID3D12Resource>& rsrc) {
				rsrc = rtColorMoment;
			}
		);
		frsrc->DelayUpdateResource(
			"rtHistoryLength",
			[rtHistoryLength](Microsoft::WRL::ComPtr<ID3D12Resource>& rsrc) {
				rsrc = rtHistoryLength;
			}
		);
		frsrc->DelayUpdateResource(
			"rtLinearZ",
			[rtLinearZ](Microsoft::WRL::ComPtr<ID3D12Resource>& rsrc) {
				rsrc = rtLinearZ;
			}
		);
		frsrc->DelayUpdateResource(
			"taaPrevRsrc",
			[taaPrevRsrc](Microsoft::WRL::ComPtr<ID3D12Resource>& rsrc) {
				rsrc = taaPrevRsrc;
			}
		);
	}
}
