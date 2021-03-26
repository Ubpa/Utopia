#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Render/MeshImporter.h>
#include <Utopia/Render/Mesh.h>

#include <iostream>
#include <cassert>

using namespace Ubpa;
using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path root = LR"(..\src\test\x\02_MeshImporter\assets)";
	AssetMngr::Instance().SetRootPath(root);

	{
		AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<MeshImporterCreator>());
		auto guid = AssetMngr::Instance().ImportAsset(LR"(cube.obj)");
		assert(guid.isValid());
		auto mesh = AssetMngr::Instance().LoadAsset<Mesh>(LR"(cube.obj)");
		assert(mesh.get());
		assert(mesh->GetPositions().size() == 24);
		assert(mesh->GetNormals().size() == 24);
		assert(mesh->GetUV().size() == 24);
		assert(mesh->GetIndices().size() == 36);
	}

	AssetMngr::Instance().Clear();
}
