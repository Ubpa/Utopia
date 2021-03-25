#pragma once

#include "../Material.h"
#include <Utopia/Core/Asset.h>

namespace Ubpa::Utopia {
	// singleton
	struct Skybox {
		TAsset<Material> material;
	};
}
