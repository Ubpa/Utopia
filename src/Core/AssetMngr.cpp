#include <Utopia/Core/AssetMngr.h>

#include <Utopia/Core/Serializer.h>

#include <fstream>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

struct AssetMngr::Impl {
	// N asset <-> guid <-> path
	//               |
	//               v
	//            importer
	//
	// ext -> importer creator

	std::map<std::filesystem::path, xg::Guid> path2guid; // relative path
	std::unordered_map<xg::Guid, std::filesystem::path> guid2path; // relative path

	std::unordered_map<void*, xg::Guid> assetID2guid;
	std::multimap<xg::Guid, Asset> guid2asset;

	std::unordered_map<xg::Guid, std::shared_ptr<AssetImporter>> guid2importer;

	std::map<xg::Guid, std::set<xg::Guid>> assetTree;

	std::map<std::filesystem::path, std::shared_ptr<AssetImporterCreator>> ext2creator;

	std::filesystem::path root{ L".." };

	static std::string LoadText(const std::filesystem::path& path);
	static rapidjson::Document LoadJSON(const std::filesystem::path& metapath);
};

AssetMngr::AssetMngr()
	: pImpl{ new Impl }
{
	Serializer::Instance().RegisterSerializeFunction([](const AssetImporter* aimporter, Serializer::SerializeContext& ctx) {
		aimporter->Serialize(ctx);
	});
	UDRefl::Mngr.RegisterType<DefaultAssetImporter>();
	UDRefl::Mngr.AddBases<DefaultAssetImporter, AssetImporter>();
	Serializer::Instance().RegisterDeserializeFunction([](DefaultAssetImporter* ptr, const rapidjson::Value& value, Serializer::DeserializeContext& ctx) {
		xg::Guid guid(value[Serializer::Key::Guid].GetString());
		new(ptr)DefaultAssetImporter(guid);
	});
}

AssetMngr::~AssetMngr() {
	delete pImpl;
}

bool AssetMngr::IsSupported(std::string_view extension) const noexcept {
	return pImpl->ext2creator.contains(extension);
}

void AssetMngr::RegisterAssetImporterCreator(std::string_view extension, std::shared_ptr<AssetImporterCreator> creator) {
	pImpl->ext2creator.insert_or_assign(std::filesystem::path{ extension }, creator);
}

const std::filesystem::path& AssetMngr::GetRootPath() const noexcept {
	return pImpl->root;
}

void AssetMngr::SetRootPath(std::filesystem::path path) {
	pImpl->root = std::move(path);
}

void AssetMngr::Clear() {
	pImpl->assetID2guid.clear();
	pImpl->guid2asset.clear();
	pImpl->path2guid.clear();
	pImpl->guid2path.clear();
	pImpl->assetTree.clear();
}

bool AssetMngr::IsImported(const std::filesystem::path& path) const {
	return pImpl->path2guid.contains(path);
}

xg::Guid AssetMngr::AssetPathToGUID(const std::filesystem::path& path) const {
	auto target = pImpl->path2guid.find(path);
	return target == pImpl->path2guid.end() ? xg::Guid{} : target->second;
}

bool AssetMngr::Contains(UDRefl::SharedObject obj) const {
	return pImpl->assetID2guid.contains(obj.GetPtr());
}

std::vector<xg::Guid> AssetMngr::FindAssets(const std::wregex& matchRegex) const {
	std::vector<xg::Guid> rst;
	for (const auto& [path, guid] : pImpl->path2guid) {
		if (std::regex_match(path.c_str(), matchRegex))
			rst.push_back(guid);
	}
	return rst;
}

xg::Guid AssetMngr::GetAssetGUID(UDRefl::SharedObject obj) const {
	auto target = pImpl->assetID2guid.find(obj.GetPtr());
	if (target == pImpl->assetID2guid.end())
		return {};
	return target->second;
}

const std::filesystem::path& AssetMngr::GetAssetPath(SharedObject obj) const {
	return GUIDToAssetPath(GetAssetGUID(obj));
}

const std::map<xg::Guid, std::set<xg::Guid>>& AssetMngr::GetAssetTree() const {
	return pImpl->assetTree;
}

Ubpa::Type AssetMngr::GetAssetType(const std::filesystem::path& path) const {
	auto guid = AssetPathToGUID(path);
	if (!guid.isValid())
		return {};
	auto target = pImpl->guid2asset.find(guid);
	if (target == pImpl->guid2asset.end())
		return {};

	return target->second.obj.GetType();
}

