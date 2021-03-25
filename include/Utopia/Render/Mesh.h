#pragma once

#include "SubMeshDescriptor.h"

#include "GPURsrc.h"

#include <UGM/UGM.hpp>

#include <vector>

namespace Ubpa::Utopia {
	// Non-editable mesh is space-saving (release CPU vertex buffer and GPU upload buffer).
	// If you want to change any data of the mesh, you should call SetToEditable() firstly.
	// After editing, you can call SetToNonEditable() to release buffers.
	class Mesh : public GPURsrc {
	public:
		Mesh() {}

		const std::vector<pointf3>&           GetPositions() const noexcept { return positions; }
		const std::vector<pointf2>&           GetUV() const noexcept { return uv; }
		const std::vector<normalf>&           GetNormals() const noexcept { return normals; }
		const std::vector<vecf3>&             GetTangents() const noexcept { return tangents; }
		const std::vector<rgbf>&              GetColors() const noexcept { return colors; }
		const std::vector<uint32_t>&          GetIndices() const noexcept { return indices; }
		const std::vector<SubMeshDescriptor>& GetSubMeshes() const noexcept { return submeshes; }

		// must editable
		void SetPositions(std::vector<pointf3> positions) noexcept;
		void SetUV(std::vector<pointf2> uv) noexcept;
		void SetNormals(std::vector<normalf> normals) noexcept;
		void SetTangents(std::vector<vecf3> tangents) noexcept;
		void SetColors(std::vector<rgbf> colors) noexcept;
		void SetIndices(std::vector<uint32_t> indices) noexcept;
		void SetSubMeshCount(size_t num);
		void SetSubMesh(size_t index, SubMeshDescriptor desc) noexcept; // index < GetSubMeshes().size()

		// must editable
		void GenNormals();
		void GenUV(); // naive uv
		void GenTangents();

		// non empty and every attributes have same num
		bool IsVertexValid() const noexcept;

	private:
		friend class GPURsrcMngrDX12;

		// call by the GPURsrcMngrDX12, need to update GPU buffer in the meantime
		const void* GetVertexBufferData();
		size_t GetVertexBufferVertexCount() const noexcept { return positions.size(); }
		size_t GetVertexBufferVertexStride();
		void ClearVertexBuffer();
		void UpdateVertexBuffer();

		std::vector<pointf3> positions;
		std::vector<pointf2> uv;
		std::vector<normalf> normals;
		std::vector<vecf3> tangents;
		std::vector<rgbf> colors;
		std::vector<uint32_t> indices;
		std::vector<SubMeshDescriptor> submeshes;

		// pos, uv, normal, tangent, color
		std::vector<uint8_t> vertexBuffer;
		bool vertexBuffer_dirty{ true };
	};
}
