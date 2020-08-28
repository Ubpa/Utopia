#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/ScriptSystem/LuaScript.h>

#include <iostream>

using namespace Ubpa::DustEngine;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	AssetMngr::Instance().ImportAssetRecursively(L"..\\assets");
	auto dir = AssetMngr::Instance().LoadAsset(L"..\\assets\\shaders");
	std::cout << AssetMngr::Instance().GetAssetPath(dir) << std::endl;

	auto shaderGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		std::cout << guid.str() << " : " << AssetMngr::Instance().GUIDToAssetPath(guid) << std::endl;
	}

	auto imgGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(.*\.(png|jpg|bmp))" });
	for (const auto& guid : imgGUIDs) {
		std::cout << guid.str() << " : " << AssetMngr::Instance().GUIDToAssetPath(guid) << std::endl;
	}

	return 0;
}
