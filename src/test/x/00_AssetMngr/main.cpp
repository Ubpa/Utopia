#include <Utopia/Core/AssetMngr.h>

#include <iostream>
#include <assert.h>

using namespace Ubpa;
using namespace Ubpa::Utopia;

struct MyAsset {
	int data;
};

class MyAssetImporter final : public TAssetImporter<MyAssetImporter> {
public:
	using TAssetImporter<MyAssetImporter>::TAssetImporter;

	virtual AssetImportContext ImportAsset() const override {
		AssetImportContext ctx;
		auto path = GetFullPath();
		if (path.empty())
			return {};
		std::string name = path.stem().string();
		auto myasset = std::make_shared<MyAsset>();
		myasset->data = initdata;
		ctx.AddObject(name, UDRefl::SharedObject{ Type_of<MyAsset>, myasset });
		ctx.AddObject("default", UDRefl::SharedObject{ Type_of<DefaultAsset>, std::make_shared<DefaultAsset>() });
		ctx.SetMainObjectID(name);

		return ctx;
	}

	virtual std::string ReserializeAsset() const override {
		auto asset = AssetMngr::Instance().GUIDToAsset(GetGuid());
		return Serializer::Instance().Serialize(asset.GetType().GetID().GetValue(), asset.GetPtr());
	}

	static void RegisterToUDRefl() {
		RegisterToUDReflHelper();
		UDRefl::Mngr.AddField<&MyAssetImporter::initdata>("initdata");
		UDRefl::Mngr.RegisterType<MyAsset>();
		UDRefl::Mngr.AddField<&MyAsset::data>("data");
	}
private:
	int initdata{ 1 };
};

class MyAssetImporterCreator final : public TAssetImporterCreator<MyAssetImporter> {
	virtual std::vector<std::string> SupportedExtentions() const override { return { ".myasset" }; }
};

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	std::filesystem::path root = LR"(..\src\test\x\00_AssetMngr\assets)";
	AssetMngr::Instance().SetRootPath(root);
	assert(AssetMngr::Instance().GetRootPath() == root);

	{
		auto guid = AssetMngr::Instance().ImportAsset(LR"(hello.txt)");
		assert(guid.isValid());
		assert(guid.isValid());
	}
	{
		auto asset = AssetMngr::Instance().LoadMainAsset(LR"(hello.txt)");
		assert(AssetMngr::Instance().NameofAsset(asset) == "hello");
		assert(asset.GetType().Is<DefaultAsset>());
		assert(asset.GetPtr());
		assert(AssetMngr::Instance().GetAssetPath(asset) == LR"(hello.txt)");
		auto guid_asset = AssetMngr::Instance().GetAssetGUID(asset);
		assert(AssetMngr::Instance().GUIDToAsset(guid_asset).GetPtr() == asset.GetPtr());
		assert(AssetMngr::Instance().GUIDToAssetPath(guid_asset) == LR"(hello.txt)");
	}
	{
		auto asset = AssetMngr::Instance().LoadMainAsset(LR"(folder\file_in_folder.txt)");
		assert(AssetMngr::Instance().NameofAsset(asset) == "file_in_folder");
		assert(asset.GetType().Is<DefaultAsset>());
		assert(asset.GetPtr());
		assert(AssetMngr::Instance().GetAssetPath(asset) == LR"(folder\file_in_folder.txt)");

		auto folder = AssetMngr::Instance().LoadMainAsset(LR"(folder)");
		assert(AssetMngr::Instance().NameofAsset(folder) == "folder");
		assert(folder.GetType().Is<DefaultAsset>());
		assert(folder.GetPtr());
		auto folder_path = AssetMngr::Instance().GetAssetPath(folder);
		assert(folder_path == LR"(folder)");
	}
	{
		auto guid = AssetMngr::Instance().ImportAsset(LR"(.no_stem)");
		assert(!guid.isValid());
	}
	{
		AssetMngr::Instance().ImportAssetRecursively(LR"(recursive_0)");
	}
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<MyAssetImporterCreator>());
	{
		auto mainasset = AssetMngr::Instance().LoadMainAsset(LR"(a.myasset)");
		assert(mainasset.GetType().Is<MyAsset>());
		auto defaultAsset = AssetMngr::Instance().LoadAsset<DefaultAsset>(LR"(a.myasset)");
		assert(defaultAsset.get() != nullptr);
		bool success = AssetMngr::Instance().ReserializeAsset(LR"(a.myasset)");
		assert(success);
	}
	{
		AssetMngr::Instance().Clear();
		auto mainasset = AssetMngr::Instance().LoadMainAsset(LR"(a.myasset)");
		assert(mainasset.GetType().Is<MyAsset>());
		auto defaultAsset = AssetMngr::Instance().LoadAsset<DefaultAsset>(LR"(a.myasset)");
		assert(defaultAsset.get() != nullptr);
		bool success = AssetMngr::Instance().ReserializeAsset(LR"(a.myasset)");
		assert(success);
	}
	{
		auto myasset = std::make_shared<MyAsset>();
		myasset->data = 2;
		auto create_success = AssetMngr::Instance().CreateAsset(myasset, LR"(b.myasset)");
		assert(create_success);
		auto delete_success = AssetMngr::Instance().DeleteAsset(LR"(b.myasset)");
		assert(delete_success);
	}

	AssetMngr::Instance().Clear();
}
