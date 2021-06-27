#pragma once

#include "../../Core/ResourceMap.h"
#include <USmallFlat/flat_map.hpp>
#include <USmallFlat/flat_multimap.hpp>

#include <span>

namespace Ubpa::UECS { class World; }

namespace Ubpa::Utopia {
	class WorldRsrcMngr {
	public:
		void Update(std::span<const std::vector<const UECS::World*>> worldArrs, size_t delayDeleteFrames = 0);
		bool Contains(std::span<const UECS::World* const> worldArr) const { return worldRsrcMaps.contains(worldArr); }
		ResourceMap& Get(std::span<const UECS::World* const> worldArr) { return worldRsrcMaps.find(worldArr)->second; }
		ResourceMap& Get(size_t i) { return (worldRsrcMaps.data() + i)->second; }
		size_t Size() const noexcept { return worldRsrcMaps.size(); }
		size_t Index(std::span<const UECS::World* const> worldArr) const { return (size_t)(worldRsrcMaps.find(worldArr) - worldRsrcMaps.begin()); }

		struct WorldArrLess {
			using is_transparent = int;
			template<typename T, typename U>
			bool operator()(const T& lhs, const U& rhs) const noexcept {
				size_t n = std::max(lhs.size(), rhs.size());
				for (size_t i = 0; i < n; i++) {
					if (i >= lhs.size()) {
						assert(i < rhs.size());
						return true;
					}
					else if (i >= rhs.size())
						return false;
					else {
						if (lhs[i] < rhs[i])
							return true;
						else if (lhs[i] > rhs[i])
							return false;
					}
				}
				return false;
			}
		};
	private:
		flat_map<std::vector<const UECS::World*>, ResourceMap, WorldArrLess> worldRsrcMaps;
		flat_multimap<size_t, ResourceMap> deletedWorldRsrcMaps;
	};
}
