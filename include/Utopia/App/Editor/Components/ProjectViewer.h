#pragma once

#include <_deps/crossguid/guid.hpp>
#include <string>

namespace Ubpa::Utopia {
	struct ProjectViewer {
		xg::Guid selectedFolder;
		xg::Guid selectedAsset;
		xg::Guid hoveredAsset;

		bool isRenaming{ false };
		std::string rename;
	};
}
