#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Render/TextureImporter.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>

#include <iostream>
#include <cassert>

using namespace Ubpa;
using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path root = LR"(..\src\test\x\05_TextureImporter\assets)";
	std::filesystem::create_directory(root);

	Image img(2, 3, 3);
	img.At<rgbf>(0, 0) = { 1.f,0.f,0.f };
	img.At<rgbf>(0, 1) = { 0.f,1.f,0.f };
	img.At<rgbf>(0, 2) = { 0.f,0.f,1.f };
	img.At<rgbf>(1, 0) = { 0.f,1.f,1.f };
	img.At<rgbf>(1, 1) = { 1.f,0.f,1.f };
	img.At<rgbf>(1, 2) = { 1.f,1.f,0.f };
	img.Save(R"(..\src\test\x\05_TextureImporter\assets\test.bmp)");

	AssetMngr::Instance().SetRootPath(root);

	{
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<TextureImporterCreator>());
		auto tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(test.bmp)");
		assert(tex2d.get());
		assert(tex2d->image == img);
		AssetMngr::Instance().DeleteAsset(LR"(test.bmp)");
	}

	std::filesystem::remove(root);

	AssetMngr::Instance().Clear();
}
