#pragma once

#include <UECS/Entity.h>
#include <UGM/point.h>

#include <vector>

namespace Ubpa::Utopia {
	class Mesh;
	struct Material;

	struct RenderObject {
		UECS::Entity entity;

		std::shared_ptr<const Material> material;
		size_t passIdx{ static_cast<size_t>(-1) };

		std::shared_ptr<const Mesh> mesh;
		size_t submeshIdx{ static_cast<size_t>(-1) };

		vecf3 translation{ 0.f };
	};

	class RenderQueue {
	public:
		void Add(RenderObject object);
		void Sort(pointf3 cameraPos);
		const std::vector<RenderObject>& GetOpaques() const noexcept { return opaques; }
		const std::vector<RenderObject>& GetTransparents() const noexcept { return transparents; }
		void Clear();
	private:
		std::vector<RenderObject> opaques;
		std::vector<RenderObject> transparents;
	};
}
