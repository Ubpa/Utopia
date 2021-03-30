#include <Utopia/Core/WorldAssetImporter.h>

#include <Utopia/Core/AssetMngr.h>

#include <filesystem>

using namespace Ubpa::Utopia;

WorldAsset::WorldAsset(const UECS::World* world) :
	data{Serializer::Instance().Serialize(world)} {}

WorldAsset::WorldAsset(const UECS::World* world, std::span<UECS::Entity> entities) :
	data{ Serializer::Instance().Serialize(world, entities) } {}

bool WorldAsset::ToWorld(UECS::World* world) {
	return Serializer::Instance().DeserializeToWorld(world, data);
}

void WorldAssetImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();
}

std::string WorldAssetImporter::ReserializeAsset() const {
	auto w = AssetMngr::Instance().GUIDToAsset<WorldAsset>(GetGuid());
	if (!w.get())
		return {};
	return w->GetData();
}

AssetImportContext WorldAssetImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	std::string name = path.stem().string();

	std::ifstream ifs(path);
	assert(ifs.is_open());
	std::string str;

	ifs.seekg(0, std::ios::end);
	str.reserve(ifs.tellg());
	ifs.seekg(0, std::ios::beg);

	str.assign(
		std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>()
	);

	WorldAsset wa(std::move(str));

	ctx.AddObject(name, UDRefl::SharedObject{ Type_of<WorldAsset>, std::make_shared<WorldAsset>(std::move(wa)) });
	ctx.SetMainObjectID(name);

	return ctx;
}

std::vector<std::string> WorldAssetImporterCreator::SupportedExtentions() const {
	return { ".world" };
}
