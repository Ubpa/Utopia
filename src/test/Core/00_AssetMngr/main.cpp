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
		ctx.AddObject("main", UDRefl::SharedObject{ Type_of<DefaultAsset>, std::make_shared<DefaultAsset>() });
		ctx.SetMainObjectID(name);

		return ctx;
	}

	virtual std::string ReserializeAsset() const override {
		auto asset = AssetMngr::Instance().GUIDToMainAsset(GetGuid());
		return Serializer::Instance().Serialize(asset);
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
	std::filesystem::path root = LR"(..\src\test\Core\00_AssetMngr\assets)";
	AssetMngr::Instance().SetRootPath(root);
	assert(AssetMngr::Instance().GetRootPath() == root);

	{
		auto guid = AssetMngr::Instance().ImportAsset(LR"(hello.txt)");
		assert(guid.isValid());
	}
	{
		auto asset = AssetMngr::Instance().LoadMainAsset(LR"(hello.txt)");
		assert(AssetMngr::Instance().NameofAsset(asset.GetPtr()) == "hello");
		assert(asset.GetType().Is<DefaultAsset>());
		assert(asset.GetPtr());
		assert(AssetMngr::Instance().GetAssetPath(asset.GetPtr()) == LR"(hello.txt)");
		auto guid_asset = AssetMngr::Instance().GetAssetGUID(asset.GetPtr());
		assert(AssetMngr::Instance().GUIDToMainAsset(guid_asset).GetPtr() == asset.GetPtr());
		assert(AssetMngr::Instance().GUIDToAssetPath(guid_asset) == LR"(hello.txt)");
	}
	{
		auto asset = AssetMngr::Instance().LoadMainAsset(LR"(folder\file_in_folder.txt)");
		assert(AssetMngr::Instance().NameofAsset(asset.GetPtr()) == "file_in_folder");
		assert(asset.GetType().Is<DefaultAsset>());
		assert(asset.GetPtr());
		assert(AssetMngr::Instance().GetAssetPath(asset.GetPtr()) == LR"(folder\file_in_folder.txt)");

		auto folder = AssetMngr::Instance().LoadMainAsset(LR"(folder)");
		assert(AssetMngr::Instance().NameofAsset(folder.GetPtr()) == "folder");
		assert(folder.GetType().Is<DefaultAsset>());
		assert(folder.GetPtr());
		auto folder_path = AssetMngr::Instance().GetAssetPath(folder.GetPtr());
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
		auto myasset = std::make_shared<MyAsset>();
		myasset->data = 1;
		auto create_success = AssetMngr::Instance().CreateAsset(myasset, LR"(a.myasset)");
		assert(create_success);
		auto mainasset = AssetMngr::Instance().LoadMainAsset(LR"(a.myasset)");
		assert(mainasset.GetType().Is<MyAsset>());
		auto defaultAsset = AssetMngr::Instance().LoadAsset<DefaultAsset>(LR"(a.myasset)");
		assert(defaultAsset.get() == nullptr);
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
