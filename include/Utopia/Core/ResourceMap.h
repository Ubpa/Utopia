#pragma once

#include <any>
#include <map>
#include <string>
#include <cassert>

namespace Ubpa::Utopia {
	class ResourceMap {
	public:
		ResourceMap() = default;
		ResourceMap(ResourceMap&&) noexcept = default;
		ResourceMap& operator=(ResourceMap&&) = default;

		bool Contains(std::string_view name) const { return resourceMap.find(name) != resourceMap.end(); }

		template<typename T>
		void Register(std::string name, T&& resource) {
			assert(!Contains(name));
			resourceMap.emplace(std::move(name), std::any{ std::forward<T>(resource) });
		}

		void Unregister(std::string_view name) { resourceMap.erase(resourceMap.find(name)); }

		template<typename T>
		T& Get(std::string_view name) {
			static_assert(!std::is_reference_v<T>);
			assert(Contains(name));
			return std::any_cast<T&>(resourceMap.find(name)->second);
		}

		template<typename T>
		const T& Get(std::string_view name) const {
			return const_cast<ResourceMap*>(this)->Get<T>(name);
		}

		ResourceMap(const ResourceMap&) = delete;
		ResourceMap& operator=(const ResourceMap&) = delete;

	private:
		std::map<std::string, std::any, std::less<>> resourceMap;
	};
}
