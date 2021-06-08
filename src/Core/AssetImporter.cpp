#include <Utopia/Core/AssetImporter.h>

#include <Utopia/Core/AssetMngr.h>

using namespace Ubpa::Utopia;

std::filesystem::path AssetImporter::GetFullPath() const {
	return AssetMngr::Instance().GetFullPath(AssetMngr::Instance().GUIDToAssetPath(GetGuid()));
}

std::string AssetImporter::ReserializeAsset() const {
	if (!SupportReserializeAsset())
		return {};

	auto asset = AssetMngr::Instance().GUIDToMainAsset(guid);
	if (!asset.GetType() || !asset.GetPtr())
		return {};
	return Serializer::Instance().Serialize(asset);
}

AssetImportContext AssetImporter::ImportAsset() const {
	AssetImportContext ctx;

	auto path = GetFullPath();
	if (path.empty())
		return {};

	std::string str;
	{ // read file to str
		std::ifstream ifs(path);
		assert(ifs.is_open());

		ifs.seekg(0, std::ios::end);
		str.reserve(ifs.tellg());
		ifs.seekg(0, std::ios::beg);

		str.assign(
			std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>()
		);
	}

	ctx.AddObject("main", Serializer::Instance().Deserialize(str));
	ctx.SetMainObjectID("main");

	return ctx;
}

void AssetImporter::RegisterToUDRefl() {
	if (UDRefl::Mngr.typeinfos.contains(Type_of<AssetImporter>))
		return;

	UDRefl::Mngr.RegisterType<AssetImporter>();
	UDRefl::Mngr.AddField<&AssetImporter::guid>(Serializer::Key::Guid);
}

void DefaultAssetImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();
	UDRefl::Mngr.RegisterType<DefaultAsset>();
}

AssetImportContext DefaultAssetImporter::ImportAsset() const {
	AssetImportContext ctx;
	
	auto path = GetFullPath();
	if (path.empty())
		return {};
	std::string name = path.stem().string();
	ctx.AddObject(name, UDRefl::SharedObject{ Type_of<DefaultAsset>, std::make_shared<DefaultAsset>() });
	ctx.SetMainObjectID(name);
	return ctx;
}
