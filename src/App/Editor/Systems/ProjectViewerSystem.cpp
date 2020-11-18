#include <Utopia/App/Editor/Systems/ProjectViewerSystem.h>

#include <Utopia/App/Editor/PlayloadType.h>

#include <Utopia/App/Editor/Components/ProjectViewer.h>
#include <Utopia/App/Editor/Components/Inspector.h>

#include <Utopia/Asset/AssetMngr.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/DX12/RsrcMngrDX12.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/misc/cpp/imgui_stdlib.h>

using namespace Ubpa::Utopia;

namespace Ubpa::Utopia::detail {
	bool IsFolderLeaf(const xg::Guid& folderGuid) {
		const auto& tree = AssetMngr::Instance().GetAssetTree();
		auto target = tree.find(folderGuid);
		if (target == tree.end())
			return true;

		for (const auto& child : target->second) {
			if (std::filesystem::is_directory(AssetMngr::Instance().GUIDToAssetPath(child)))
				return false;
		}

		return true;
	}

	bool IsParentFolder(const std::filesystem::path& x, const std::filesystem::path& y) {
		if (y.empty())
			return false;
		else {
			const auto p = y.parent_path();
			if (p == x)
				return true;
			else
				return IsParentFolder(x, p);
		}
	}

	void NameShrink(std::string& name, size_t maxlength) {
		if (name.size() <= maxlength)
			return;
		name.resize(maxlength);
		name[maxlength - 2] = '.';
		name[maxlength - 1] = '.';
	}

	void ProjectViewerSystemPrintChildren(ProjectViewer* viewer, const xg::Guid& guid) {
		static constexpr ImGuiTreeNodeFlags nodeBaseFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		const auto& tree = AssetMngr::Instance().GetAssetTree();

		const auto& selectFolderPath = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedFolder);
		const auto& children = tree.find(guid)->second;

		for (const auto& child : children) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			if (!std::filesystem::is_directory(path))
				continue;
			bool isFolderLeaf = IsFolderLeaf(child);

			auto name = path.filename();

