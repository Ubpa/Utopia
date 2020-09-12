#pragma once

#include <DustEngine/_deps/crossguid/guid.hpp>

namespace Ubpa::DustEngine {
	struct ProjectViewer {
		xg::Guid selectedFolder;
		xg::Guid selectedAsset;
	};
}
