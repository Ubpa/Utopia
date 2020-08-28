#include <DustEngine/Core/ImGUIMngr.h>

#include <DustEngine/_deps/imgui/imgui.h>
#include <DustEngine/_deps/imgui/imgui_impl_dx12.h>
#include <DustEngine/_deps/imgui/imgui_impl_win32.h>

using namespace Ubpa::DustEngine;

void ImGUIMngr::Init(HWND hwnd, ID3D12Device* device, size_t numFrame, Style style) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	switch (style)
	{
	case Ubpa::DustEngine::ImGUIMngr::Style::Dark:
		ImGui::StyleColorsDark();
		break;
	case Ubpa::DustEngine::ImGUIMngr::Style::Classic:
		ImGui::StyleColorsClassic();
		break;
	default:
		break;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hwnd);
	fontDH = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	ImGui_ImplDX12_Init(device, static_cast<int>(numFrame),
		DXGI_FORMAT_R8G8B8A8_UNORM, Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap(),
		fontDH.GetCpuHandle(),
		fontDH.GetGpuHandle()
	);
}

void ImGUIMngr::Clear() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	
	if (!fontDH.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(fontDH));
}
