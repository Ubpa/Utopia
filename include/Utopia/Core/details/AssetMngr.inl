#pragma once

namespace Ubpa::Utopia {
	template<typename T> requires std::negation_v<std::is_void<T>>
	bool AssetMngr::CreateAsset(std::shared_ptr<T> ptr, const std::filesystem::path& path) {
		return CreateAsset(UDRefl::SharedObject{ Type_of<T>, ptr }, path);
	}

	template<typename T>
	std::shared_ptr<T> AssetMngr::GUIDToAsset(const xg::Guid& guid) const {
		auto asset = GUIDToAsset(guid, Type_of<T>);
		if (!asset.GetType())
			return {};
		return asset.AsShared<T>();
	}

	template<typename T>
	std::shared_ptr<T> AssetMngr::LoadAsset(const std::filesystem::path& path) {
		auto asset = LoadAsset(path, Type_of<T>);
		if (!asset.GetType())
			return {};
		return asset.AsShared<T>();
	}
}
