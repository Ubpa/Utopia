#include <Utopia/Core/ImGUIMngr.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_dx12.h>
#include <_deps/imgui/imgui_impl_win32.h>

using namespace Ubpa::Utopia;

struct ImGUIMngr::Impl {
	void* hwnd;
	ID3D12Device* device;
	size_t numFrames;
	struct ImGuiContextData {
		Ubpa::UDX12::DescriptorHeapAllocation fontDH;
		ImGuiContext* ctx;
	};
	std::map<std::string, ImGuiContextData, std::less<>> contextDatas;
};

ImGUIMngr::ImGUIMngr()
	: pImpl{ new Impl }
{}

ImGUIMngr::~ImGUIMngr() {
	delete pImpl;
}

void ImGUIMngr::Init(void* hwnd, ID3D12Device* device, size_t numFrames) {
	IMGUI_CHECKVERSION();
	assert(hwnd && device && numFrames > 0);

	pImpl->hwnd = hwnd;
	pImpl->device = device;
	pImpl->numFrames = numFrames;
}

ImGuiContext* ImGUIMngr::CreateContext(std::string name, bool supportViewports, StyleColors styleColors) {
	ImGui::WrapContextGuard ImGuiContextGuard(nullptr);

	auto ctx = ImGui::CreateContext();
	assert(ctx == ImGui::GetCurrentContext());
	ImGuiIO& io = ImGui::GetIO();
	if(supportViewports)
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	switch (styleColors)
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

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	Impl::ImGuiContextData contextData;
	contextData.ctx = ctx;
	contextData.fontDH = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);

	ImGui_ImplWin32_Init(pImpl->hwnd);
	ImGui_ImplDX12_Init(
		pImpl->device,
		(int)pImpl->numFrames,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap(),
		contextData.fontDH.GetCpuHandle(),
		contextData.fontDH.GetGpuHandle());

	pImpl->contextDatas.emplace(std::move(name), std::move(contextData));

	return ctx;
}

ImGuiContext* ImGUIMngr::GetContext(std::string_view name) const {
	auto target = pImpl->contextDatas.find(name);
	if (target == pImpl->contextDatas.end())
		return nullptr;

	return target->second.ctx;
}

void ImGUIMngr::DestroyContext(std::string_view name) {
	auto target = pImpl->contextDatas.find(name);
	if (target == pImpl->contextDatas.end())
		return;

	auto ctx = target->second.ctx;
	ImGui::WrapContextGuard ImGuiContextGuard(ctx);

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(ctx);

	pImpl->contextDatas.erase(target);
}

void ImGUIMngr::Clear() {
	for (auto& [name, contextData] : pImpl->contextDatas)
	{
		ImGui::WrapContextGuard ImGuiContextGuard(contextData.ctx);
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(contextData.ctx);
		contextData.ctx = nullptr;

		if (!contextData.fontDH.IsNull())
			Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(contextData.fontDH));
	}

	pImpl->contextDatas.clear();
}
