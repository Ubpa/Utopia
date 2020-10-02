#pragma once

#include <_deps/crossguid/guid.hpp>

namespace Ubpa::Utopia {
	struct ProjectViewer {
		xg::Guid selectedFolder;
		xg::Guid selectedAsset;
	};
}
