#include "ProjectViewerSystem.h"

#include "../PlayloadType.h"

#include "../Components/ProjectViewer.h"

#include <DustEngine/Transform/Components/Components.h>
#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Render/DX12/RsrcMngrDX12.h>

#include <_deps/imgui/imgui.h>

using namespace Ubpa::DustEngine;

namespace Ubpa::DustEngine::detail {
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

			bool nodeOpen = ImGui::TreeNodeEx(AssetMngr::Instance().LoadAsset(path), nodeFlags, "%s", name.string().c_str());

			if (ImGui::IsItemClicked())
				viewer->selectedFolder = child;

			if (nodeOpen && !isFolderLeaf) {
				ProjectViewerSystemPrintChildren(viewer, child);
				ImGui::TreePop();
			}
		}
	}

	void ProjectViewerSystemPrintFolder(ProjectViewer* viewer) {
		const auto& tree = AssetMngr::Instance().GetAssetTree();
		if (!viewer->selectedFolder.isValid())
			return;
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

		auto fileID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(file);
		auto folderID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(folder);
		auto codeID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(code);
		auto imageID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(image);
		auto materialID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(material);
		auto shaderID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(shader);
		auto hlslID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(hlsl);
		auto sceneID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(scene);
		auto modelID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(model);
		auto texcubeID = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(texcube);

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 button_sz(64, 64);
		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		size_t idx = 0;
		// pass 1 : folder
		for (const auto& child : children) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			if (!std::filesystem::is_directory(path))
				continue;
			ImGui::PushID(idx);
			auto name = path.filename();

			ImGui::BeginGroup();
			{
				ImVec4 tint = viewer->selectedAsset == child ? ImVec4{ 0.65f, 0.65f, 0.65f, 1.f } : ImVec4{ 1,1,1,1 };
				ImGui::ImageButton(ImTextureID(folderID.ptr), button_sz, { 0,0 }, { 1,1 }, 0, style.Colors[ImGuiCol_WindowBg], tint);
				if (ImGui::IsItemClicked()) {
					viewer->selectedAsset = child;
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						viewer->selectedFolder = child;
				}
				auto nameStr = name.string();
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					ImGui::SetDragDropPayload(PlayloadType::GUID, &child, sizeof(xg::Guid));
					ImGui::ImageButton(ImTextureID(folderID.ptr), { 32,32 });
					ImGui::Text(nameStr.c_str());
					ImGui::EndDragDropSource();
				}
				NameShrink(nameStr, 8);
				ImGui::Text(nameStr.c_str());
			}
			ImGui::EndGroup();

			float last_x = ImGui::GetItemRectMax().x;
			float next_x = last_x + style.ItemSpacing.x + button_sz.x;
			if (idx + 1 < children.size() && next_x < window_visible_x2)
				ImGui::SameLine();

			idx++;
			ImGui::PopID();
		}
		// pass 2 : files
		for (const auto& child : children) {
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(child);
			if (std::filesystem::is_directory(path))
				continue;
			ImGui::PushID(idx);
			auto name = path.stem();
			auto ext = path.extension();

			ImGui::BeginGroup();
			{
				ImVec4 tint = viewer->selectedAsset == child ? ImVec4{ 0.65f, 0.65f, 0.65f, 1.f } : ImVec4{ 1,1,1,1 };
				UINT64 id;
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
					Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
						Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
						tex2d
					);
					id = RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(tex2d).ptr;
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
				ImGui::ImageButton(ImTextureID(id), button_sz, { 0,0 }, { 1,1 }, 0, style.Colors[ImGuiCol_WindowBg], tint);
				auto nameStr = name.string();
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					ImGui::SetDragDropPayload(PlayloadType::GUID, &child, sizeof(xg::Guid));
					ImGui::ImageButton(ImTextureID(id), { 32,32 });
					ImGui::Text(nameStr.c_str());
					ImGui::EndDragDropSource();
				}
				if (ImGui::IsItemClicked())
					viewer->selectedAsset = child;
				NameShrink(nameStr, 9);
				ImGui::Text(nameStr.c_str());
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
	GetWorld()->AddCommand([](UECS::World* w) {
		auto viewer = w->entityMngr.GetSingleton<ProjectViewer>();

		ImGui::Begin("Project");
		detail::ProjectViewerSystemPrintChildren(viewer, xg::Guid{});
		ImGui::End();

		ImGui::Begin("Folder");
		detail::ProjectViewerSystemPrintFolder(viewer);
		ImGui::End();
	});
}
