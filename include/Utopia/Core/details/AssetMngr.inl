#pragma once

namespace Ubpa::Utopia {
	template<typename T> requires std::negation_v<std::is_void<T>>
	bool AssetMngr::CreateAsset(std::shared_ptr<T> ptr, const std::filesystem::path& path) {
		return CreateAsset(UDRefl::SharedObject{ Type_of<T>, ptr }, path);
	}

	template<typename T>
	TAsset<T> AssetMngr::GUIDToAsset(const xg::Guid& guid) const {
		Asset asset = GUIDToAsset(guid, Type_of<T>);
		if (!asset.Valid())
			return {};
		return { asset.name, asset.obj.AsShared<T>() };
	}

	template<typename T>
	TAsset<T> AssetMngr::LoadAsset(const std::filesystem::path& path) {
		Asset asset = LoadAsset(path, Type_of<T>);
		if (!asset.Valid())
			return {};
		return { asset.name, asset.obj.AsShared<T>() };
	}
}

//inline bool operator<(const xg::Guid& lhs, const xg::Guid& rhs) noexcept {
//	static_assert(sizeof(xg::Guid) % sizeof(size_t) == 0);
//	auto lhs_p = reinterpret_cast<const size_t*>(&lhs.bytes());
//	auto rhs_p = reinterpret_cast<const size_t*>(&rhs.bytes());
//	for (size_t i = 0; i < sizeof(xg::Guid) / sizeof(size_t); i++) {
//		size_t lhs_vi = *(lhs_p + i);
//		size_t rhs_vi = *(rhs_p + i);
//		if (lhs_vi < rhs_vi)
//			return true;
//		if (lhs_vi > rhs_vi)
//			return false;
//	}
//	return false;
//}
