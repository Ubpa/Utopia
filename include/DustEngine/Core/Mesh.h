#pragma once

#include "SubMeshDescriptor.h"
#include "Object.h"

#include <UGM/UGM.h>

#include <vector>

namespace Ubpa::DustEngine {
	class Mesh : public Object {
	public:
		Mesh(bool isEditable = true) : isEditable{ isEditable } {}

		const std::vector<pointf3>& GetPositions() const noexcept { return positions; }
		const std::vector<pointf2>& GetUV() const noexcept { return uv; }
		const std::vector<normalf>& GetNormals() const noexcept { return normals; }
		const std::vector<vecf3>& GetTangents() const noexcept { return tangents; }
		const std::vector<rgbf>& GetColors() const noexcept { return colors; }
		const std::vector<uint32_t>& GetIndices() const noexcept { return indices; }
		const std::vector<SubMeshDescriptor>& GetSubMeshes() const noexcept { return submeshes; }

		// must editable
		void SetPositions(std::vector<pointf3> positions);
		void SetUV(std::vector<pointf2> uv);
		void SetNormals(std::vector<normalf> normals);
		void SetTangents(std::vector<vecf3> tangents);
		void SetColors(std::vector<rgbf> colors);
		void SetIndices(std::vector<uint32_t> indices);
		void SetSubMeshCount(size_t num);
		void SetSubMesh(size_t index, SubMeshDescriptor desc);

		// must editable
		void GenNormals();
		void GenUV();
		void GenTangents();

		void SetToNonEditable() noexcept { isEditable = false; }

		bool IsDirty() const noexcept { return dirty; }

		bool IsEditable() const noexcept { return isEditable; }

		const void* GetVertexBufferData() const noexcept { return vertexBuffer.data(); }
		size_t GetVertexBufferVertexCount() const noexcept { return positions.size(); }
		size_t GetVertexBufferVertexStride() const noexcept { return vertexBuffer.size() / positions.size(); }

		// asset(IsDirty())
		// call by the engine, need to update GPU buffer
		void UpdateVertexBuffer();

		// non empty and every attributes have same num
		bool IsVertexValid();
	private:
		std::vector<pointf3> positions;
		std::vector<pointf2> uv;
		std::vector<normalf> normals;
		std::vector<vecf3> tangents;
		std::vector<rgbf> colors;
		std::vector<uint32_t> indices;
		std::vector<SubMeshDescriptor> submeshes;

		// pos, uv, normal, tangent, color
		std::vector<uint8_t> vertexBuffer;

		bool isEditable;
		bool dirty{ false };
	};
}
