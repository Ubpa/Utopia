#pragma once

namespace Ubpa::Utopia {
	constexpr size_t MeshLayoutMngr::GetMeshLayoutID(
		bool uv,
		bool normal,
		bool tangent,
		bool color
	) noexcept {
		size_t rst = 0;
		rst |= (static_cast<size_t>(uv     ) << 0);
		rst |= (static_cast<size_t>(normal ) << 1);
		rst |= (static_cast<size_t>(tangent) << 2);
		rst |= (static_cast<size_t>(color  ) << 3);
		return rst;
	}

	constexpr std::array<bool, 4> MeshLayoutMngr::DecodeMeshLayoutID(size_t ID) noexcept {
		bool uv      = static_cast<bool>(ID & (1 << 0));
		bool normal  = static_cast<bool>(ID & (1 << 1));
		bool tangent = static_cast<bool>(ID & (1 << 2));
		bool color   = static_cast<bool>(ID & (1 << 3));
		return { uv,normal,tangent,color };
	}
}
