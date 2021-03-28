#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/HLSLFileImporter.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/ShaderImporter.h>
#include <Utopia/Render/TextureImporter.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/MaterialImporter.h>

#include <iostream>
#include <cassert>

using namespace Ubpa;
using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path root = LR"(..\src\test\x\06_MaterialImporter\assets)";
	Image img(2, 3, 3);
	img.At<rgbf>(0, 0) = { 1.f,0.f,0.f };
	img.At<rgbf>(0, 1) = { 0.f,1.f,0.f };
	img.At<rgbf>(0, 2) = { 0.f,0.f,1.f };
	img.At<rgbf>(1, 0) = { 0.f,1.f,1.f };
	img.At<rgbf>(1, 1) = { 1.f,0.f,1.f };
	img.At<rgbf>(1, 2) = { 1.f,1.f,0.f };
	img.Save(R"(..\src\test\x\06_MaterialImporter\assets\test.bmp)");

	AssetMngr::Instance().SetRootPath(root);

	{
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<HLSLFileImporterCreator>());
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<TextureImporterCreator>());
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<ShaderImporterCreator>());
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<MaterialImporterCreator>());
		AssetMngr::Instance().ImportAssetRecursively(LR"(.)");
		auto shader = AssetMngr::Instance().LoadAsset<Shader>(LR"(PreFilter.shader)");
		auto tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(test.bmp)");
		auto mat = std::make_shared<Material>();
		mat->shader = shader;
		mat->properties.emplace("mytex", SharedVar<Texture2D>(tex2d));
		mat->properties.emplace("myfloat", 2.f);
		auto success = AssetMngr::Instance().CreateAsset(mat, LR"(hello.mat)");
		assert(success);
		AssetMngr::Instance().Clear();
		AssetMngr::Instance().ImportAssetRecursively(LR"(.)");
		auto nmat = AssetMngr::Instance().LoadAsset<Material>(LR"(hello.mat)");
		assert(mat->shader->name == nmat->shader->name);
		assert(mat->properties.size() == nmat->properties.size());
		std::cout << Serializer::Instance().Serialize(nmat.get()) << std::endl;
	}

	AssetMngr::Instance().DeleteAsset(LR"(test.bmp)");

	AssetMngr::Instance().Clear();

	AssetMngr::Instance().SetRootPath(LR"(..\assets)");
	AssetMngr::Instance().ImportAssetRecursively(LR"(.)");

	auto m = AssetMngr::Instance().LoadAsset<Material>(LR"(_internal\materials\skyBlack.mat)");
	const auto skyboxprop = m->properties.at("gSkybox");
	return 0;
}
