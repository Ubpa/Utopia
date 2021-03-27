#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/HLSLFileImporter.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/ShaderImporter.h>

#include <iostream>
#include <cassert>

using namespace Ubpa;
using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path root = LR"(..\src\test\x\04_ShaderImporter\assets)";
	AssetMngr::Instance().SetRootPath(root);


	{
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<HLSLFileImporterCreator>());
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<ShaderImporterCreator>());
		auto hlsl = AssetMngr::Instance().LoadAsset<HLSLFile>(LR"(hello.hlsl)");
		auto shader = AssetMngr::Instance().LoadAsset<Shader>(LR"(PreFilter.shader)");
		assert(shader.get());
		assert(shader->hlslFile == hlsl);
		assert(shader->name == "StdPipeline/PreFilter");
		assert(shader->rootParameters.size() == 3);
		assert(shader->passes.size() == 1);

		auto nshader = std::make_shared<Shader>(*shader);
		nshader->properties.emplace("myproperty", ShaderProperty{ .value{3.4f} });
		// TODO
		// AssetMngr::Instance().CreateAsset(shader, LR"(PreFilter2.shader)");
		std::cout << Serializer::Instance().Serialize(nshader.get()) << std::endl;
	}

	AssetMngr::Instance().Clear();
}
