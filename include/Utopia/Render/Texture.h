#pragma once

#include "../Core/Object.h"

namespace Ubpa::Utopia {
	class Texture : public Object {
	public:
		virtual ~Texture() = default;
	protected:
		Texture() = default;
	};
}
