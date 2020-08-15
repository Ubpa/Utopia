#pragma once

#include "SubMeshDescriptor.h"

#include <UGM/UGM.h>

#include <vector>

namespace Ubpa::DustEngine {
	class Mesh {
	public:
		Mesh(bool isEditable = true) : isEditable{ isEditable } {}

		const std::vector<pointf3>& GetPositions() const noexcept { return positions; }
		const std::vector<rgbf>& GetColors() const noexcept { return colors; }
		const std::vector<normalf>& GetNormals() const noexcept { return normals; }
		const std::vector<vecf3>& GetTangents() const noexcept { return tangents; }
		const std::vector<pointf2>& GetUV() const noexcept { return uv; }
		const std::vector<uint32_t>& GetIndices() const noexcept { return indices; }
		const std::vector<SubMeshDescriptor>& GetSubMeshes() const noexcept { return submeshes; }

		// must editable
		void SetPositions(std::vector<pointf3> positions);
		void SetColors(std::vector<rgbf> colors);
		void SetNormals(std::vector<normalf> normals);
		void SetTangents(std::vector<vecf3> tangents);
		void SetUV(std::vector<pointf2> uv);
		void SetIndices(std::vector<uint32_t> indices);
		void SetSubMeshCount(size_t num);
		void SetSubMesh(size_t index, SubMeshDescriptor desc);

		bool IsEditable() const noexcept { return isEditable; }
	private:
		std::vector<pointf3> positions;
		std::vector<rgbf> colors;
		std::vector<normalf> normals;
		std::vector<vecf3> tangents;
		std::vector<pointf2> uv;
		std::vector<uint32_t> indices;
		std::vector<SubMeshDescriptor> submeshes;

		bool isEditable;
		bool dirty{ false };
	};
}