const std::filesystem::path& AssetMngr::GUIDToAssetPath(const xg::Guid& guid) const {
	static const std::filesystem::path ERROR;
	auto target = pImpl->guid2path.find(guid);
	return target == pImpl->guid2path.end() ? ERROR : target->second;
}

Asset AssetMngr::GUIDToAsset(const xg::Guid& guid) const {
	auto target = pImpl->guid2asset.find(guid);
	if (target == pImpl->guid2asset.end())
		return {};

	return target->second;
}

Asset AssetMngr::GUIDToAsset(const xg::Guid& guid, Type type) const {
	auto iter_begin = pImpl->guid2asset.lower_bound(guid);
	auto iter_end = pImpl->guid2asset.upper_bound(guid);
	for (auto iter = iter_begin; iter != iter_end; ++iter) {
		if (iter->second.obj.GetType() == type)
			return iter->second;
	}

	return {};
}

xg::Guid AssetMngr::ImportAsset(const std::filesystem::path& path) {
	if (path.empty())
		return {};

	assert(path.is_relative());
	const auto ext = path.extension();

	 std::filesystem::path fullpath = std::filesystem::path{ GetRootPath() } += path;

	assert(ext != ".meta");

	auto target = pImpl->path2guid.find(fullpath);
	if (target != pImpl->path2guid.end())
		return target->second;

	assert(std::filesystem::exists(fullpath));
	auto metapath = std::filesystem::path{ fullpath }.concat(".meta");
	bool existMeta = std::filesystem::exists(metapath);
	assert(!existMeta || !std::filesystem::is_directory(metapath));

	xg::Guid guid;
	std::shared_ptr<AssetImporter> importer;
	if (!existMeta) {
		auto ctarget = pImpl->ext2creator.find(ext);
		if (ctarget == pImpl->ext2creator.end()) {
			// default
			guid = xg::newGuid();
			DefaultAssetImporterCreator ctor;
			importer = ctor.CreateAssetImporter(guid);
			std::string matastr = Serializer::Instance().Serialize(importer.get());
			std::ofstream ofs(metapath);
			assert(ofs.is_open());
			ofs << matastr;
			ofs.close();
		}
		else {
			guid = xg::newGuid();
			importer = ctarget->second->CreateAssetImporter(guid);
			std::string matastr = Serializer::Instance().Serialize(importer.get());

			std::ofstream ofs(metapath);
			assert(ofs.is_open());
			ofs << matastr;
			ofs.close();
		}
	}
	else {
		std::string importerJson = Impl::LoadText(metapath);
		auto importer_impl = Serializer::Instance().Deserialize(importerJson);
		auto importer_base = importer_impl.StaticCast_DerivedToBase(Type_of<AssetImporter>);
		if (!importer_base)
			return {};
		importer = importer_base.AsShared<AssetImporter>();
		guid = importer->GetGuid();
	}

	pImpl->path2guid.emplace(path, guid);
	pImpl->guid2path.emplace(guid, path);
	pImpl->guid2importer.emplace(guid, importer);

	auto pguid = ImportAsset(path.parent_path());
	if (pguid.isValid())
		pImpl->assetTree[pguid].insert(guid);

	return guid;
}

void AssetMngr::ImportAssetRecursively(const std::filesystem::path& directory) {
	assert(!directory.has_extension());
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
		auto path = entry.path();
		if(path.extension() == ".meta")
			continue;

		ImportAsset(path);
	}
}

Asset AssetMngr::LoadMainAsset(const std::filesystem::path& path) {
	auto guid = ImportAsset(path);
	if (!guid.isValid())
		return {};

	auto target = pImpl->guid2asset.find(guid);
	if (target != pImpl->guid2asset.end())
		return target->second;
	auto ctx = pImpl->guid2importer.at(guid)->ImportAsset();
	if (ctx.GetAssets().empty())
		return {};
	auto mainObj = ctx.GetMainObject();
	for (const auto& [n, obj] : ctx.GetAssets()) {
		pImpl->assetID2guid.emplace(obj.GetPtr(), guid);
		if (obj == mainObj)
			continue;
		pImpl->guid2asset.emplace(guid, Asset{ n, obj });
	}
	if (mainObj)
		pImpl->guid2asset.emplace(guid, Asset{ ctx.GetMainObjectID(), mainObj });

	return { ctx.GetMainObjectID(), mainObj ? mainObj : ctx.GetAssets().begin()->second };
}

