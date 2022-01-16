#include <UGM/UGM.hpp>
#include <UECS/UECS.hpp>

#include <vector>

namespace Ubpa {
	struct TsfmLerper {
		struct TsfmLerpData {
			vecf3 translation{ 0.f };
			quatf rotation{ quatf::identity() };
			float timepoint{ 0.f };
			scalef3 nonuniform_scale{ 1.f };
			float scale{ 1.f };
		};
		std::vector<TsfmLerpData> keyframes;

		static void RegisterUDRefl();
	};

	struct TsfmLerperSystem {
		static void OnUpdate(UECS::Schedule&);
	};
}