			ImGuiTreeNodeFlags nodeFlags = nodeBaseFlags;
			if (viewer->selectedFolder == child)
				nodeFlags |= ImGuiTreeNodeFlags_Selected;
			if (isFolderLeaf)
				nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if (IsParentFolder(path, selectFolderPath))
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);

			bool nodeOpen = ImGui::TreeNodeEx(
				(const void*)AssetMngr::Instance().LoadAsset(path)->GetInstanceID(),
				nodeFlags,
				"%s", name.string().c_str()
			);

			if (ImGui::IsItemClicked())
				viewer->selectedFolder = child;

			if (nodeOpen && !isFolderLeaf) {
				ProjectViewerSystemPrintChildren(viewer, child);
				ImGui::TreePop();
			}
		}
	}

	void ProjectViewerSystemPrintFolder(Inspector* inspector, ProjectViewer* viewer) {
		const auto& tree = AssetMngr::Instance().GetAssetTree();
		if (!viewer->selectedFolder.isValid())
			return;

		std::vector<std::filesystem::path> paths;
		auto curPath = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedFolder);
		while (curPath != AssetMngr::Instance().GetRootPath() && !curPath.empty()) {
			paths.emplace_back(curPath);
			curPath = curPath.parent_path();
		}
		for (size_t i = 0; i < paths.size(); i++) {
			size_t idx = paths.size() - 1 - i;
			const auto& path = paths[idx];
			if (idx > 0) {
				if (ImGui::SmallButton(path.stem().string().c_str()))
					viewer->selectedFolder = AssetMngr::Instance().AssetPathToGUID(path);
				ImGui::SameLine();
				ImGui::Text(">");
				ImGui::SameLine();
			}
			else
				ImGui::Text(path.stem().string().c_str());
		}
		ImGui::Separator();

		const auto& children = tree.find(viewer->selectedFolder)->second;
		auto file = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\file.tex2d");
		auto folder = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\folder.tex2d");
		auto code = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\code.tex2d");
		auto image = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\image.tex2d");
		auto material = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\material.tex2d");
		auto shader = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\shader.tex2d");
		auto hlsl = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\hlsl.tex2d");
		auto scene = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\scene.tex2d");
		auto model = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\model.tex2d");
		auto texcube = AssetMngr::Instance().LoadAsset<Texture2D>(L"..\\assets\\_internal\\FolderViewer\\textures\\texcube.tex2d");

		auto fileID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*file);
		auto folderID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*folder);
		auto codeID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*code);
		auto imageID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*image);
		auto materialID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*material);
		auto shaderID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*shader);
		auto hlslID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*hlsl);
		auto sceneID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*scene);
		auto modelID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*model);
		auto texcubeID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*texcube);

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 button_sz(64, 64);
		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		UINT idx = 0;

		// folder first
		std::vector<xg::Guid> childQueue;
		childQueue.reserve(children.size());

		for (const auto& child : children) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			if (std::filesystem::is_directory(path))
				childQueue.push_back(child);
		}
		for (const auto& child : children) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			if (!std::filesystem::is_directory(path))
				childQueue.push_back(child);
		}

		for (const auto& child : childQueue) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			const bool isDir = std::filesystem::is_directory(path);
			ImGui::PushID(reinterpret_cast<const void* const &>(child.bytes()));
			auto ext = path.extension();
			auto name = path.stem();

			ImGui::BeginGroup();
			{
				ImVec4 tint = viewer->selectedAsset == child ? ImVec4{ 0.65f, 0.65f, 0.65f, 1.f } : ImVec4{ 1,1,1,1 };
				UINT64 id;
				if (!isDir) {
					if (ext == ".lua")
						id = codeID.ptr;
					else if (
						ext == ".png"
						|| ext == ".jpg"
						|| ext == ".bmp"
						|| ext == ".hdr"
						|| ext == ".tga"
						) {
						id = imageID.ptr;
					}
					else if (ext == ".tex2d") {
						auto tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(path);
						Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTexture2D(
							*tex2d
						);
						id = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*tex2d).ptr;
					}
					else if (ext == ".mat")
						id = materialID.ptr;
					else if (ext == ".shader")
						id = shaderID.ptr;
					else if (ext == ".hlsl")
						id = hlslID.ptr;
					else if (ext == ".scene")
						id = sceneID.ptr;
					else if (
						ext == ".obj"
						|| ext == ".ply" && AssetMngr::Instance().IsSupported("ply")
						) {
						id = modelID.ptr;
					}
					else if (ext == ".texcube")
						id = texcubeID.ptr;
					else
						id = fileID.ptr;
				}
				else
					id = folderID.ptr;
				
				if (ImGui::ImageButton(ImTextureID(id), button_sz, { 0,0 }, { 1,1 }, 0, style.Colors[ImGuiCol_WindowBg], tint)) {
					viewer->selectedAsset = child;
					viewer->isRenaming = false;
					if (inspector && !inspector->lock) {
						inspector->mode = Inspector::Mode::Asset;
						inspector->asset = child;
					}
				}
				if (ImGui::IsItemHovered()) {
					viewer->hoveredAsset = child;
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						viewer->selectedAsset = child;
						viewer->isRenaming = false;
					}
				}
				if (isDir && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					viewer->selectedFolder = child;
					viewer->isRenaming = false;
				}
				auto nameStr = name.string();
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					ImGui::SetDragDropPayload(PlayloadType::GUID, &child, sizeof(xg::Guid));
					ImGui::ImageButton(ImTextureID(id), { 32,32 });
					ImGui::Text(nameStr.c_str());
					ImGui::EndDragDropSource();
				}
				if (viewer->selectedAsset == child && viewer->isRenaming) {
					ImGui::SetNextItemWidth(64.f);
					if (ImGui::InputText("", &viewer->rename, ImGuiInputTextFlags_EnterReturnsTrue)) {
						viewer->isRenaming = false;
						if (!viewer->rename.empty()) {
							std::filesystem::path newpath = path.parent_path().wstring()
								+ L"\\"
								+ std::filesystem::path(viewer->rename).wstring()
								+ ext.wstring();
							if (!std::filesystem::exists(newpath) && AssetMngr::Instance().MoveAsset(path, newpath))
								nameStr = viewer->rename;
						}
					}
				}
				else {
					NameShrink(nameStr, 9);
					ImGui::Text(nameStr.c_str());
				}
			}
			ImGui::EndGroup();

			float last_x = ImGui::GetItemRectMax().x;
			float next_x = last_x + style.ItemSpacing.x + button_sz.x;
			if (idx + 1 < children.size() && next_x < window_visible_x2)
				ImGui::SameLine();

			idx++;
			ImGui::PopID();
		}
	}
}

void ProjectViewerSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterCommand([](UECS::World* w) {
		auto viewer = w->entityMngr.GetSingleton<ProjectViewer>();
		auto inspector = w->entityMngr.GetSingleton<Inspector>();

		if (!viewer)
			return;

		viewer->hoveredAsset = xg::Guid{};
		
		if (ImGui::Begin("Project"))
			detail::ProjectViewerSystemPrintChildren(viewer, xg::Guid{});
		ImGui::End();

		if (ImGui::Begin("Folder")) {
			detail::ProjectViewerSystemPrintFolder(inspector, viewer);
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				if (viewer->hoveredAsset.isValid())
					ImGui::OpenPopup("Folder_Item_Popup");
				else
					ImGui::OpenPopup("Folder_Popup");
			}
			if (ImGui::BeginPopup("Folder_Item_Popup")) {
				if (ImGui::MenuItem("Rename")) {
					viewer->isRenaming = true;
					viewer->rename = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedAsset).stem().string();
				}
				/*if (ImGui::MenuItem("Delete")) {
					const auto& path = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedAsset);
					AssetMngr::Instance().DeleteAsset(path);
					viewer->selectedAsset = xg::Guid{};
				}*/
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopup("Folder_Popup")) {
				if (ImGui::MenuItem("Create Material")) {
					const auto& folderPath = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedFolder);
					auto wstr = folderPath.wstring();
					std::filesystem::path newPath;
					const auto& tree = AssetMngr::Instance().GetAssetTree();
					size_t i = 0;
					do {
						newPath = wstr + L"\\" + L"new file (" + std::to_wstring(i) + L").mat";
						i++;
					} while (std::filesystem::exists(newPath));
					AssetMngr::Instance().CreateAsset(Material{}, newPath);
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
	});
}
