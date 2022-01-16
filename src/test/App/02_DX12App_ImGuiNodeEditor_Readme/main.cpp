#include <Utopia/App/DX12App/DX12App.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui_node_editor/imgui_node_editor.h>

using namespace Ubpa::Utopia;
namespace ed = ax::NodeEditor;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ImGuiNodeEditorReadmeApp : public DX12App {
    Ubpa::UDX12::DescriptorHeapAllocation fontDH;

    ed::EditorContext* edctx = nullptr;

public:
    ImGuiNodeEditorReadmeApp(HINSTANCE hInstance) : DX12App(hInstance) {
        mMainWndCaption = L"ImGui Node Editor Readme App";
        edctx = ed::CreateEditor();
    }

    ~ImGuiNodeEditorReadmeApp() {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        ed::DestroyEditor(edctx);
    }

    virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        return DX12App::MsgProc(hWnd, msg, wParam, lParam);
    }

    virtual bool Init() override {
        if (!DX12App::Init())
            return false;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        fontDH = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(MainWnd());
        ImGui_ImplDX12_Init(uDevice.Get(), DX12App::NumFrameResources,
            DXGI_FORMAT_R8G8B8A8_UNORM, Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap(),
            fontDH.GetCpuHandle(),
            fontDH.GetGpuHandle());

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        return true;
    }

    virtual void Update() override {
        GetFrameResourceMngr()->BeginFrame();
        ID3D12CommandAllocator* cmdAlloc = GetCurFrameCommandAllocator();
        cmdAlloc->Reset();
        ThrowIfFailed(uGCmdList->Reset(cmdAlloc, nullptr));

        {
            // Start the Dear ImGui frame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            { // node editor
                ed::SetCurrentEditor(edctx);

                ed::Begin("My Editor");

                int uniqueId = 1;

                // Start drawing nodes.
                ed::BeginNode(uniqueId++);
                    ImGui::Text("Node A");
                    ed::BeginPin(uniqueId++, ed::PinKind::Input);
                        ImGui::Text("-> In");
                    ed::EndPin();
                    ImGui::SameLine();
                    ed::BeginPin(uniqueId++, ed::PinKind::Output);
                        ImGui::Text("Out ->");
                    ed::EndPin();
                ed::EndNode();

                ed::End();
            }

            // Rendering
            ImGui::Render();
        }

        uGCmdList.ResourceBarrierTransition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        uGCmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightBlue, 0, NULL);

        const D3D12_CPU_DESCRIPTOR_HANDLE curBack = CurrentBackBufferView();
        uGCmdList->OMSetRenderTargets(1, &curBack, FALSE, NULL);
        uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), uGCmdList.Get());

        uGCmdList.ResourceBarrierTransition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        uGCmdList->Close();
        uCmdQueue.Execute(uGCmdList.Get());
        SwapBackBuffer();
        GetFrameResourceMngr()->EndFrame(uCmdQueue.Get());

        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(NULL, uGCmdList.Get());
        }
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	int rst;
    try {
        ImGuiNodeEditorReadmeApp app(hInstance);
        if(!app.Init())
            return 1;
        
		rst = app.Run();
    }
    catch(Ubpa::UDX12::Util::Exception& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        rst = 1;
	}

	return rst;
}
