#pragma once

namespace Ubpa::DustEngine {
	template<typename T>
	T* AssetMngr::LoadAsset(const std::filesystem::path& path) {
		void* ptr = LoadAsset(path, typeid(std::decay_t<T>));
		return reinterpret_cast<T*>(ptr);
	}
}
