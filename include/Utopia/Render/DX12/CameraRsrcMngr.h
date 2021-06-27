#pragma once

#include "../../Core/ResourceMap.h"
#include "IPipeline.h"
#include <USmallFlat/flat_map.hpp>

namespace Ubpa::Utopia {
	class CameraRsrcMngr {
	public:
		void Update(std::span<const IPipeline::CameraData> cameras);
		bool Contains(const IPipeline::CameraData& camera) const { return cameraRsrcs.contains(camera); }
		ResourceMap& Get(const IPipeline::CameraData& camera) { return cameraRsrcs.at(camera); }
		ResourceMap& Get(size_t i) { return (cameraRsrcs.data() + i)->second; }
		size_t Size() const noexcept { return cameraRsrcs.size(); }
		size_t Index(const IPipeline::CameraData& camera) const { return (size_t)(cameraRsrcs.find(camera) - cameraRsrcs.begin()); }

	private:
		struct CameraDataLess {
			bool operator()(const IPipeline::CameraData& lhs, const IPipeline::CameraData& rhs) const noexcept;
		};
		flat_map<IPipeline::CameraData, ResourceMap, CameraDataLess> cameraRsrcs;
	};
}