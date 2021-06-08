#include <Utopia/Render/DX12/MeshLayoutMngr.h>

#include <Utopia/Render/Mesh.h>

using namespace Ubpa::Utopia;

std::vector<D3D12_INPUT_ELEMENT_DESC> MeshLayoutMngr::GenerateDesc(
	bool uv,
	bool normal,
	bool tangent,
	bool color
) {
	std::vector<D3D12_INPUT_ELEMENT_DESC> rst;

	UINT offset = 0;

	rst.reserve(5);

	rst.push_back(
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	);
	offset += 12;

	if (uv) {
		rst.push_back(
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 8;
	}
	else {
		rst.push_back(
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
	}

	if (normal) {
		rst.push_back(
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 12;
	}
	else {
		rst.push_back(
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
	}

	if (tangent) {
		rst.push_back(
			{ "TENGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 12;
	}
	else{
		rst.push_back(
			{ "TENGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
	}

	if (color) {
		rst.push_back(
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 16;
	}
	else {
		rst.push_back(
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
	}

	return rst;
}

MeshLayoutMngr::MeshLayoutMngr() {
	layoutMap.reserve(0b1111);
	for (size_t ID = 0; ID <= 0b1111; ID++) {
		auto [uv, normal, tangent, color] = DecodeMeshLayoutID(ID);
		layoutMap.emplace(ID, GenerateDesc(uv, normal, tangent, color));
	}
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& MeshLayoutMngr::GetMeshLayoutValue(size_t ID) {
	auto target = layoutMap.find(ID);
	assert(target != layoutMap.end());
	return target->second;
}

size_t MeshLayoutMngr::GetMeshLayoutID(const Mesh& mesh) noexcept {
	return GetMeshLayoutID(
		!mesh.GetUV().empty(),
		!mesh.GetNormals().empty(),
		!mesh.GetTangents().empty(),
		!mesh.GetColors().empty()
	);
}
