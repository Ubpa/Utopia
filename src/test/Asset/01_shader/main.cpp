#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Render/HLSLFile.h>
#include <DustEngine/Render/Shader.h>

#include <iostream>

using namespace Ubpa::DustEngine;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	AssetMngr::Instance().ImportAssetRecursively(L"..\\assets");
	auto hlslPath = AssetMngr::Instance().GUIDToAssetPath(xg::Guid{ "d1ee38ec-7485-422e-93f3-c8886169a858" });
	auto hlsl = AssetMngr::Instance().LoadAsset<HLSLFile>(hlslPath);

	Shader shader;
	shader.hlslFile = hlsl;
	shader.shaderName = "StdPipeline/Geometry";
	shader.targetName = "5_0";
	ShaderPass pass;
	pass.vertexName = "VS";
	pass.fragmentName = "PS";
	shader.passes.push_back(pass);
	
	DescriptorRange ranges[4];
	ranges[0].Init(RootDescriptorType::SRV, 1, 0);
	ranges[1].Init(RootDescriptorType::SRV, 1, 1);
	ranges[2].Init(RootDescriptorType::SRV, 1, 2);
	ranges[3].Init(RootDescriptorType::SRV, 1, 3);
	RootDescriptorTable tables[4];
	tables[0].push_back(ranges[0]);
	tables[1].push_back(ranges[1]);
	tables[2].push_back(ranges[2]);
	tables[3].push_back(ranges[3]);

	RootDescriptor rootDescs[3];
	rootDescs[0].Init(RootDescriptorType::CBV, 0);
	rootDescs[1].Init(RootDescriptorType::CBV, 1);
	rootDescs[2].Init(RootDescriptorType::CBV, 2);

	shader.rootParameters.emplace_back(tables[0]);
	shader.rootParameters.emplace_back(tables[1]);
	shader.rootParameters.emplace_back(tables[2]);
	shader.rootParameters.emplace_back(tables[3]);
	shader.rootParameters.emplace_back(rootDescs[0]);
	shader.rootParameters.emplace_back(rootDescs[1]);
	shader.rootParameters.emplace_back(rootDescs[2]);

	AssetMngr::Instance().CreateAsset(shader, L"..\\assets\\test\\test.shader");

	return 0;
}
