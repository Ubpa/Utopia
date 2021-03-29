#include <Utopia/App/Editor/Systems/ProjectViewerSystem.h>

#include <Utopia/App/Editor/InspectorRegistry.h>

#include <Utopia/App/Editor/Components/ProjectViewer.h>
#include <Utopia/App/Editor/Components/Inspector.h>

#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/misc/cpp/imgui_stdlib.h>

using namespace Ubpa::Utopia;

namespace Ubpa::Utopia::details {
	bool IsDeepestDirectory(const std::filesystem::path& directory) {
		assert(std::filesystem::is_directory(directory));

		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			const auto& path = entry.path();
			if (AssetMngr::Instance().AssetPathToGUID(AssetMngr::Instance().GetRelativePath(path)).isValid())
				return false;
		}

		return true;
	}

	bool IsAncestorDirectory(const std::filesystem::path& x, const std::filesystem::path& y) {
		if (y.empty())
			return false;
		else {
			const auto p = y.parent_path();
			if (p == x)
				return true;
			else
				return IsAncestorDirectory(x, p);
		}
	}

	void NameShrink(std::string& name, size_t maxlength) {
		if (name.size() <= maxlength)
			return;
		name.resize(maxlength);
		name[maxlength - 2] = '.';
		name[maxlength - 1] = '.';
	}

	void ProjectViewerSystemPrintDirectoryTree(ProjectViewer* viewer, const std::filesystem::path& parent) {
		static constexpr ImGuiTreeNodeFlags nodeBaseFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		const auto selectedDirectoryPath = AssetMngr::Instance().GetFullPath(AssetMngr::Instance().GUIDToAssetPath(viewer->selectedAsset));

		for (const auto& directory_entry : std::filesystem::directory_iterator(parent)) {
			if (!directory_entry.is_directory())
				continue;
			
			const auto& fullpath = directory_entry.path();
			const auto relpath = AssetMngr::Instance().GetRelativePath(fullpath);
			auto guid = AssetMngr::Instance().AssetPathToGUID(relpath);

			if (!guid.isValid())
				continue;

			bool isDeepestDirectory = IsDeepestDirectory(fullpath);

			auto name = relpath.stem();

			ImGuiTreeNodeFlags nodeFlags = nodeBaseFlags;
			if (viewer->selectedFolder == guid)
				nodeFlags |= ImGuiTreeNodeFlags_Selected;
			if (isDeepestDirectory)
				nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if (IsAncestorDirectory(relpath, selectedDirectoryPath))
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);

			bool nodeOpen = ImGui::TreeNodeEx(
				(void*)string_hash(fullpath.string()),
				nodeFlags,
				"%s", name.string().c_str()
			);

			if (ImGui::IsItemClicked())
				viewer->selectedFolder = guid;

			if (nodeOpen && !isDeepestDirectory) {
				ProjectViewerSystemPrintDirectoryTree(viewer, fullpath);
				ImGui::TreePop();
			}
		}
	}

	void ProjectViewerSystemPrintFolder(Inspector* inspector, ProjectViewer* viewer) {
		if (!viewer->selectedFolder.isValid())
			return;

		const auto selectedFolderPath = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedFolder);
		const auto selectedFolderFullPath = AssetMngr::Instance().GetFullPath(selectedFolderPath);

		{ // header
			std::vector<std::filesystem::path> paths;
			{
				auto curPath = selectedFolderPath;
				while (!curPath.empty()) {
					paths.emplace_back(curPath);
					curPath = curPath.parent_path();
				}
			}
			for (size_t i = 0; i < paths.size(); i++) {
				size_t idx = paths.size() - 1 - i;
				const auto& path = paths[idx];
				if (idx > 0) {
					if (ImGui::SmallButton(path.stem().string().c_str())) {
						viewer->selectedFolder = AssetMngr::Instance().AssetPathToGUID(path);
						viewer->selectedAsset = {};
					}
					ImGui::SameLine();
					ImGui::Text(">");
					ImGui::SameLine();
				}
				else
					ImGui::Text(path.stem().string().c_str());
			}
		}

		ImGui::Separator();

		auto file = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\file.png)");
		auto folder = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\folder.png)");
		//auto code = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\code.png)");
		//auto image = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\image.png)");
		auto material = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\material.png)");
		auto shader = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\shader.png)");
		auto hlsl = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\hlsl.png)");
		//auto scene = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\scene.png)");
		auto model = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\model.png)");
		//auto texcube = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\FolderViewer\texcube.png)");

		auto fileID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*file);
		auto folderID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*folder);
		//auto codeID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*code);
		//auto imageID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*image);
		auto materialID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*material);
		auto shaderID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*shader);
		auto hlslID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*hlsl);
		//auto sceneID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*scene);
		auto modelID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*model);
		//auto texcubeID = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*texcube);

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 button_sz(64, 64);
		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		UINT idx = 0;

		// folder first
		std::deque<xg::Guid> childQueue;

		for (const auto& entry : std::filesystem::directory_iterator(selectedFolderFullPath)) {
			auto guid = AssetMngr::Instance().AssetPathToGUID(AssetMngr::Instance().GetRelativePath(entry.path()));
			if (!guid.isValid())
				continue;

			if (std::filesystem::is_directory(entry.path()))
				childQueue.push_front(guid);
			else
				childQueue.push_back(guid);
		}

		for (const auto& child : childQueue) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			const bool isDir = std::filesystem::is_directory(AssetMngr::Instance().GetFullPath(path));
			ImGui::PushID(reinterpret_cast<const void* const &>(child.bytes()));
			auto ext = path.extension();
			auto name = path.stem();

			ImGui::BeginGroup();
			{
				ImVec4 tint = viewer->selectedAsset == child ? ImVec4{ 0.65f, 0.65f, 0.65f, 1.f } : ImVec4{ 1,1,1,1 };
				UINT64 id;
				if (!isDir) {
					if (ext == ".png"
						|| ext == ".jpg"
						|| ext == ".bmp"
						|| ext == ".hdr"
						|| ext == ".tga")
					{
						auto tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(path);
						if (tex2d.get()) {
							Ubpa::Utopia::GPURsrcMngrDX12::Instance().RegisterTexture2D(
								*tex2d
							);
							id = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*tex2d).ptr;
						}
						else
							id = fileID.ptr;
					}
					else if (ext == ".mat")
						id = materialID.ptr;
					else if (ext == ".shader")
						id = shaderID.ptr;
					else if (ext == ".hlsl")
						id = hlslID.ptr;
					else if ( ext == ".obj"
						|| ext == ".ply" /*&& AssetMngr::Instance().IsSupported("ply")*/
						) {
						id = modelID.ptr;
					}
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
					InspectorRegistry::Playload::AssetHandle handle{ .guid = child };

					ImGui::SetDragDropPayload(InspectorRegistry::Playload::Asset, &handle, sizeof(InspectorRegistry::Playload::AssetHandle));
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
								+ LR"(\)"
								+ std::filesystem::path(viewer->rename).wstring()
								+ ext.wstring();
							if (!std::filesystem::exists(AssetMngr::Instance().GetFullPath(newpath)) && AssetMngr::Instance().MoveAsset(path, newpath))
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
			if (static_cast<std::size_t>(idx) + 1 < childQueue.size() && next_x < window_visible_x2)
				ImGui::SameLine();

			idx++;
			ImGui::PopID();
		}
	}
}

void ProjectViewerSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.GetWorld()->AddCommand([w = schedule.GetWorld()]() {
		auto viewer = w->entityMngr.WriteSingleton<ProjectViewer>();
		auto inspector = w->entityMngr.WriteSingleton<Inspector>();

		if (!viewer)
			return;

		viewer->hoveredAsset = xg::Guid{};
		
		if (ImGui::Begin("Project"))
			details::ProjectViewerSystemPrintDirectoryTree(viewer, AssetMngr::Instance().GetRootPath());
		ImGui::End();

		if (ImGui::Begin("Folder")) {
			details::ProjectViewerSystemPrintFolder(inspector, viewer);
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
				if (ImGui::MenuItem("Delete")) {
					const auto& path = AssetMngr::Instance().GUIDToAssetPath(viewer->selectedAsset);
					AssetMngr::Instance().DeleteAsset(path);
					viewer->selectedAsset = xg::Guid{};
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopup("Folder_Popup")) {
				if (ImGui::MenuItem("Create Material")) {
					const auto& folderPath = AssetMngr::Instance().GetFullPath(AssetMngr::Instance().GUIDToAssetPath(viewer->selectedFolder));
					auto wstr = folderPath.wstring();
					std::filesystem::path newPath;
					size_t i = 0;
					do {
						newPath = wstr + L"(" + L"new file (" + std::to_wstring(i) + L").mat)";
						i++;
					} while (std::filesystem::exists(newPath));
					AssetMngr::Instance().CreateAsset(std::make_shared<Material>(), AssetMngr::Instance().GetRelativePath(newPath));
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}, 0);
}