Asset AssetMngr::LoadAsset(const std::filesystem::path& path, Type type) {
	auto guid = ImportAsset(path);
	if (!guid.isValid())
		return {};

	if (!pImpl->guid2asset.contains(guid)) {
		auto ctx = pImpl->guid2importer.at(guid)->ImportAsset();
		if (ctx.GetAssets().empty())
			return {};
		auto mainObj = ctx.GetMainObject();
		for (const auto& [n, obj] : ctx.GetAssets()) {
			pImpl->assetID2guid.emplace(obj.GetPtr(), guid);
			if (obj == mainObj)
				continue;
			pImpl->guid2asset.emplace(guid, Asset{ n, obj });
		}
		if (mainObj)
			pImpl->guid2asset.emplace(guid, Asset{ ctx.GetMainObjectID(), mainObj });
	}

	auto [iter_begin, iter_end] = pImpl->guid2asset.equal_range(guid);
	for (auto cursor = iter_begin; cursor != iter_end; ++cursor) {
		if (cursor->second.obj.GetType() == type)
			return cursor->second;
	}
	return {};
}

std::vector<Asset> AssetMngr::LoadAllAssets(const std::filesystem::path& path) {
	auto guid = ImportAsset(path);
	if (!guid.isValid())
		return {};

	if (!pImpl->guid2asset.contains(guid)) {
		auto ctx = pImpl->guid2importer.at(guid)->ImportAsset();
		if (ctx.GetAssets().empty())
			return {};
		auto mainObj = ctx.GetMainObject();
		for (const auto& [n, obj] : ctx.GetAssets()) {
			pImpl->assetID2guid.emplace(obj.GetPtr(), guid);
			if (obj == mainObj)
				continue;
			pImpl->guid2asset.emplace(guid, Asset{ n, obj });
		}
		if (mainObj)
			pImpl->guid2asset.emplace(guid, Asset{ ctx.GetMainObjectID(), mainObj });
	}

	auto [iter_begin, iter_end] = pImpl->guid2asset.equal_range(guid);
	std::vector<Asset> rst;
	for (auto cursor = iter_begin; cursor != iter_end; ++cursor)
		rst.push_back(cursor->second);
	return rst;
}

bool AssetMngr::CreateAsset(SharedObject ptr, const std::filesystem::path& path) {
	if (std::filesystem::exists(path))
		return false;

	const auto ext = path.extension().string();
	assert(ext != ".meta");
	auto ctarget = pImpl->ext2creator.find(ext);
	if (ctarget == pImpl->ext2creator.end())
		return false;

	auto json = Serializer::Instance().Serialize(ptr.GetType().GetID().GetValue(), ptr.GetPtr());
	if (json.empty())
		return false;

	auto dirPath = path.parent_path();
	if (!std::filesystem::is_directory(dirPath))
		std::filesystem::create_directories(dirPath);

	std::ofstream ofs(path);
	assert(ofs.is_open());
	ofs << json;
	ofs.close();

	auto guid = ImportAsset(path);
	if (!guid.isValid())
		return false;

	pImpl->guid2asset.emplace(guid, Asset{ path.stem().string(), ptr });
	pImpl->assetID2guid.emplace(ptr.GetPtr(), guid);

	return true;
}

bool AssetMngr::ReserializeAsset(const std::filesystem::path& path) {
	auto guid = AssetPathToGUID(path);
	if (!guid.isValid())
		return false;
	auto target = pImpl->guid2importer.find(guid);
	if (target == pImpl->guid2importer.end())
		return false;
	return target->second->ReserializeAsset();
}

bool AssetMngr::MoveAsset(const std::filesystem::path& src, const std::filesystem::path& dst) {
	if (!dst.is_relative())
		return false;

	if (std::filesystem::exists(dst))
		return false;

	auto srcguidtarget = pImpl->path2guid.find(src);
	if (srcguidtarget == pImpl->path2guid.end())
		return false;

	auto target = pImpl->path2guid.find(src);
	auto guid = target->second;
	try {
		std::filesystem::rename(src, dst);
		std::filesystem::rename(src.wstring() + L".meta", dst.wstring() + L".meta");
	}
	catch (...) {
		return false;
	}

	pImpl->guid2path.at(guid) = dst;
	pImpl->path2guid.erase(srcguidtarget);
	pImpl->path2guid.emplace(dst, guid);

	pImpl->assetTree.at(AssetPathToGUID(src.parent_path())).erase(guid);
	pImpl->assetTree.at(AssetPathToGUID(dst.parent_path())).insert(guid);
	
	return true;
}

std::string AssetMngr::Impl::LoadText(const std::filesystem::path& path) {
	std::ifstream t(path);
	std::string str;

	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign(
		std::istreambuf_iterator<char>(t),
		std::istreambuf_iterator<char>()
	);

	return str;
}
