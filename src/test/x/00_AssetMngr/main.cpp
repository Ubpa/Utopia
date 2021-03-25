#include <Utopia/Core/AssetMngr.h>

#include <iostream>
#include <assert.h>

using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	std::filesystem::path root = LR"(..\src\test\x\00_AssetMngr\assets\)";
	AssetMngr::Instance().SetRootPath(root);
	assert(AssetMngr::Instance().GetRootPath() == root);

	{
		auto guid = AssetMngr::Instance().ImportAsset(LR"(hello.txt)");
		assert(guid.isValid());
	}
	{
		auto asset = AssetMngr::Instance().LoadMainAsset(LR"(hello.txt)");
		assert(asset.name == "hello");
		assert(asset.obj.GetType().Is<DefaultAsset>());
		assert(asset.obj.GetPtr());
		assert(AssetMngr::Instance().GetAssetPath(asset.obj) == LR"(hello.txt)");
		auto guid_asset = AssetMngr::Instance().GetAssetGUID(asset.obj);
		assert(AssetMngr::Instance().GUIDToAsset(guid_asset).obj.GetPtr() == asset.obj.GetPtr());
		assert(AssetMngr::Instance().GUIDToAssetPath(guid_asset) == LR"(hello.txt)");
	}
	{
		auto asset = AssetMngr::Instance().LoadMainAsset(LR"(folder\file_in_folder.txt)");
		assert(asset.name == "file_in_folder");
		assert(asset.obj.GetType().Is<DefaultAsset>());
		assert(asset.obj.GetPtr());
		assert(AssetMngr::Instance().GetAssetPath(asset.obj) == LR"(folder\file_in_folder.txt)");

		auto folder = AssetMngr::Instance().LoadMainAsset(LR"(folder)");
		assert(folder.name == "folder");
		assert(folder.obj.GetType().Is<DefaultAsset>());
		assert(folder.obj.GetPtr());
		auto folder_path = AssetMngr::Instance().GetAssetPath(folder.obj);
		assert(folder_path == LR"(folder)");
		assert(std::filesystem::is_directory(std::filesystem::path{ AssetMngr::Instance().GetRootPath() } += folder_path));
		auto tree = AssetMngr::Instance().GetAssetTree();
		auto guid_folder = AssetMngr::Instance().GetAssetGUID(folder.obj);
		assert(tree.contains(guid_folder));
		assert(tree.at(guid_folder).contains(AssetMngr::Instance().GetAssetGUID(asset.obj)));
	}

	AssetMngr::Instance().Clear();
}
