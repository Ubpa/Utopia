#pragma once

#include "RenderRsrcObject.h"

namespace Ubpa::Utopia {
	class Texture : public RenderRsrcObject {
	public:
		virtual ~Texture() = default;
	protected:
		Texture() = default;
	};
}
