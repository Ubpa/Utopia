#include <DustEngine/Asset/AssetMngr.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <DustEngine/ScriptSystem/LuaScript.h>

#include <fstream>
#include <map>
#include <any>
#include <memory>
#include <functional>

using namespace Ubpa::DustEngine;

struct AssetMngr::Impl {
	// asset <-> path <-> guid
	
	struct Asset {
		xg::Guid localID;
		std::unique_ptr<void, std::function<void(void*)>> ptr;

		template<typename T>
		static std::function<void(void*)> DefaultDeletor() {
			return [](void* p) {
				delete (T*)p;
			};
		}

		template<typename T>
		Asset(T* p)
			: ptr{ p, DefaultDeletor<T>() } {}

		template<typename T>
		Asset(xg::Guid localID, T* p)
			:
			localID{ localID },
			ptr { p, DefaultDeletor<T>() }
		{}
	};

	std::unordered_map<const void*, std::filesystem::path> asset2path;
	std::multimap<std::filesystem::path, Asset> path2assert;

	std::map<std::filesystem::path, xg::Guid> path2guid;
	std::unordered_map<xg::Guid, std::filesystem::path> guid2path;

	static std::string LoadFile(const std::filesystem::path& path);
	static rapidjson::Document LoadMeta(const std::filesystem::path& metapath);
};

void AssetMngr::Init() {
	assert(!pImpl);
	pImpl = new AssetMngr::Impl;
}

void AssetMngr::Clear() {
	assert(pImpl != nullptr);
	delete pImpl;
	pImpl = nullptr;
}

xg::Guid AssetMngr::AssetPathToGUID(const std::filesystem::path& path) const {
	assert("Init first" && pImpl != nullptr);
	auto target = pImpl->path2guid.find(path);
	return target == pImpl->path2guid.end() ? xg::Guid{} : target->second;
}

bool AssetMngr::Contains(const void* ptr) const {
	assert("Init first" && pImpl != nullptr);
	return pImpl->asset2path.find(ptr) != pImpl->asset2path.end();
}

const std::filesystem::path& AssetMngr::GetAssetPath(const void* ptr) const {
	assert("Init first" && pImpl != nullptr);
	static const std::filesystem::path ERROR;
	auto target = pImpl->asset2path.find(ptr);
	return target == pImpl->asset2path.end() ? ERROR : target->second;
}

const std::filesystem::path& AssetMngr::GUIDToAssetPath(const xg::Guid& guid) const {
	assert("Init first" && pImpl != nullptr);
	static const std::filesystem::path ERROR;
	auto target = pImpl->guid2path.find(guid);
	return target == pImpl->guid2path.end() ? ERROR : target->second;
}

void AssetMngr::ImportAsset(const std::filesystem::path& path) {
	assert("Init first" && pImpl != nullptr);
	assert(!path.empty() && path.is_relative());

	if (pImpl->path2guid.find(path) != pImpl->path2guid.end())
		return;
	assert(std::filesystem::exists(path));
	if (path.extension() == ".lua") {
		auto metapath = std::filesystem::path{ path }.concat(".meta");
		bool existMeta = std::filesystem::exists(metapath);
		assert(!existMeta || !std::filesystem::is_directory(metapath));
		auto target_guid = pImpl->path2guid.find(path);
		if (target_guid != pImpl->path2guid.end())
			return;

		xg::Guid guid;
		if (!existMeta) {
			// generate meta file

			guid = xg::newGuid();

			rapidjson::StringBuffer sb;
			rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
			writer.StartObject();
			writer.Key("guid");
			writer.String(guid.str());
			writer.EndObject();

			auto dirPath = path.parent_path();
			if (!std::filesystem::is_directory(dirPath))
				std::filesystem::create_directories(dirPath);

			std::ofstream ofs(metapath);
			assert(ofs.is_open());
			ofs << sb.GetString();
			ofs.close();
		}
		else {
			rapidjson::Document doc = Impl::LoadMeta(metapath);
			guid = xg::Guid{ doc["uuid"].GetString() };
		}
		pImpl->path2guid.emplace(path, guid);
		pImpl->guid2path.emplace(guid, path);
	}
	else
		assert("not support" && false);
}

void* AssetMngr::LoadAsset(const std::filesystem::path& path) {
	assert("Init first" && pImpl != nullptr);
	ImportAsset(path);
	if (path.extension() == ".lua") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto str = Impl::LoadFile(path);
		auto lua = new LuaScript(std::move(str));
		pImpl->path2assert.emplace(path, Impl::Asset{ lua });
		pImpl->asset2path.emplace(lua, path);
		return lua;
	}
	else {
		assert(false);
		return nullptr;
	}
}

// ========================

std::string AssetMngr::Impl::LoadFile(const std::filesystem::path& path) {
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

rapidjson::Document AssetMngr::Impl::LoadMeta(const std::filesystem::path& metapath) {
	// load guid
	auto str = LoadFile(metapath);
	rapidjson::Document doc;
	doc.Parse(str.c_str(), str.size());
	return doc;
}
