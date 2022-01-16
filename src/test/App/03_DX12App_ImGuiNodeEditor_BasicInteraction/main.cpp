#include <Utopia/App/DX12App/DX12App.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui_node_editor/imgui_node_editor.h>

using namespace Ubpa::Utopia;
namespace ed = ax::NodeEditor;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ImGuiNodeEditorBasicInteractionApp : public DX12App {
    Ubpa::UDX12::DescriptorHeapAllocation fontDH;
    
    // Struct to hold basic information about connection between
    // pins. Note that connection (aka. link) has its own ID.
    // This is useful later with dealing with selections, deletion
    // or other operations.
    struct LinkInfo
    {
        ed::LinkId Id;
        ed::PinId  InputId;
        ed::PinId  OutputId;
    };

    ed::EditorContext*   m_Context = nullptr;    // Editor context, required to trace a editor state.
    bool                 m_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
    ImVector<LinkInfo>   m_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
    int                  m_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.

    static void ImGuiEx_BeginColumn()
    {
        ImGui::BeginGroup();
    }

    static void ImGuiEx_NextColumn()
    {
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
    }

    static void ImGuiEx_EndColumn()
    {
        ImGui::EndGroup();
    }

public:
    ImGuiNodeEditorBasicInteractionApp(HINSTANCE hInstance) : DX12App(hInstance) {
        mMainWndCaption = L"ImGui Node Editor Basic Interaction App";

        ed::Config config;
        config.SettingsFile = "BasicInteraction.json";
        m_Context = ed::CreateEditor(&config);
    }

    ~ImGuiNodeEditorBasicInteractionApp() {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        ed::DestroyEditor(m_Context);
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
                ed::SetCurrentEditor(m_Context);

                // Start interaction with editor.
                ed::Begin("My Editor", ImVec2(0.0, 0.0f));

                int uniqueId = 1;

                //
                // 1) Commit known data to editor
                //

                // Submit Node A
                ed::NodeId nodeA_Id = uniqueId++;
                ed::PinId  nodeA_InputPinId = uniqueId++;
                ed::PinId  nodeA_OutputPinId = uniqueId++;

                if (m_FirstFrame)
                    ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
                ed::BeginNode(nodeA_Id);
                    ImGui::Text("Node A");
                    ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
                        ImGui::Text("-> In");
                    ed::EndPin();
                    ImGui::SameLine();
                    ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
                        ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGui::Button("Hello world!");
                ed::EndNode();

                // Submit Node B
                ed::NodeId nodeB_Id = uniqueId++;
                ed::PinId  nodeB_InputPinId1 = uniqueId++;
                ed::PinId  nodeB_InputPinId2 = uniqueId++;
                ed::PinId  nodeB_OutputPinId = uniqueId++;

                if (m_FirstFrame)
                    ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));
                ed::BeginNode(nodeB_Id);
                    ImGui::Text("Node B");
                    ImGuiEx_BeginColumn();
                        ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
                            ImGui::Text("-> In1");
                        ed::EndPin();
                        ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
                            ImGui::Text("-> In2");
                        ed::EndPin();
                    ImGuiEx_NextColumn();
                        ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
                            ImGui::Text("Out ->");
                        ed::EndPin();
                    ImGuiEx_EndColumn();
                ed::EndNode();

                // Submit Links
                for (auto& linkInfo : m_Links)
                    ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

                //
                // 2) Handle interactions
                //

                // Handle creation action, returns true if editor want to create new object (node or link)
                if (ed::BeginCreate())
                {
                    ed::PinId inputPinId, outputPinId;
                    if (ed::QueryNewLink(&inputPinId, &outputPinId))
                    {
                        // QueryNewLink returns true if editor want to create new link between pins.
                        //
                        // Link can be created only for two valid pins, it is up to you to
                        // validate if connection make sense. Editor is happy to make any.
                        //
                        // Link always goes from input to output. User may choose to drag
                        // link from output pin or input pin. This determine which pin ids
                        // are valid and which are not:
                        //   * input valid, output invalid - user started to drag new ling from input pin
                        //   * input invalid, output valid - user started to drag new ling from output pin
                        //   * input valid, output valid   - user dragged link over other pin, can be validated

                        if (inputPinId && outputPinId) // both are valid, let's accept link
                        {
                            // ed::AcceptNewItem() return true when user release mouse button.
                            if (ed::AcceptNewItem())
                            {
                                // Since we accepted new link, lets add one to our list of links.
                                m_Links.push_back({ ed::LinkId(m_NextLinkId++), inputPinId, outputPinId });

                                // Draw new link.
                                ed::Link(m_Links.back().Id, m_Links.back().InputId, m_Links.back().OutputId);
                            }

                            // You may choose to reject connection between these nodes
                            // by calling ed::RejectNewItem(). This will allow editor to give
                            // visual feedback by changing link thickness and color.
                        }
                    }
                }
                ed::EndCreate(); // Wraps up object creation action handling.


                // Handle deletion action
                if (ed::BeginDelete())
                {
                    // There may be many links marked for deletion, let's loop over them.
                    ed::LinkId deletedLinkId;
                    while (ed::QueryDeletedLink(&deletedLinkId))
                    {
                        // If you agree that link can be deleted, accept deletion.
                        if (ed::AcceptDeletedItem())
                        {
                            // Then remove link from your data.
                            for (auto& link : m_Links)
                            {
                                if (link.Id == deletedLinkId)
                                {
                                    m_Links.erase(&link);
                                    break;
                                }
                            }
                        }

                        // You may reject link deletion by calling:
                        // ed::RejectDeletedItem();
                    }
                }
                ed::EndDelete(); // Wrap up deletion action



                // End of interaction with editor.
                ed::End();

                if (m_FirstFrame)
                    ed::NavigateToContent(0.0f);

                ed::SetCurrentEditor(nullptr);

                m_FirstFrame = false;
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
        ImGuiNodeEditorBasicInteractionApp app(hInstance);
        if (!app.Init())
            return 1;

        rst = app.Run();
    }
    catch (Ubpa::UDX12::Util::Exception& e) {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        rst = 1;
    }

    return rst;
}
