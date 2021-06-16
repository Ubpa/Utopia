#pragma once

#include "IPipeline.h"
#include <any>
#include <map>
#include <string>
#include <USmallFlat/flat_map.hpp>

namespace Ubpa::Utopia {
	class CameraResource {
	public:
		bool HaveResource(std::string_view name) const { return resourceMap.find(name) != resourceMap.end(); }

		template<typename T>
		void RegisterResource(std::string name, T&& resource) {
			assert(!HaveResource(name));
			resourceMap.emplace(std::move(name), std::any{ std::forward<T>(resource) });
		}

		void UnregisterResource(std::string_view name) { resourceMap.erase(resourceMap.find(name)); }

		template<typename T>
		T& GetResource(std::string_view name) {
			static_assert(!std::is_reference_v<T>);
			assert(HaveResource(name));
			return std::any_cast<T&>(resourceMap.find(name)->second);
		}

		template<typename T>
		const T& GetResource(std::string_view name) const {
			return const_cast<CameraResource*>(this)->GetResource<T>();
		}

	private:
		std::map<std::string, std::any, std::less<>> resourceMap;
	};

	class CameraRsrcMngr {
	public:
		void Update(std::span<const IPipeline::CameraData> cameras);
		bool Contain(const IPipeline::CameraData& camera) const { return cameraRsrcs.contains(camera); }
		CameraResource& Get(const IPipeline::CameraData& camera) { return cameraRsrcs.at(camera); }
		CameraResource& Get(size_t i) { return (cameraRsrcs.data() + i)->second; }
		size_t Size() const noexcept { return cameraRsrcs.size(); }
		size_t Index(const IPipeline::CameraData& camera) const { return (size_t)(cameraRsrcs.find(camera) - cameraRsrcs.begin()); }

	private:
		struct CameraDataLess {
			bool operator()(const IPipeline::CameraData& lhs, const IPipeline::CameraData& rhs) const noexcept;
		};
		flat_map<IPipeline::CameraData, CameraResource, CameraDataLess> cameraRsrcs;
	};
}