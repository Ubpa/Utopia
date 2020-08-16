#pragma once

#include "../_deps/crossguid/guid.hpp"

#include <UECS/Entity.h>

#include <unordered_map>
#include <filesystem>
#include <vector>

namespace Ubpa::DustEngine {
	// ref: https://docs.unity3d.com/ScriptReference/AssetDatabase.html
	// support : .lua, .obj, .hlsl, .shader, image(.jpg, .png, .bmp, .tga, .hdr), .tex2d
	class AssetMngr {
	public:
		static AssetMngr& Instance() {
			static AssetMngr instance;
			return instance;
		}

		void Clear();

		// Get the GUID for the asset at path.
		// If the asset does not exist, AssetPathToGUID will return invalid xg::Guid
		xg::Guid AssetPathToGUID(const std::filesystem::path& path) const;

		void CreateAsset(const void* ptr, const std::filesystem::path& path);

		bool Contains(const void* ptr) const;

		// if ptr is not an asset, return empty path
		const std::filesystem::path& GetAssetPath(const void* ptr) const;

		// gets the corresponding asset path for the supplied guid, or an empty path if the GUID can't be found.
		const std::filesystem::path& GUIDToAssetPath(const xg::Guid& guid) const;

		// import asset at path (relative)
		// * generate meta
		// * gen raw format in cache folder
		void ImportAsset(const std::filesystem::path& path);

		// load first asset at path
		void* LoadAsset(const std::filesystem::path& path);
		// returns the first asset object of type at given path
		void* LoadAsset(const std::filesystem::path& path, const std::type_info&);
		template<typename T>
		T* LoadAsset(const std::filesystem::path& path);

	private:
		struct Impl;
		Impl* pImpl;
		AssetMngr();
		~AssetMngr();
	};
}

#include "detail/AssetMngr.inl"
