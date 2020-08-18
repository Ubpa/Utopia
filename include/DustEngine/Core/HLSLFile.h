#pragma once

#include <string>

namespace Ubpa::DustEngine {
	class HLSLFile {
	public:
		HLSLFile(std::string str, std::string localDir)
			: str{ std::move(str) }, localDir{ std::move(localDir) }{}

		const std::string& GetString() const noexcept { return str; }
		const std::string& GetLocalDir() const noexcept { return localDir; }
	private:
		std::string str;
		std::string localDir;
	};
}
