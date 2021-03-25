#pragma once

#include "GPURsrc.h"

namespace Ubpa::Utopia {
	class Texture : public GPURsrc {
	public:
		virtual ~Texture() = default;
	protected:
		Texture() = default;
	};
}
