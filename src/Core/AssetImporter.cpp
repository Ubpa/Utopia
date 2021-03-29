#include <Utopia/Core/AssetImporter.h>

#include <Utopia/Core/AssetMngr.h>

using namespace Ubpa::Utopia;

std::filesystem::path AssetImporter::GetFullPath() const {
	return AssetMngr::Instance().GetFullPath(AssetMngr::Instance().GUIDToAssetPath(GetGuid()));
}

std::string AssetImporter::ReserializeAsset() const {
	auto asset = AssetMngr::Instance().GUIDToAsset(guid);
	if (!asset.GetType() || !asset.GetPtr())
		return {};
	return Serializer::Instance().Serialize(asset);
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
