#pragma once

#include "MeshTopology.h"
#include <UGM/bbox.h>

namespace Ubpa::Utopia {
	struct SubMeshDescriptor {
		SubMeshDescriptor(size_t indexStart, size_t indexCount, MeshTopology topology = MeshTopology::Triangles)
			: indexStart{ indexStart }, indexCount{ indexCount }, topology{ topology } {}

		bboxf3 bounds; // Bounding box of vertices in local space.
		MeshTopology topology; // Face topology of this sub-mesh.
		size_t indexStart; // Starting point inside the whole Mesh index buffer where the face index data is found.
		size_t indexCount; // Index count for this sub-mesh face data.
		size_t baseVertex{ 0 }; // Offset that is added to each value in the index buffer, to compute the final vertex index.
		size_t firstVertex{ static_cast<size_t>(-1) }; // First vertex in the index buffer for this sub-mesh.
		size_t vertexCount{ static_cast<size_t>(-1) }; // Number of vertices used by the index buffer of this sub-mesh.
	};
}
