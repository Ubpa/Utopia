#include <DustEngine/Core/Systems/TRSToLocalToWorldSystem.h>

#include <DustEngine/Core/Components/LocalToWorld.h>
#include <DustEngine/Core/Components/Rotation.h>
#include <DustEngine/Core/Components/Scale.h>
#include <DustEngine/Core/Components/Translation.h>

using namespace Ubpa::DustEngine;

void TRSToLocalToWorldSystem::OnUpdate(UECS::Schedule& schedule) {
	UECS::ArchetypeFilter filter;
	filter.all = { UECS::CmptAccessType::Of<UECS::Write<LocalToWorld>> };
	filter.any = {
		UECS::CmptAccessType::Of<UECS::Latest<Translation>>,
		UECS::CmptAccessType::Of<UECS::Latest<Rotation>>,
		UECS::CmptAccessType::Of<UECS::Latest<Scale>>,
	};

	schedule.RegisterChunkJob([](UECS::ChunkView chunk) {
		auto chunkL2W = chunk.GetCmptArray<LocalToWorld>();
		auto chunkT = chunk.GetCmptArray<Translation>();
		auto chunkR = chunk.GetCmptArray<Rotation>();
		auto chunkS = chunk.GetCmptArray<Scale>();

		bool containsT = chunkT != nullptr;
		bool containsR = chunkR != nullptr;
		bool containsS = chunkS != nullptr;

		for (size_t i = 0; i < chunk.EntityNum(); i++) {
			// 000
			if (!containsT && !containsR && !containsS) {
				assert(false);
			}
			// 001
			else if (!containsT && !containsR && containsS) {
				chunkL2W[i].value = transformf{ chunkS[i].value };
			}
			// 010
			else if (!containsT && containsR && !containsS) {
				chunkL2W[i].value = transformf{ chunkR[i].value };
			}
			// 011
			else if (!containsT && containsR && containsS) {
				chunkL2W[i].value = transformf{ chunkR[i].value, chunkS[i].value };
			}
			// 100
			else if (containsT && !containsR && !containsS) {
				chunkL2W[i].value = transformf{ chunkT[i].value };
			}
			// 101
			else if (containsT && !containsR && containsS) {
				chunkL2W[i].value = transformf{ chunkT[i].value, scalef3{chunkS[i].value} };
			}
			// 110
			else if (containsT && containsR && !containsS) {
				chunkL2W[i].value = transformf{ chunkT[i].value, chunkR[i].value };
			}
			// 111
			else/* if (containsT && containsR && containsS)*/ {
				chunkL2W[i].value = transformf{ chunkT[i].value, chunkR[i].value, scalef3{chunkS[i].value} };
			}
		}
	}, SystemFuncName, filter);
}
