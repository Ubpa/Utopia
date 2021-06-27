#pragma once

#include "../../Core/ResourceMap.h"
#include "IPipeline.h"
#include <map>

namespace Ubpa::Utopia {
	class CameraRsrcMngr {
	public:
		void Update(std::span<const IPipeline::CameraData> cameras);
		bool Contains(const IPipeline::CameraData& camera) const { return cameraRsrcs.contains(camera); }
		ResourceMap& Get(const IPipeline::CameraData& camera) { return cameraRsrcs.at(camera); }
		size_t Size() const noexcept { return cameraRsrcs.size(); }

	private:
		struct CameraDataLess {
			bool operator()(const IPipeline::CameraData& lhs, const IPipeline::CameraData& rhs) const noexcept;
		};
		std::map<IPipeline::CameraData, ResourceMap, CameraDataLess> cameraRsrcs;
	};
}
