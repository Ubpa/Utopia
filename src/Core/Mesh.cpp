#include <DustEngine/Core/Mesh.h>

using namespace Ubpa::DustEngine;

void Mesh::SetPositions(std::vector<pointf3> positions) {
	assert(isEditable);
	dirty = true;
	this->positions = std::move(positions);
}

void Mesh::SetColors(std::vector<rgbf> colors) {
	assert(isEditable);
	dirty = true;
	this->colors = std::move(colors);
}

void Mesh::SetNormals(std::vector<normalf> normals) {
	assert(isEditable);
	dirty = true;
	this->normals = std::move(normals);
}

void Mesh::SetTangents(std::vector<vecf3> tangents) {
	assert(isEditable);
	dirty = true;
	this->tangents = std::move(tangents);
}

void Mesh::SetUV(std::vector<pointf2> uv) {
	assert(isEditable);
	dirty = true;
	this->uv = std::move(uv);
}

void Mesh::SetIndices(std::vector<uint32_t> indices) {
	assert(isEditable);
	dirty = true;
	this->indices = std::move(indices);
}

void Mesh::SetSubMeshCount(size_t num) {
	assert(isEditable);
	if (submeshes.size() < num) {
		for (size_t i = submeshes.size(); i < num; i++) {
			submeshes.emplace_back(
				static_cast<size_t>(-1),
				static_cast<size_t>(-1)
			);
		}
	}
	else {
		size_t cnt = submeshes.size() - num;
		for (size_t i = 0; i < cnt; i++)
			submeshes.pop_back();
	}
}

void Mesh::SetSubMesh(size_t index, SubMeshDescriptor desc) {
	assert(isEditable);
	assert(index < submeshes.size());
	dirty = true;
	desc.firstVertex = indices[desc.indexStart] + desc.baseVertex;
	desc.bounds = { positions[desc.firstVertex], positions[desc.firstVertex] };
	for (size_t i = 0; i < desc.indexCount; i++) {
		auto index = indices[desc.indexStart + i] + desc.baseVertex;
		desc.bounds.combine_to_self(positions[index]);
	}
	submeshes[index] = desc;
}
