#pragma once

#include <UDX12/UDX12.h>

namespace Ubpa::DustEngine {
	class ImGUIMngr {
	public:
		static ImGUIMngr& Instance() noexcept {
			static ImGUIMngr instance;
			return instance;
		}

		enum class Style {
			Dark,
			Classic
		};

		void Init(HWND, ID3D12Device*, size_t numFrame, Style = Style::Dark);
		void Clear();

	private:
		Ubpa::UDX12::DescriptorHeapAllocation fontDH;
	};
}
