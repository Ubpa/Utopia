#pragma once

#include "../Mesh.h"
#include "../../Core/SharedVar.h"

namespace Ubpa::Utopia {
	struct MeshFilter {
		SharedVar<Mesh> mesh;
	};
}
