#include "TsfmLerper.h"
#include <Utopia/Core/Components/Rotation.h>
#include <Utopia/Core/Components/RotationEuler.h>
#include <Utopia/Core/Components/Translation.h>
#include <Utopia/Core/Components/Scale.h>
#include <Utopia/Core/Components/NonUniformScale.h>
#include <Utopia/Core/Components/WorldTime.h>
#include <UDRefl/UDRefl.hpp>

using namespace Ubpa;
using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void TsfmLerper::RegisterUDRefl() {
	UDRefl::Mngr.RegisterType<TsfmLerpData>();
	UDRefl::Mngr.SimpleAddField<&TsfmLerpData::translation>("translation");
	UDRefl::Mngr.SimpleAddField<&TsfmLerpData::rotation>("rotation");
	UDRefl::Mngr.SimpleAddField<&TsfmLerpData::nonuniform_scale>("nonuniform_scale");
	UDRefl::Mngr.SimpleAddField<&TsfmLerpData::scale>("scale");
	UDRefl::Mngr.SimpleAddField<&TsfmLerpData::timepoint>("timepoint");

	UDRefl::Mngr.RegisterType<TsfmLerper>();
	UDRefl::Mngr.AddField<&TsfmLerper::keyframes>("keyframes");
}

void TsfmLerperSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterChunkJob(
		[](ChunkView chunk, Latest<Singleton<WorldTime>> time) {
			size_t N = chunk->EntityNum();
			auto lerpers = chunk->GetCmptArray<TsfmLerper>();
			auto positions = chunk->GetCmptArray<Translation>();
			auto rotations = chunk->GetCmptArray<Rotation>();
			auto rotationEs = chunk->GetCmptArray<RotationEuler>();
			auto nscales = chunk->GetCmptArray<NonUniformScale>();
			auto scales = chunk->GetCmptArray<Scale>();

			for (size_t i = 0; i < N; i++) {
				if (lerpers[i].keyframes.empty())
					continue;
				float curT = 0.f;
				size_t left = 0;
				for (size_t j = 0; j < lerpers[i].keyframes.size(); j++) {
					if (lerpers[i].keyframes[j].timepoint < curT) {
						lerpers[i].keyframes[j].timepoint = curT;
						continue;
					}
					if (time->elapsedTime <= lerpers[i].keyframes[j].timepoint)
						break;
					left = j;
					curT = lerpers[i].keyframes[left].timepoint;
				}
				size_t right = std::min(left + 1, lerpers[i].keyframes.size() - 1);

				const auto& leftF = lerpers[i].keyframes[left];
				const auto& rightF = lerpers[i].keyframes[right];

				float delta = std::max(rightF.timepoint - leftF.timepoint, Epsilon<float>);
				float t = std::clamp(((float)time->elapsedTime - leftF.timepoint) / delta, 0.f, 1.f);

				if (!positions.empty()) {
					vecf3 pos = leftF.translation.lerp(rightF.translation, t);
					positions[i].value = pos;
				}

				quatf rot = leftF.rotation.slerp(rightF.rotation, t);

				if (!rotations.empty())
					rotations[i].value = rot;
				
				if (!rotationEs.empty())
					rotationEs[i].value = rot.to_euler();

				if (!scales.empty())
					scales[i].value = Ubpa::lerp(leftF.scale, rightF.scale, t);

				if (!nscales.empty())
					nscales[i].value = leftF.nonuniform_scale.lerp(rightF.nonuniform_scale, t);
			}
		},
		"TsfmLerperSystem",
		Schedule::ChunkJobConfig{
			.archetypeFilter = {
				.all = {AccessTypeID_of<Write<TsfmLerper>>},
				.any = {
					AccessTypeID_of<Write<RotationEuler>>,
					AccessTypeID_of<Write<Rotation>>,
					AccessTypeID_of<Write<Translation>>,
					AccessTypeID_of<Write<Scale>>,
					AccessTypeID_of<Write<NonUniformScale>>
				}
			}
		}
	);
}