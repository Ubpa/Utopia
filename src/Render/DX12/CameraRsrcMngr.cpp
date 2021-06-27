#include <Utopia/Render/DX12/CameraRsrcMngr.h>

using namespace Ubpa::Utopia;

bool CameraRsrcMngr::CameraDataLess::operator()(const IPipeline::CameraData& lhs, const IPipeline::CameraData& rhs) const noexcept {
	return std::tuple{ lhs.entity.index,lhs.entity.version,lhs.world } < std::tuple{ rhs.entity.index,rhs.entity.version,rhs.world };
}

void CameraRsrcMngr::Update(std::span<const IPipeline::CameraData> cameras) {
	std::set<IPipeline::CameraData, CameraRsrcMngr::CameraDataLess> unvisitedCameras;
	for (const auto& [cam, rsrc] : cameraRsrcs)
		unvisitedCameras.insert(cam);

	for (const auto& camera : cameras) {
		unvisitedCameras.erase(camera);
		cameraRsrcs.emplace(camera, ResourceMap{});
	}

	for (const auto& camera : unvisitedCameras)
		cameraRsrcs.erase(camera);
}
