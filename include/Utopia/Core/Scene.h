#pragma once

#include <string>

namespace Ubpa::Utopia {
	class Scene {
	public:
		Scene(std::string text) : text{ std::move(text) } {}
		const std::string& GetText() const noexcept { return text; }
	private:
		std::string text;
	};
}
