#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/HLSLFileImporter.h>

#include <iostream>
#include <cassert>

using namespace Ubpa;
using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path root = LR"(..\src\test\Render\01_HLSLImporter\assets)";
	AssetMngr::Instance().SetRootPath(root);

	{
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<HLSLFileImporterCreator>());
		auto guid = AssetMngr::Instance().ImportAsset(LR"(hello.hlsl)");
		assert(guid.isValid());
		auto hlsl = AssetMngr::Instance().LoadAsset<HLSLFile>(LR"(hello.hlsl)");
		assert(hlsl.get());
		assert(hlsl->GetText() == "hello");
		assert(hlsl->GetLocalDir() == R"(..\src\test\Render\01_HLSLImporter\assets)");
	}

	AssetMngr::Instance().Clear();
}
