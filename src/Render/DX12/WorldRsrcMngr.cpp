#include <Utopia/Render/DX12/WorldRsrcMngr.h>

#include <set>

using namespace Ubpa::Utopia;

void WorldRsrcMngr::Update(std::span<const std::vector<const UECS::World*>> worldArrs, size_t delayDeleteFrames) {
	std::set<std::span<const UECS::World* const>, WorldRsrcMngr::WorldArrLess> unvisitedWorldArrs;
	for (const auto& [worldArr, rsrc] : worldRsrcMaps)
		unvisitedWorldArrs.insert(worldArr);

	for (const auto& worldArr : worldArrs) {
		unvisitedWorldArrs.erase(worldArr);
		worldRsrcMaps.emplace(worldArr, ResourceMap{});
	}

	flat_multimap<size_t, ResourceMap> newDeletedWorldRsrcMaps;
	for (auto& [frame, rsrcMap] : deletedWorldRsrcMaps) {
		assert(frame > 0);
		if (frame == 1)
			continue;
		newDeletedWorldRsrcMaps.emplace(frame - 1, std::move(rsrcMap));
	}

	deletedWorldRsrcMaps = std::move(newDeletedWorldRsrcMaps);

	for (const auto& worldArr : unvisitedWorldArrs) {
		if(delayDeleteFrames > 0)
			deletedWorldRsrcMaps.emplace(delayDeleteFrames, std::move(worldRsrcMaps.find(worldArr)->second));
		worldRsrcMaps.erase(worldRsrcMaps.find(worldArr));
	}
}
