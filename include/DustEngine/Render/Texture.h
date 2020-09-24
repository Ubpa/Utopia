#pragma once

#include "RenderRsrcObject.h"

namespace Ubpa::DustEngine {
	class Texture : public RenderRsrcObject {
	public:
		virtual ~Texture() = default;
	protected:
		Texture() = default;
	};
}
