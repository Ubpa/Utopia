#include <Utopia/Core/AssetImporter.h>

#include <Utopia/Core/AssetMngr.h>

using namespace Ubpa::Utopia;

void DefaultAssetImporter::Serialize(Serializer::SerializeContext& ctx) const {
	ctx.writer.StartObject();
	ctx.writer.Key(Serializer::Key::TypeID);
	ctx.writer.Uint64(Type_of<DefaultAssetImporter>.GetID().GetValue());
	ctx.writer.Key(Serializer::Key::TypeName);
	ctx.writer.String(type_name<DefaultAssetImporter>().Data());
	ctx.writer.Key(Serializer::Key::Content);
	ctx.writer.StartObject();
	ctx.writer.Key(Serializer::Key::Guid);
	ctx.writer.String(GetGuid().str());
	ctx.writer.EndObject();
	ctx.writer.EndObject();
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

std::shared_ptr<AssetImporter> DefaultAssetImporterCreator::CreateAssetImporter(xg::Guid guid) {
	return std::make_shared<DefaultAssetImporter>(guid);
}
