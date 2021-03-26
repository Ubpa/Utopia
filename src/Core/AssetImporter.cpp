#include <Utopia/Core/AssetImporter.h>

#include <Utopia/Core/AssetMngr.h>

using namespace Ubpa::Utopia;

void AssetImporter::RegisterToUDRefl() {
	if (UDRefl::Mngr.typeinfos.contains(Type_of<AssetImporter>))
		return;

	UDRefl::Mngr.RegisterType<AssetImporter>();
	UDRefl::Mngr.AddField<&AssetImporter::guid>(Serializer::Key::Guid);
}

AssetImportContext DefaultAssetImporter::ImportAsset() const {
	AssetImportContext ctx;
	const auto& path = AssetMngr::Instance().GUIDToAssetPath(GetGuid());
	if (path.empty())
		return {};
	std::string name = path.stem().string();
	ctx.AddObject(name, UDRefl::SharedObject{ Type_of<DefaultAsset>, std::make_shared<DefaultAsset>() });
	ctx.SetMainObjectID(name);
	return ctx;
}
