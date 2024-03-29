#include <Utopia/Render/DX12/StdDXRPipeline.h>

#include <Utopia/Render/DX12/PipelineCommonUtils.h>
#include <Utopia/Render/DX12/ShaderCBMngr.h>

#include <Utopia/Render/DX12/CameraRsrcMngr.h>
#include <Utopia/Render/DX12/WorldRsrcMngr.h>

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/RenderQueue.h>

#include <Utopia/Core/AssetMngr.h>

#include <Utopia/Render/Components/Camera.h>

#include <UECS/UECS.hpp>

#include <UDX12/FrameResourceMngr.h>

#include "IBLPreprocess.h"
#include "GeometryBuffer.h"
#include "DeferredLighting.h"
#include "ForwardLighting.h"
#include "Sky.h"
#include "Tonemapping.h"
#include "TAA.h"
#include <Utopia/Render/DX12/FrameGraphVisualize.h>

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
	Microsoft::WRL::ComPtr<ID3DBlob> pDxilLib = UDX12::Util::CompileLibrary(hlsl->GetText().data(), (UINT32)hlsl->GetText().size(), AssetMngr::Instance().GetFullPath(LR"(shaders)").c_str(), LR"(BasicRayTracing.rt.hlsl)");
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
	struct CameraStages {
		GeometryBuffer geometryBuffer;
		DeferredLighting deferredLighting;
		ForwardLighting forwardLighting;
		Sky sky;
		Tonemapping tonemapping;
		TAA taa;
		FrameGraphVisualize frameGraphVisualize;
	};

	struct WorldStages {
		IBLPreprocess iblPreprocess;
		FrameGraphVisualize frameGraphVisualize;
	};

	struct FrameRsrcs {
		FrameRsrcs(ID3D12Device* device) : shaderCBMngr(device) {}

		ShaderCBMngr shaderCBMngr;
		CameraRsrcMngr cameraRsrcMngr;
		WorldRsrcMngr worldRsrcMngr;
	};

	Impl(InitDesc initDesc) :
		initDesc{ initDesc },
		frameRsrcMngr{ initDesc.numFrame, initDesc.device }
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

		for (const auto& fr : frameRsrcMngr.GetFrameResources())
			fr->RegisterResource("FrameRsrcs", std::make_shared<FrameRsrcs>(initDesc.device));

		BuildShaders();
	}

	~Impl();

	WorldRsrcMngr crossFrameWorldRsrcMngr;
	CameraRsrcMngr crossFrameCameraRsrcMngr;

	size_t hitgroupsize = 0;

	const InitDesc initDesc;

	UDX12::FrameResourceMngr frameRsrcMngr;

	UFG::Compiler fgCompiler;
	std::map<std::string, FrameGraphData> frameGraphDataMap;

	std::shared_ptr<Shader> gaussBlurXShader;
	std::shared_ptr<Shader> gaussBlurYShader;
	std::shared_ptr<Shader> atrousShader;
	std::shared_ptr<Shader> rtWithBRDFLUTShader;
	std::shared_ptr<Shader> rtReprojectShader;
	std::shared_ptr<Shader> filterMomentsShader;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// DXR
	Microsoft::WRL::ComPtr<ID3D12Device5> dxr_device;
	Microsoft::WRL::ComPtr<ID3D12StateObject> rtpso;
	std::size_t mRayGenAndMissShaderTableEntrySize;
	std::size_t mHitGroupTableEntrySize;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	Shader placeholder_rtshader; // a place holder for cbuffer
	struct TLASData {
		TLASData(ID3D12Device5* device, ID3D12StateObject* rtpso, std::size_t entrySize) : instanceDescs(device, 0), hitGroupTable(device, 0)  {
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

	static constexpr const char key_CameraResizeData[] = "CameraResizeData";
	static constexpr const char key_CameraResizeData_Backup[] = "CameraResizeData_Backup";
	struct CameraResizeData {
		size_t width{ 0 };
		size_t height{ 0 };

		Microsoft::WRL::ComPtr<ID3D12Resource> taaPrevRsrc;
		Microsoft::WRL::ComPtr<ID3D12Resource> rtSRsrc;
		Microsoft::WRL::ComPtr<ID3D12Resource> rtURsrc;
		Microsoft::WRL::ComPtr<ID3D12Resource> rtColorMoment;
		Microsoft::WRL::ComPtr<ID3D12Resource> rtHistoryLength;
		Microsoft::WRL::ComPtr<ID3D12Resource> rtLinearZ;
	};

	void BuildShaders();

	void Render(
		std::span<const UECS::World* const> worlds,
		std::span<const CameraData> cameras,
		std::span<const WorldCameraLink> links,
		std::span<ID3D12Resource* const> defaultRTs);

	void CameraRender(
		const RenderContext& ctx,
		const CameraData& cameraData,
		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress,
		ID3D12Resource* default_rtb,
		const ResourceMap& worldRsrc,
		std::string_view fgName);

	void UpdateCameraResource(const CameraData& cameraData, size_t width, size_t height);
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

StdDXRPipeline::Impl::~Impl() = default;

void StdDXRPipeline::Impl::BuildShaders() {
	gaussBlurXShader = ShaderMngr::Instance().Get("StdPipeline/GaussBlur_x");
	gaussBlurYShader = ShaderMngr::Instance().Get("StdPipeline/GaussBlur_y");
	atrousShader = ShaderMngr::Instance().Get("StdPipeline/ATrous");
	rtWithBRDFLUTShader = ShaderMngr::Instance().Get("StdPipeline/RTwithBRDFLUT");
	rtReprojectShader = ShaderMngr::Instance().Get("StdPipeline/RTRepeoject");
	filterMomentsShader = ShaderMngr::Instance().Get("StdPipeline/FilterMoments");
}

void StdDXRPipeline::Impl::CameraRender(
	const RenderContext& ctx,
	const CameraData& cameraData,
	D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress,
	ID3D12Resource* default_rtb,
	const ResourceMap& worldRsrc,
	std::string_view fgName)
{
	ID3D12Resource* rtb = default_rtb;
	{ // set rtb
		if (cameraData.entity.Valid()) {
			auto camera_rtb = cameraData.world->entityMngr.ReadComponent<Camera>(cameraData.entity)->renderTarget;
			if (camera_rtb) {
				GPURsrcMngrDX12::Instance().RegisterRenderTargetTexture2D(*camera_rtb);
				rtb = GPURsrcMngrDX12::Instance().GetRenderTargetTexture2DResource(*camera_rtb);
			}
		}
		assert(rtb);
	}
	size_t width = rtb->GetDesc().Width;
	size_t height = rtb->GetDesc().Height;
	CD3DX12_VIEWPORT viewport(0.f, 0.f, width, height);
	D3D12_RECT scissorRect(0.f, 0.f, static_cast<LONG>(width), static_cast<LONG>(height));

	UpdateCameraResource(cameraData, width, height);

	auto frameRsrcs = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs");

	if (!crossFrameCameraRsrcMngr.Get(cameraData).Contains("CameraStages"))
		crossFrameCameraRsrcMngr.Get(cameraData).Register("CameraStages", std::make_shared<CameraStages>());
	auto& stages = *crossFrameCameraRsrcMngr.Get(cameraData).Get<std::shared_ptr<CameraStages>>("CameraStages");

	auto& cameraRsrcMngr = frameRsrcs->cameraRsrcMngr;

	if (!cameraRsrcMngr.Get(cameraData).Contains("FrameGraphRsrcMngr"))
		cameraRsrcMngr.Get(cameraData).Register("FrameGraphRsrcMngr", std::make_shared<UDX12::FG::RsrcMngr>(initDesc.device));
	auto fgRsrcMngr = cameraRsrcMngr.Get(cameraData).Get<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");

	if (!cameraRsrcMngr.Get(cameraData).Contains("FrameGraphExecutor"))
		cameraRsrcMngr.Get(cameraData).Register("FrameGraphExecutor", std::make_shared<UDX12::FG::Executor>(initDesc.device));
	auto fgExecutor = cameraRsrcMngr.Get(cameraData).Get<std::shared_ptr<UDX12::FG::Executor>>("FrameGraphExecutor");

	fgExecutor->NewFrame();
	fgRsrcMngr->NewFrame();
	stages.geometryBuffer.NewFrame();
	stages.deferredLighting.NewFrame();
	stages.forwardLighting.NewFrame();
	stages.sky.NewFrame();
	stages.taa.NewFrame();
	stages.tonemapping.NewFrame();
	stages.frameGraphVisualize.NewFrame();

	const auto& cameraResizeData = cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData);
	auto rtSRsrc = cameraResizeData.rtSRsrc;
	auto rtURsrc = cameraResizeData.rtURsrc;
	auto rtColorMoment = cameraResizeData.rtColorMoment;
	auto rtHistoryLength = cameraResizeData.rtHistoryLength;
	auto rtLinearZ = cameraResizeData.rtLinearZ;
	auto taaPrevRsrc = cameraResizeData.taaPrevRsrc;

	auto iblData = worldRsrc.Get<std::shared_ptr<IBLData>>("IBL data");
	auto tlasBuffers = worldRsrc.Get<std::shared_ptr<TLASData>>("TLASData");

	auto [frameGraphDataMapIter, frameGraphDataEmplaceSuccess] = frameGraphDataMap.emplace(std::string(fgName),
		FrameGraphData{ UFG::FrameGraph{std::string(fgName) }, &stages.frameGraphVisualize });

	assert(frameGraphDataEmplaceSuccess);
	UFG::FrameGraph& fg = frameGraphDataMapIter->second.fg;

	stages.geometryBuffer.RegisterInputOutputPassNodes(fg, {});

	auto gbuffer0 = stages.geometryBuffer.GetOutputNodeIDs()[0];
	auto gbuffer1 = stages.geometryBuffer.GetOutputNodeIDs()[1];
	auto gbuffer2 = stages.geometryBuffer.GetOutputNodeIDs()[2];
	auto motion = stages.geometryBuffer.GetOutputNodeIDs()[3];
	auto deferDS = stages.geometryBuffer.GetOutputNodeIDs()[4];
	auto linearZ = stages.geometryBuffer.GetOutputNodeIDs()[5];
	auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
	auto prefilterMap = fg.RegisterResourceNode("PreFilter Map");
	const size_t deferredLightingInputs[6] = { gbuffer0, gbuffer1, gbuffer2, deferDS, irradianceMap, prefilterMap };
	stages.deferredLighting.RegisterInputOutputPassNodes(fg, deferredLightingInputs);
	auto deferLightedRT = stages.deferredLighting.GetOutputNodeIDs()[0];
	const size_t forwardLightingInputs[2] = { irradianceMap, prefilterMap };
	stages.forwardLighting.RegisterInputOutputPassNodes(fg, forwardLightingInputs);
	auto sceneRT = stages.forwardLighting.GetOutputNodeIDs()[0];
	auto forwardDS = stages.forwardLighting.GetOutputNodeIDs()[1];
	stages.sky.RegisterInputOutputPassNodes(fg, { &deferDS, 1 });
	auto deferLightedSkyRT = stages.sky.GetOutputNodeIDs()[0];
	auto presentedRT = fg.RegisterResourceNode("Present");
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
	auto colorMoment = fg.RegisterResourceNode("Color Moment");
	auto taaPrevResult = fg.RegisterResourceNode("TAA Prev Result");
	const size_t taaInputs[3] = {taaPrevResult, sceneRT, motion};
	stages.taa.RegisterInputOutputPassNodes(fg, taaInputs);
	auto taaResult = stages.taa.GetOutputNodeIDs().front();
	stages.tonemapping.RegisterInputOutputPassNodes(fg, { &taaResult, 1 });
	auto tonemappedRT = stages.tonemapping.GetOutputNodeIDs().front();
	std::array<std::size_t, ATrousN> rtATrousDenoiseResults;
	for (std::size_t i = 0; i < ATrousN; i++)
		rtATrousDenoiseResults[i] = fg.RegisterResourceNode("RT ATrous denoised result " + std::to_string(i));
	std::array<std::size_t, ATrousN> rtUATrousDenoiseResults;
	for (std::size_t i = 0; i < ATrousN; i++)
		rtUATrousDenoiseResults[i] = fg.RegisterResourceNode("RT U ATrous denoised result " + std::to_string(i));
	auto rtWithBRDFLUT = fg.RegisterResourceNode("RT with BRDFLUT");
	auto rtDenoisedResult = fg.RegisterResourceNode("RT Denoised Result");
	auto rtUDenoisedResult = fg.RegisterResourceNode("RT U Denoised Result");

	auto rtPass = fg.RegisterGeneralPassNode(
		"RT Pass",
		{ gbuffer0,gbuffer1,gbuffer2,deferDS,TLAS },
		{ rtResult, rtUResult }
	);
	fg.RegisterMoveNode(accRTResult, rtResult);
	fg.RegisterMoveNode(accRTUResult, rtUResult);
	auto reprojectPass = fg.RegisterGeneralPassNode(
		"RTReproject Pass",
		{ prevRTResult, prevRTUResult, prevColorMoment, motion, prevLinearZ, linearZ },
		{ accRTResult, accRTUResult, colorMoment, historyLength }
	);
	auto filterMomentsPass = fg.RegisterGeneralPassNode(
		"Filter Moments Pass",
		{ accRTResult, accRTUResult, colorMoment, historyLength, gbuffer1, linearZ },
		{ fAccRTResult, fAccRTUResult }
	);
	std::array<std::size_t, ATrousN> rtATrousDenoisePasses;
	for (std::size_t i = 0; i < ATrousN; i++) {
		std::size_t input_node_S = i == 0 ? fAccRTResult : rtATrousDenoiseResults[i - 1];
		std::size_t input_node_U = i == 0 ? fAccRTUResult : rtUATrousDenoiseResults[i - 1];
		rtATrousDenoisePasses[i] = fg.RegisterGeneralPassNode(
			"RT ATrous Denoise Pass" + std::to_string(i),
			{ input_node_S, input_node_U, gbuffer1, linearZ },
			{ rtATrousDenoiseResults[i], rtUATrousDenoiseResults[i] }
		);
	}
	auto copyPass = fg.RegisterCopyPassNode(
		"RT Copy Pass",
		{ linearZ, rtATrousDenoiseResults.front(), rtUATrousDenoiseResults.front(), colorMoment },
		{ prevLinearZ, prevRTResult, prevRTUResult, prevColorMoment }
	);
	fg.RegisterMoveNode(rtDenoisedResult, rtATrousDenoiseResults.back());
	fg.RegisterMoveNode(rtUDenoisedResult, rtUATrousDenoiseResults.back());
	auto rtWithBRDFLUTPass = fg.RegisterGeneralPassNode(
		"RT with BRDFLUT Pass",
		{ gbuffer0,gbuffer1,gbuffer2,deferDS,rtDenoisedResult,rtUDenoisedResult },
		{ rtWithBRDFLUT }
	);
	fg.RegisterMoveNode(deferLightedSkyRT, rtWithBRDFLUT);
	fg.RegisterMoveNode(forwardDS, deferDS);
	fg.RegisterMoveNode(sceneRT, deferLightedSkyRT);
	fg.RegisterMoveNode(presentedRT, tonemappedRT);
	const size_t frameGraphVisualizeInputs[] = { gbuffer0, gbuffer1, gbuffer2 };
	stages.frameGraphVisualize.RegisterInputOutputPassNodes(fg, frameGraphVisualizeInputs);

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

	D3D12_CLEAR_VALUE gbuffer_clearvalue = {};
	gbuffer_clearvalue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gbuffer_clearvalue.Color[0] = 0;
	gbuffer_clearvalue.Color[1] = 0;
	gbuffer_clearvalue.Color[2] = 0;
	gbuffer_clearvalue.Color[3] = 0;

	static constexpr const char RTATrousDenoiseRsrcTableID[] = "RTATrousDenoiseRsrcTableID";
	static constexpr const char GBufferRsrcTableID[] = "GBufferRsrcTableID";
	static constexpr const char ReprojectRsrcTable0ID[] = "ReprojectRsrcTable0ID";
	static constexpr const char ReprojectRsrcTable1ID[] = "ReprojectRsrcTable1ID";
	static constexpr const char FilterMomentsRsrcTableID[] = "FilterMomentsRsrcTableID";
	static constexpr const char RTWithBRDFLUTRsrcTableID[] = "RTWithBRDFLUTRsrcTableID";

	for (std::size_t i = 0; i < ATrousN; i++) {
		fgRsrcMngr->RegisterTemporalRsrcAutoClear(rtATrousDenoiseResults[i], rt2dDesc);
		fgRsrcMngr->RegisterTemporalRsrcAutoClear(rtUATrousDenoiseResults[i], rt2dDesc);

		// rts, rtu
		fgRsrcMngr->RegisterRsrcTable(
			RTATrousDenoiseRsrcTableID,
			{
				{fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[0], srvDesc},
				{fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[1], srvDesc},
			}
		);

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
		.RegisterTemporalRsrc(motion, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(linearZ, rt2dDesc, gbuffer_clearvalue)
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

		.RegisterImportedRsrc(tonemappedRT, { rtb, D3D12_RESOURCE_STATE_PRESENT })
		.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(TLAS, { nullptr /*special for TLAS*/, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE })
		.RegisterImportedRsrc(taaPrevResult, { taaPrevRsrc.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })

		.RegisterRsrcTable(
			GBufferRsrcTableID,
			{
				{gbuffer0,srvDesc},
				{gbuffer1,srvDesc},
				{gbuffer2,srvDesc},
			}
		)

		.RegisterRsrcHandle(rtResult, tex2dUAVDesc, tlasBuffers->raygenDescriptors.GetCpuHandle(0), tlasBuffers->raygenDescriptors.GetGpuHandle(0), false)
		.RegisterRsrcHandle(rtUResult, tex2dUAVDesc, tlasBuffers->raygenDescriptors.GetCpuHandle(1), tlasBuffers->raygenDescriptors.GetGpuHandle(1), false)

		.RegisterPassRsrc(rtPass, rtResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(rtPass, rtUResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tex2dUAVDesc)
		.RegisterPassRsrc(rtPass, TLAS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, tlasSrvDesc)
		.RegisterPassRsrc(rtPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtPass, deferDS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ, dsSrvDesc)

		.RegisterRsrcTable(
			ReprojectRsrcTable0ID,
			{
				{ prevRTResult, srvDesc },
				{ prevRTUResult, srvDesc },
				{ prevColorMoment, srvDesc },
				{ motion, srvDesc },
				{ prevLinearZ, srvDesc },
				{ linearZ, srvDesc },
			}
		)

		.RegisterRsrcTable(
			ReprojectRsrcTable1ID,
			{
				{ accRTResult, tex2dUAVDesc },
				{ accRTUResult, tex2dUAVDesc },
				{ colorMoment, tex2dUAVDesc },
				{ historyLength, tex2dUAVDesc },
			}
		)

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

		.RegisterRsrcTable(
			FilterMomentsRsrcTableID,
			{
				{ accRTResult, srvDesc },
				{ accRTUResult, srvDesc },
				{ colorMoment, srvDesc },
				{ historyLength, UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32_UINT) },
			}
		)

		.RegisterPassRsrc(filterMomentsPass, accRTResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, accRTUResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, colorMoment, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, historyLength, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32_UINT))
		.RegisterPassRsrc(filterMomentsPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, linearZ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(filterMomentsPass, fAccRTResult, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(filterMomentsPass, fAccRTUResult, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterCopyPassRsrcState(fg, copyPass)

		.RegisterRsrcTable(
			RTWithBRDFLUTRsrcTableID,
			{
				{rtDenoisedResult,srvDesc},
				{rtUDenoisedResult,srvDesc},
			}
		)

		.RegisterPassRsrc(rtWithBRDFLUTPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, deferDS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ, dsSrvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, rtDenoisedResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, rtUDenoisedResult, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(rtWithBRDFLUTPass, rtWithBRDFLUT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		;

	stages.geometryBuffer.RegisterPassResources(*fgRsrcMngr);
	stages.deferredLighting.RegisterPassResources(*fgRsrcMngr);
	stages.forwardLighting.RegisterPassResources(*fgRsrcMngr);
	stages.sky.RegisterPassResources(*fgRsrcMngr);
	stages.taa.RegisterPassResources(*fgRsrcMngr);
	stages.tonemapping.RegisterPassResources(*fgRsrcMngr);
	stages.frameGraphVisualize.RegisterPassResourcesData(256, 256);
	stages.frameGraphVisualize.RegisterPassResources(*fgRsrcMngr);

	auto& shaderCBMngr = frameRsrcs->shaderCBMngr;

	const D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress = shaderCBMngr.GetLightCBAddress(ctx.ID);

	// irradiance, prefilter, BRDF LUT
	const D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle = ctx.skyboxSrvGpuHandle.ptr == PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle().ptr ?
		PipelineCommonResourceMngr::GetInstance().GetDefaultIBLSrvDHA().GetGpuHandle()
		: iblData->SRVDH.GetGpuHandle();

	stages.geometryBuffer.RegisterPassFuncData(cameraCBAddress, &shaderCBMngr, &ctx, "RTDeferred");
	stages.geometryBuffer.RegisterPassFunc(*fgExecutor);

	stages.deferredLighting.RegisterPassFuncData(iblDataSrvGpuHandle, cameraCBAddress, lightCBAddress);
	stages.deferredLighting.RegisterPassFunc(*fgExecutor);

	stages.forwardLighting.RegisterPassFuncData(iblData->SRVDH.GetGpuHandle(), cameraCBAddress, &shaderCBMngr, &ctx);
	stages.forwardLighting.RegisterPassFunc(*fgExecutor);

	stages.sky.RegisterPassFuncData(ctx.skyboxSrvGpuHandle, cameraCBAddress);
	stages.sky.RegisterPassFunc(*fgExecutor);

	fgExecutor->RegisterPassFunc(
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
			auto ds = rsrcs.at(deferDS);

			D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
			raytraceDesc.Width = (UINT)width;
			raytraceDesc.Height = (UINT)height;
			raytraceDesc.Depth = (UINT)1;

			{ // fill shader table every frame
				uint8_t* pData;
				ThrowIfFailed(tlasBuffers->shaderTable->Map(0, nullptr, (void**)&pData));

				// Entry 0
				////////////

				// rt uav, rtU uav
				// *(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 0)

				// gbuffers
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) = gb0.info->desc2info_srv.at(srvDesc).at(GBufferRsrcTableID).gpuHandle.ptr;

				// depth
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 16) = ds.info->GetAnySRV(dsSrvDesc).gpuHandle.ptr;

				// brdf lut
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 24) = iblData->SRVDH.GetGpuHandle(2).ptr;

				// camera
				*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 32) = cameraCBAddress;

				// Entry 2
				////////////

				*(uint64_t*)(pData + 2 * mRayGenAndMissShaderTableEntrySize + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = ctx.skyboxSrvGpuHandle.ptr;

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
			dxr_cmdlist->SetComputeRootDescriptorTable(0, tlas.info->GetAnySRV(tlasSrvDesc).gpuHandle);
			dxr_cmdlist->SetComputeRootConstantBufferView(1, lightCBAddress);
			dxr_cmdlist->DispatchRays(&raytraceDesc);
		}
	);

	fgExecutor->RegisterPassFunc(
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

			cmdList->SetGraphicsRootDescriptorTable(0, in_table.info->desc2info_srv.at(srvDesc).at(ReprojectRsrcTable0ID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, out_table.info->desc2info_uav.at(tex2dUAVDesc).at(ReprojectRsrcTable1ID).gpuHandle);

			cmdList->SetGraphicsRootConstantBufferView(2, cameraCBAddress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor->RegisterPassFunc(
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
			//cmdList->ClearRenderTargetView(rt0.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);
			//cmdList->ClearRenderTargetView(rt1.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			D3D12_CPU_DESCRIPTOR_HANDLE rts[2] = { rt0.info->null_info_rtv.cpuHandle, rt1.info->null_info_rtv.cpuHandle };
			cmdList->OMSetRenderTargets(2, rts, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, rtS.info->desc2info_srv.at(srvDesc).at(FilterMomentsRsrcTableID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, gb1.info->desc2info_srv.at(srvDesc).at(GBufferRsrcTableID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(2, lz.info->GetAnySRV(srvDesc).gpuHandle);

			cmdList->SetGraphicsRootConstantBufferView(3, cameraCBAddress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor->RegisterCopyPassFunc(fg, copyPass);

	{ // denoise passes
		for (std::size_t i = 0; i < ATrousN; i++) {
			fgExecutor->RegisterPassFunc(
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
					//auto imgU = rsrcs.at(fg.GetPassNodes()[rtATrousDenoisePasses[i]].Inputs()[1]);
					auto gb1 = rsrcs.at(gbuffer1);
					auto lz = rsrcs.at(linearZ);

					// Clear the render texture and depth buffer.
					cmdList->ClearRenderTargetView(rtS.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);
					cmdList->ClearRenderTargetView(rtU.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

					// Specify the buffers we are going to render to.
					D3D12_CPU_DESCRIPTOR_HANDLE rts[2] = { rtS.info->null_info_rtv.cpuHandle, rtU.info->null_info_rtv.cpuHandle };
					cmdList->OMSetRenderTargets(2, rts, false, nullptr);

					cmdList->SetGraphicsRootDescriptorTable(0, imgS.info->desc2info_srv.at(srvDesc).at(RTATrousDenoiseRsrcTableID).gpuHandle);
					cmdList->SetGraphicsRootDescriptorTable(1, gb1.info->desc2info_srv.at(srvDesc).at(GBufferRsrcTableID).gpuHandle);
					cmdList->SetGraphicsRootDescriptorTable(2, lz.info->GetAnySRV(srvDesc).gpuHandle);

					cmdList->SetGraphicsRootConstantBufferView(3, cameraCBAddress);

					cmdList->SetGraphicsRootConstantBufferView(4, PipelineCommonResourceMngr::GetInstance().GetATrousConfigGpuAddress(i));

					cmdList->IASetVertexBuffers(0, 0, nullptr);
					cmdList->IASetIndexBuffer(nullptr);
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(6, 1, 0, 0);
				}
			);
		}
	}

	fgExecutor->RegisterPassFunc(
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
			cmdList->ClearRenderTargetView(rt.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info->null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*rtWithBRDFLUTShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*rtWithBRDFLUTShader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				DXGI_FORMAT_UNKNOWN)
			);

			cmdList->SetGraphicsRootDescriptorTable(0, rtDR.info->desc2info_srv.at(srvDesc).at(RTWithBRDFLUTRsrcTableID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, gb0.info->desc2info_srv.at(srvDesc).at(GBufferRsrcTableID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(2, ds.info->GetAnySRV(dsSrvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(3, iblData->SRVDH.GetGpuHandle(2));

			cmdList->SetGraphicsRootConstantBufferView(4, cameraCBAddress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	stages.taa.RegisterPassFuncData(cameraCBAddress);
	stages.taa.RegisterPassFunc(*fgExecutor);

	stages.tonemapping.RegisterPassFunc(*fgExecutor);

	stages.frameGraphVisualize.RegisterPassFunc(*fgExecutor);

	static bool flag{ false };
	if (!flag) {
		OutputDebugStringA(fg.ToGraphvizGraph2().Dump().c_str());
		flag = true;
	}

	auto crst = fgCompiler.Compile(fg);
	fgExecutor->Execute(
		initDesc.cmdQueue,
		crst,
		*fgRsrcMngr
	);
}

void StdDXRPipeline::Impl::Render(
	std::span<const UECS::World* const> worlds,
	std::span<const CameraData> cameras,
	std::span<const WorldCameraLink> links,
	std::span<ID3D12Resource* const> defaultRTs
) {
	// Cycle through the circular frame resource array.
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	frameRsrcMngr.BeginFrame();

	frameGraphDataMap.clear();

	UpdateCrossFrameCameraResources(crossFrameCameraRsrcMngr, cameras, defaultRTs);

	auto frameRsrcs = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs");

	frameRsrcs->cameraRsrcMngr.Update(cameras);

	std::vector<std::vector<const UECS::World*>> worldArrs;
	for (const auto& link : links) {
		std::vector<const UECS::World*> worldArr;
		for (auto idx : link.worldIndices)
			worldArr.push_back(worlds[idx]);
		worldArrs.push_back(std::move(worldArr));
	}

	auto& shaderCBMngr = frameRsrcs->shaderCBMngr;
	shaderCBMngr.NewFrame();

	std::vector<RenderContext> ctxs;
	for (size_t i = 0; i < worldArrs.size(); ++i) {
		auto ctx = GenerateRenderContext(i, worldArrs[i]);

		std::vector<CameraData> linkedCameras;
		for (size_t camIdx : links[i].cameraIndices)
			linkedCameras.push_back(cameras[camIdx]);
		std::vector<const CameraConstants*> cameraConstantsVector;
		for (const auto& linkedCamera : linkedCameras) {
			auto* camConts = &crossFrameCameraRsrcMngr.Get(linkedCamera).Get<CameraConstants>(key_CameraConstants);
			cameraConstantsVector.push_back(camConts);
		}
		shaderCBMngr.RegisterRenderContext(ctx, cameraConstantsVector);

		ctxs.push_back(std::move(ctx));
	}

	hitgroupsize = 0;

	crossFrameWorldRsrcMngr.Update(worldArrs, initDesc.numFrame);
	auto& curFrameWorldRsrcMngr = frameRsrcs->worldRsrcMngr;
	curFrameWorldRsrcMngr.Update(worldArrs);

	for (size_t linkIdx = 0; linkIdx < links.size(); ++linkIdx) {
		auto& ctx = ctxs[linkIdx];
		auto& crossFrameWorldRsrc = crossFrameWorldRsrcMngr.Get(worldArrs[linkIdx]);
		auto& curFrameWorldRsrc = curFrameWorldRsrcMngr.Get(worldArrs[linkIdx]);

		if (!curFrameWorldRsrc.Contains("FrameGraphRsrcMngr"))
			curFrameWorldRsrc.Register("FrameGraphRsrcMngr", std::make_shared<UDX12::FG::RsrcMngr>(initDesc.device));
		auto fgRsrcMngr = curFrameWorldRsrc.Get<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");
		fgRsrcMngr->NewFrame();

		if (!curFrameWorldRsrc.Contains("FrameGraphExecutor"))
			curFrameWorldRsrc.Register("FrameGraphExecutor", std::make_shared<UDX12::FG::Executor>(initDesc.device));
		auto fgExecutor = curFrameWorldRsrc.Get<std::shared_ptr<UDX12::FG::Executor>>("FrameGraphExecutor");
		fgExecutor->NewFrame();

		if (!crossFrameWorldRsrc.Contains("IBL data")) {
			auto iblData = std::make_shared<IBLData>();
			iblData->Init(initDesc.device);
			crossFrameWorldRsrc.Register("IBL data", iblData);
		}

		if (!crossFrameWorldRsrc.Contains("WorldStages"))
			crossFrameWorldRsrc.Register("WorldStages", std::make_shared<WorldStages>());
		auto& stages = *crossFrameWorldRsrc.Get<std::shared_ptr<WorldStages>>("WorldStages");
		stages.iblPreprocess.NewFrame();
		stages.frameGraphVisualize.NewFrame();

		if (!curFrameWorldRsrc.Contains("IBL data"))
			curFrameWorldRsrc.Register("IBL data", crossFrameWorldRsrc.Get<std::shared_ptr<IBLData>>("IBL data"));
		auto iblData = curFrameWorldRsrc.Get<std::shared_ptr<IBLData>>("IBL data");

		if (!curFrameWorldRsrc.Contains("TLASData"))
		{
			auto tlasBuffers = std::make_shared<TLASData>(dxr_device.Get(), rtpso.Get(), mRayGenAndMissShaderTableEntrySize);
			curFrameWorldRsrc.Register("TLASData", tlasBuffers);
		}
		auto tlasBuffers = curFrameWorldRsrc.Get<std::shared_ptr<TLASData>>("TLASData");

		{ // tlas buffers reverse
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
			inputs.NumDescs = (UINT)ctx.entity2data.size(); // upper bound
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			inputs.InstanceDescs = tlasBuffers->instanceDescs.GetResource() ? tlasBuffers->instanceDescs.GetResource()->GetGPUVirtualAddress() : 0;

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
			dxr_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
			tlasBuffers->Reserve(initDesc.device, info, ctx.renderQueue.GetOpaques().size());
		}

		const std::string fgName = "L" + std::to_string(linkIdx);
		auto [frameGraphDataMapIter, frameGraphEmplaceSuccess] = frameGraphDataMap.emplace(fgName, UFG::FrameGraph{ fgName });
		assert(frameGraphEmplaceSuccess);
		UFG::FrameGraph& fg = frameGraphDataMapIter->second.fg;

		stages.iblPreprocess.RegisterInputOutputPassNodes(fg, {});
		auto irradianceMap = stages.iblPreprocess.GetOutputNodeIDs()[0];
		auto prefilterMap = stages.iblPreprocess.GetOutputNodeIDs()[1];
		auto TLAS = fg.RegisterResourceNode("TLAS");
		stages.frameGraphVisualize.RegisterInputOutputPassNodes(fg, {});

		auto tlasPass = fg.RegisterGeneralPassNode(
			"TLAS Pass",
			{},
			{ TLAS }
		);

		D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc = {};
		{ // fill tlasSrvDesc
			tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			tlasSrvDesc.RaytracingAccelerationStructure.Location = tlasBuffers->pResult->GetGPUVirtualAddress();
		}

		(*fgRsrcMngr)
			.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
			.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
			.RegisterImportedRsrc(TLAS, { nullptr /*special for TLAS*/, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE })

			.RegisterPassRsrcState(tlasPass, TLAS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
			;

		stages.iblPreprocess.RegisterPassResources(*fgRsrcMngr);

		stages.iblPreprocess.RegisterPassFuncData(iblData.get(), ctx.skyboxSrvGpuHandle);

		stages.iblPreprocess.RegisterPassFunc(*fgExecutor);

		fgExecutor->RegisterPassFunc(
			tlasPass,
			[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
				auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
				cmdList->SetDescriptorHeaps(1, &heap);

				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxr_cmdlist;
				ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&dxr_cmdlist)));
				size_t cnt = 0;

				std::vector<std::size_t> entities;
				size_t submeshCnt = 0;
				for (const auto& [id, data] : ctx.entity2data) {
					// check
					{ // 2 step
						// 1. material valid

						bool valid = true;
						size_t N = std::min(data.mesh->GetSubMeshes().size(), data.materials.size());
						for (size_t i = 0; i < N; i++) {
							if (data.materials[i] && data.materials[i]->shader->name != "StdPipeline/Geometry Buffer") {
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
					auto l2wT = transformf(ctx.entity2data.at(id).l2w).transpose();
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
						const auto& data = ctx.entity2data[id];
						for (size_t i = 0; i < data.mesh->GetSubMeshes().size(); i++) {
							tlasBuffers->hitGroupTable.Set(mHitGroupTableEntrySize * (1 + tableIdx),
								pRtsoProps->GetShaderIdentifier(kIndirectHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
							if (i < data.materials.size() && data.materials[i]) {
								RootTable rootTable{
								.buffer_table = GPURsrcMngrDX12::Instance().GetMeshBufferTableGpuHandle(*data.mesh, i).ptr,
								.albedo_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gAlbedoMap").value)).ptr,
								.roughness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gRoughnessMap").value)).ptr,
								.metalness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gMetalnessMap").value)).ptr,
								.normal_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*std::get<SharedVar<Texture2D>>(data.materials[i]->properties.at("gNormalMap").value)).ptr
								};
								tlasBuffers->hitGroupTable.Set(mHitGroupTableEntrySize * (1 + (tableIdx++)) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &rootTable);
							}
							else { // error material
								RootTable rootTable{
									.buffer_table = GPURsrcMngrDX12::Instance().GetMeshBufferTableGpuHandle(*data.mesh, i).ptr,
									.albedo_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*PipelineCommonResourceMngr::GetInstance().GetErrorTex2D()).ptr,
									.roughness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*PipelineCommonResourceMngr::GetInstance().GetWhiteTex2D()).ptr,
									.metalness_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*PipelineCommonResourceMngr::GetInstance().GetBlackTex2D()).ptr,
									.normal_table = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*PipelineCommonResourceMngr::GetInstance().GetNormalTex2D()).ptr
								};
								tlasBuffers->hitGroupTable.Set(mHitGroupTableEntrySize * (1 + (tableIdx++)) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &rootTable);
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

		stages.frameGraphVisualize.RegisterPassResourcesData(256, 256);
		stages.frameGraphVisualize.RegisterPassResources(*fgRsrcMngr);
		stages.frameGraphVisualize.RegisterPassFunc(*fgExecutor);

		static bool flag{ false };
		if (!flag) {
			OutputDebugStringA(fg.ToGraphvizGraph2().Dump().c_str());
			flag = true;
		}

		auto crst = fgCompiler.Compile(fg);
		fgExecutor->Execute(
			initDesc.cmdQueue,
			crst,
			*fgRsrcMngr
		);

		for (size_t i = 0; i < links[linkIdx].cameraIndices.size(); ++i) {
			size_t cameraIdx = links[linkIdx].cameraIndices[i];
			CameraRender(
				ctx,
				cameras[cameraIdx],
				shaderCBMngr.GetCameraCBAddress(ctx.ID, i),
				defaultRTs[cameraIdx],
				curFrameWorldRsrc,
				fgName + "C" + std::to_string(i));
		}
	}

	frameRsrcMngr.EndFrame(initDesc.cmdQueue);
}

void StdDXRPipeline::Init(InitDesc desc) {
	if (!pImpl)
		pImpl = new Impl{ desc };
}

bool StdDXRPipeline::IsInitialized() const {
	return pImpl != nullptr;
}

StdDXRPipeline::~StdDXRPipeline() { delete pImpl; }

void StdDXRPipeline::Render(
	std::span<const UECS::World* const> worlds,
	std::span<const CameraData> cameras,
	std::span<const WorldCameraLink> links,
	std::span<ID3D12Resource* const> defaultRTs)
{
	assert(pImpl);
	pImpl->Render(worlds, cameras, links, defaultRTs);
}

const std::map<std::string, IPipeline::FrameGraphData>& StdDXRPipeline::GetFrameGraphDataMap() const {
	assert(pImpl);
	return pImpl->frameGraphDataMap;
}

void StdDXRPipeline::Impl::UpdateCameraResource(const CameraData& cameraData, size_t width, size_t height)  {
	auto& cameraRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs")->cameraRsrcMngr;
	if(cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData_Backup))
		cameraRsrcMngr.Get(cameraData).Unregister(key_CameraResizeData_Backup);
	if (!cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData))
		cameraRsrcMngr.Get(cameraData).Register(key_CameraResizeData, CameraResizeData{});
	const auto& currCameraResizeData = cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData);
	if (currCameraResizeData.width == width && currCameraResizeData.height == height)
		return;
	CameraResizeData cameraResizeData;
	cameraResizeData.width = width;
	cameraResizeData.height = height;

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

	cameraResizeData.rtURsrc = std::move(rtURsrc);
	cameraResizeData.rtSRsrc = std::move(rtSRsrc);
	cameraResizeData.rtColorMoment = std::move(rtColorMoment);
	cameraResizeData.rtHistoryLength = std::move(rtHistoryLength);
	cameraResizeData.rtLinearZ = std::move(rtLinearZ);
	cameraResizeData.taaPrevRsrc = std::move(taaPrevRsrc);

	cameraRsrcMngr.Get(cameraData).Unregister(key_CameraResizeData);
	cameraRsrcMngr.Get(cameraData).Register(key_CameraResizeData, cameraResizeData);

	for (auto& frsrc : frameRsrcMngr.GetFrameResources()) {
		if (frsrc.get() == frameRsrcMngr.GetCurrentFrameResource())
			continue;

		auto& cameraRsrcMngr = frsrc->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs")->cameraRsrcMngr;
		if(!cameraRsrcMngr.Contains(cameraData))
			continue;

		if (!cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData))
			cameraRsrcMngr.Get(cameraData).Register(key_CameraResizeData, CameraResizeData{});

		if (!cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData_Backup)){
			cameraRsrcMngr.Get(cameraData).Register(
				key_CameraResizeData_Backup,
				std::move(cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData))
			);
		}
		
		cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData) = cameraResizeData;
	}
}
