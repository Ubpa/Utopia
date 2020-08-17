#pragma once

#include "Object.h"

namespace Ubpa::DustEngine {
	class Texture : public Object {
	public:
		virtual ~Texture() = default;
	protected:
		Texture() = default;
	};
}
