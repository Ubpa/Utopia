#pragma once

#include "../Material.h"
#include "../../Core/SharedVar.h"

namespace Ubpa::Utopia {
	// singleton
	struct Skybox {
		SharedVar<Material> material;
	};
}
