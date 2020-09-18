#pragma once

#include <_deps/crossguid/guid.hpp>

namespace Ubpa::DustEngine {
	struct ProjectViewer {
		xg::Guid selectedFolder;
		xg::Guid selectedAsset;
	};
}
