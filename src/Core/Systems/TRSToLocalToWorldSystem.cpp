#include <Utopia/Core/Systems/TRSToLocalToWorldSystem.h>

#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/Rotation.h>
#include <Utopia/Core/Components/Scale.h>
#include <Utopia/Core/Components/NonUniformScale.h>
#include <Utopia/Core/Components/Translation.h>

using namespace Ubpa::Utopia;

void TRSToLocalToWorldSystem::OnUpdate(UECS::Schedule& schedule) {
	UECS::ArchetypeFilter filter;
	filter.all = { UECS::CmptAccessType::Of<UECS::Write<LocalToWorld>> };
	filter.any = {
		UECS::CmptAccessType::Of<UECS::Latest<Translation>>,
		UECS::CmptAccessType::Of<UECS::Latest<Rotation>>,
		UECS::CmptAccessType::Of<UECS::Latest<Scale>>,
		UECS::CmptAccessType::Of<UECS::Latest<NonUniformScale>>,
	};

	schedule.RegisterChunkJob([](UECS::ChunkView chunk) {
		auto chunkL2W = chunk.GetCmptArray<LocalToWorld>();
		auto chunkT = chunk.GetCmptArray<Translation>();
		auto chunkR = chunk.GetCmptArray<Rotation>();
		auto chunkS = chunk.GetCmptArray<Scale>();
		auto chunkNUS = chunk.GetCmptArray<NonUniformScale>();

		bool containsT = !chunkT.empty();
		bool containsR = !chunkR.empty();
		bool containsS = !chunkS.empty() || !chunkNUS.empty();
		assert(containsT || containsR || containsS);

		for (size_t i = 0; i < chunk.EntityNum(); i++) {
			scalef3 s = !chunkS.empty() ? chunkS[i].value : 1.f;
			if (!chunkNUS.empty())
				s *= chunkNUS[i].value;

			// 00
			if (!containsT && !containsR)
				chunkL2W[i].value = transformf{ s };
			// 01
			else if (!containsT && containsR)
				chunkL2W[i].value = transformf{ chunkR[i].value, s };
			// 10
			else if (containsT && !containsR)
				chunkL2W[i].value = transformf{ chunkT[i].value, s };
			// 11
			else // if (containsT && containsR)
				chunkL2W[i].value = transformf{ chunkT[i].value, chunkR[i].value, s };
		}
	}, SystemFuncName, filter);
}
