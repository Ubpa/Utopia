#include <Utopia/Core/ImGUIMngr.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_dx12.h>
#include <_deps/imgui/imgui_impl_win32.h>

using namespace Ubpa::Utopia;

struct ImGUIMngr::Impl {
	Ubpa::UDX12::DescriptorHeapAllocation fontDH;
	std::map<std::string, ImGuiContext*, std::less<>> contexts;
	ImFontAtlas sharedFontAtlas;
};

ImGUIMngr::ImGUIMngr()
	: pImpl{ new Impl }
{}

ImGUIMngr::~ImGUIMngr() {
	delete pImpl;
}

void ImGUIMngr::Init(void* hwnd, ID3D12Device* device, size_t numFrames) {
	IMGUI_CHECKVERSION();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init_Shared(hwnd);

	pImpl->fontDH = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	ImGui_ImplDX12_Init_Shared(
		device,
		static_cast<int>(numFrames),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap(),
		pImpl->fontDH.GetCpuHandle(),
		pImpl->fontDH.GetGpuHandle()
	);

	ImGui_ImplDX12_SetSharedFontAtlas(&pImpl->sharedFontAtlas);
}

ImGuiContext* ImGUIMngr::CreateContext(std::string name, StyleColors style) {
	auto ctx = ImGui::CreateContext(&pImpl->sharedFontAtlas);
	ImGui_ImplWin32_Init_Context(ctx);
	ImGui_ImplDX12_Init_Context(ctx);
	switch (style)
	{
	case StyleColors::Classic:
		ImGui::StyleColorsClassic();
		break;
	case StyleColors::Dark:
		ImGui::StyleColorsDark();
		break;
	case StyleColors::Light:
		ImGui::StyleColorsLight();
		break;
	default:
		break;
	}
	pImpl->contexts.emplace(std::move(name), ctx);

	return ctx;
}

ImGuiContext* ImGUIMngr::GetContext(std::string_view name) const {
	auto target = pImpl->contexts.find(name);
	if (target == pImpl->contexts.end())
		return nullptr;

	return target->second;
}

void ImGUIMngr::DestroyContext(std::string_view name) {
	auto target = pImpl->contexts.find(name);
	if (target == pImpl->contexts.end())
		return;

	auto ctx = target->second;

	ImGui_ImplDX12_Shutdown_Context(ctx);
	ImGui_ImplWin32_Shutdown_Context(ctx);
	ImGui::DestroyContext(ctx);

	pImpl->contexts.erase(target);
}

void ImGUIMngr::Clear() {
	for(const auto& [name, ctx] : pImpl->contexts)
		ImGui_ImplDX12_Shutdown_Context(ctx);

	ImGui_ImplDX12_Shutdown_Shared();

	ImGui_ImplWin32_Shutdown_Shared();
	for (const auto& [name, ctx] : pImpl->contexts)
		ImGui_ImplWin32_Shutdown_Context(ctx);

	for (const auto& [name, ctx] : pImpl->contexts)
		ImGui::DestroyContext(ctx);

	pImpl->contexts.clear();
	
	if (!pImpl->fontDH.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(pImpl->fontDH));
}
