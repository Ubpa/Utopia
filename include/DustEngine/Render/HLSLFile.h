#pragma once

#include <string>

namespace Ubpa::DustEngine {
	class HLSLFile {
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
