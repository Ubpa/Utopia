#pragma once

#include <string>

namespace Ubpa::Utopia {
	// txt, json
	class TextAsset {
	public:
		TextAsset(std::string text) : text{ std::move(text) } {}
		const std::string& GetText() const noexcept { return text; }
	private:
		std::string text;
	};
}
