#pragma once

#include "../Core/Object.h"

#include <string>

namespace Ubpa::Utopia {
	class HLSLFile : public Object {
	public:
		HLSLFile(std::string text, std::string localDir)
			: text{ std::move(text) }, localDir{ std::move(localDir) }{}

		const std::string& GetText() const noexcept { return text; }
		const std::string& GetLocalDir() const noexcept { return localDir; }
	private:
		std::string text;
		std::string localDir;
	};
}
