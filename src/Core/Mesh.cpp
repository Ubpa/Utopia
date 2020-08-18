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

bool Mesh::IsVertexValid() {
	size_t num = positions.size();
	if (num != 0
		&& (uv.size() == 0 || uv.size() == num)
		&& (normals.size() == 0 || normals.size() == num)
		&& (tangents.size() == 0 || tangents.size() == num)
		&& (colors.size() == 0 || colors.size() == num)
	)
		return true;
	else
		return false;
}

void Mesh::UpdateVertexBuffer(bool setToNonEditable) {
	assert(IsVertexValid());

	if (setToNonEditable)
		isEditable = false;

	if (!IsDirty())
		return;

	size_t num = GetVertexBufferVertexCount();

	size_t stride = 0;
	stride += sizeof(decltype(positions)::value_type);
	if (!uv.empty()) stride += sizeof(decltype(uv)::value_type);
	if (!normals.empty()) stride += sizeof(decltype(normals)::value_type);
	if (!tangents.empty()) stride += sizeof(decltype(tangents)::value_type);
	if (!colors.empty()) stride += sizeof(decltype(colors)::value_type);

	vertexBuffer.resize(stride * positions.size());

	size_t offset = 0;
	uint8_t* data = vertexBuffer.data();
	for (size_t i = 0; i < num; i++) {
		memcpy(data + offset, positions[i].data(), sizeof(decltype(positions)::value_type));
		offset += sizeof(decltype(positions)::value_type);

		if (!uv.empty()) {
			memcpy(data + offset, uv[i].data(), sizeof(decltype(uv)::value_type));
			offset += sizeof(decltype(uv)::value_type);
		}

		if (!normals.empty()) {
			memcpy(data + offset, normals[i].data(), sizeof(decltype(normals)::value_type));
			offset += sizeof(decltype(normals)::value_type);
		}

		if (!tangents.empty()) {
			memcpy(data + offset, tangents[i].data(), sizeof(decltype(tangents)::value_type));
			offset += sizeof(decltype(tangents)::value_type);
		}

		if (!colors.empty()) {
			memcpy(data + offset, colors[i].data(), sizeof(decltype(colors)::value_type));
			offset += sizeof(decltype(colors)::value_type);
		}
	}

	dirty = false;
}
