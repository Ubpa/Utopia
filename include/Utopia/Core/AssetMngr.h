#pragma once

#include "AssetImporter.h"

#include <_deps/crossguid/guid.hpp>

#include <UDRefl/UDRefl.hpp>

#include <unordered_map>
#include <filesystem>
#include <vector>
#include <regex>
#include <map>
#include <set>
#include <memory>
#include <type_traits>

namespace Ubpa::Utopia {
	// ref: https://docs.unity3d.com/ScriptReference/AssetDatabase.html
	class AssetMngr {
	public:
		static AssetMngr& Instance() {
			static AssetMngr instance;
			return instance;
		}

		// default : ".."
		const std::filesystem::path& GetRootPath() const noexcept;

		// clear all imported assets and change root
		void SetRootPath(std::filesystem::path path);

		void Clear();

		bool IsImported(const std::filesystem::path& path) const;

		// Get the GUID for the asset at path.
		// If the asset does not exist, AssetPathToGUID will return invalid xg::Guid
		xg::Guid AssetPathToGUID(const std::filesystem::path& path) const;

		bool DeleteAsset(const std::filesystem::path& path);

		bool CreateAsset(UDRefl::SharedObject obj, const std::filesystem::path& path);

		template<typename T> requires std::negation_v<std::is_void<T>>
		bool CreateAsset(std::shared_ptr<T> ptr, const std::filesystem::path& path);

		bool Contains(UDRefl::SharedObject obj) const;

		std::vector<xg::Guid> FindAssets(const std::wregex& matchRegex) const;

		xg::Guid GetAssetGUID(UDRefl::SharedObject obj) const;
		const std::filesystem::path& GetAssetPath(UDRefl::SharedObject obj) const;

		// get first asset type
		Type GetAssetType(const std::filesystem::path&) const;

		// gets the corresponding asset path for the supplied guid, or an empty path if the GUID can't be found.
		const std::filesystem::path& GUIDToAssetPath(const xg::Guid&) const;

		// if not loaded, return nullptr
		
		UDRefl::SharedObject GUIDToAsset(const xg::Guid&) const;
		UDRefl::SharedObject GUIDToAsset(const xg::Guid&, Type type) const;
		template<typename T>
		std::shared_ptr<T> GUIDToAsset(const xg::Guid&) const;

		// import asset at path (relative)
		// * generate meta
		xg::Guid ImportAsset(const std::filesystem::path& path);
		// recursively import asset in directory (relative)
		// not import the 'directory'
		void ImportAssetRecursively(const std::filesystem::path& directory);

		UDRefl::SharedObject LoadMainAsset(const std::filesystem::path& path);
		// returns the first asset object of type at given path
		UDRefl::SharedObject LoadAsset(const std::filesystem::path& path, Type);
		std::vector<UDRefl::SharedObject> LoadAllAssets(const std::filesystem::path& path);
		template<typename T>
		std::shared_ptr<T> LoadAsset(const std::filesystem::path& path);

		bool ReserializeAsset(const std::filesystem::path& path);

		bool MoveAsset(const std::filesystem::path& src, const std::filesystem::path& dst);

		void RegisterAssetImporterCreator(std::string_view extension, std::shared_ptr<AssetImporterCreator> creator);

		std::string_view NameofAsset(UDRefl::SharedObject obj) const;

		void SetImporterOverride(const std::filesystem::path& path, std::shared_ptr<AssetImporter> importer);
	private:
		struct Impl;
		Impl* pImpl;
		AssetMngr();
		~AssetMngr();
	};
}

#include "details/AssetMngr.inl"
