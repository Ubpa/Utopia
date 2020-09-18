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

void Mesh::GenNormals() {
	normals.clear();
	normals.resize(positions.size(), normalf(0, 1, 0));

	for (const auto& submesh : GetSubMeshes()) {
		if (submesh.topology != MeshTopology::Triangles)
			continue;

		assert(submesh.indexCount % 3 == 0);
		for (size_t i = 0; i < submesh.indexCount; i += 3) {
			auto v0 = indices[submesh.indexStart + i + 0];
			auto v1 = indices[submesh.indexStart + i + 1];
			auto v2 = indices[submesh.indexStart + i + 2];

			auto d10 = positions.at(v0) - positions.at(v1);
			auto d12 = positions.at(v2) - positions.at(v1);
			auto wN = d12.cross(d10).cast_to<normalf>();

			normals.at(v0) += wN;
			normals.at(v1) += wN;
			normals.at(v2) += wN;
		}
	}

	for (auto& normal : normals)
		normal.normalize_self();
}

void Mesh::GenUV() {
	uv.resize(positions.size());
	pointf3 center = pointf3::combine(positions, 1.f / positions.size());
	for (size_t i = 0; i < positions.size(); i++)
		uv.at(i) = (positions[i] - center).normalize().cast_to<normalf>().to_sphere_texcoord();
}

void Mesh::GenTangents() {
	if (normals.empty())
		GenNormals();
	if (uv.empty())
		GenUV();

	const size_t vertexNum = positions.size();
	const size_t triangleCount = indices.size() / 3;

	std::vector<vecf3> tanS(vertexNum, vecf3{ 0,0,0 });
	std::vector<vecf3> tanT(vertexNum, vecf3{ 0,0,0 });

	for (const auto& submesh : GetSubMeshes()) {
		if (submesh.topology != MeshTopology::Triangles)
			continue;

		assert(submesh.indexCount % 3 == 0);
		for (size_t i = 0; i < submesh.indexCount; i += 3) {
			auto i1 = indices[submesh.indexStart + i + 0];
			auto i2 = indices[submesh.indexStart + i + 1];
			auto i3 = indices[submesh.indexStart + i + 2];

			const pointf3& v1 = positions.at(i1);
			const pointf3& v2 = positions.at(i2);
			const pointf3& v3 = positions.at(i3);

			const pointf2& w1 = uv.at(i1);
			const pointf2& w2 = uv.at(i2);
			const pointf2& w3 = uv.at(i3);

			float x1 = v2[0] - v1[0];
			float x2 = v3[0] - v1[0];
			float y1 = v2[1] - v1[1];
			float y2 = v3[1] - v1[1];
			float z1 = v2[2] - v1[2];
			float z2 = v3[2] - v1[2];

			float s1 = w2[0] - w1[0];
			float s2 = w3[0] - w1[0];
			float t1 = w2[1] - w1[1];
			float t2 = w3[1] - w1[1];

			float denominator = s1 * t2 - s2 * t1;
			float r = denominator == 0.f ? 1.f : 1.f / denominator;
			vecf3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r);
			vecf3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r);

			tanS[i1] += sdir;
			tanS[i2] += sdir;
			tanS[i3] += sdir;
			tanT[i1] += tdir;
			tanT[i2] += tdir;
			tanT[i3] += tdir;
		}
	}

	tangents.resize(vertexNum);
	for (size_t i = 0; i < vertexNum; i++) {
		const vecf3& n = normals.at(i).cast_to<vecf3>();
		const vecf3& t = tanS[i];

		// Gram-Schmidt orthogonalize
		auto projT = t - n * n.dot(t);
		tangents.at(i) = projT.norm2() == 0.f ? vecf3{ 1,0,0 } : projT.normalize();

		// Calculate handedness
		tangents.at(i) *= (n.cross(t).dot(tanT[i]) < 0.0F) ? -1.f : 1.f;
	}
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

void Mesh::UpdateVertexBuffer() {
	assert(IsDirty());

	assert(IsVertexValid());

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
