#pragma once

namespace Ubpa::Utopia {
	template<typename Asset>
	bool AssetMngr::CreateAsset(std::shared_ptr<Asset> ptr, const std::filesystem::path& path) {
		static_assert(std::is_base_of_v<Object, Asset>);
		return  CreateAsset(std::static_pointer_cast<Object>(std::move(ptr)), path);
	}

	template<typename Asset>
	bool AssetMngr::CreateAsset(Asset&& asset, const std::filesystem::path& path) {
		using RawAsset = std::remove_reference_t<Asset>;
		return CreateAsset(std::make_shared<RawAsset>(std::forward<Asset>(asset)), path);
	}

	template<typename T>
	std::shared_ptr<T> AssetMngr::LoadAsset(const std::filesystem::path& path) {
		static_assert(std::is_base_of_v<Object, T>);
		return std::static_pointer_cast<T>(LoadAsset(path, typeid(T)));
	}

	template<typename T>
	std::shared_ptr<T> AssetMngr::GUIDToAsset(const xg::Guid& guid) const {
		void* ptr = GUIDToAsset(guid, typeid(std::decay_t<T>));
		return reinterpret_cast<T*>(ptr);
	}
}

inline bool operator<(const xg::Guid& lhs, const xg::Guid& rhs) noexcept {
	static_assert(sizeof(xg::Guid) % sizeof(size_t) == 0);
	auto lhs_p = reinterpret_cast<const size_t*>(&lhs.bytes());
	auto rhs_p = reinterpret_cast<const size_t*>(&rhs.bytes());
	for (size_t i = 0; i < sizeof(xg::Guid) / sizeof(size_t); i++) {
		size_t lhs_vi = *(lhs_p + i);
		size_t rhs_vi = *(rhs_p + i);
		if (lhs_vi < rhs_vi)
			return true;
		if (lhs_vi > rhs_vi)
			return false;
	}
	return false;
}
