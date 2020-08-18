#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>

#include <iostream>

using namespace Ubpa::DustEngine;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path path = "../assets/shaders/Default.hlsl";

	if (!std::filesystem::is_directory("../assets/shaders"))
		std::filesystem::create_directories("../assets/shaders");

	AssetMngr::Instance().ImportAsset(path);
	auto hlslFile = AssetMngr::Instance().LoadAsset<HLSLFile>(path);
	std::cout << hlslFile->GetString() << std::endl;

	std::cout << AssetMngr::Instance().Contains(hlslFile) << std::endl;
	auto guid = AssetMngr::Instance().AssetPathToGUID(path);
	std::cout << guid.str() << std::endl;
	std::cout << AssetMngr::Instance().GUIDToAssetPath(guid).string() << std::endl;
	std::cout << AssetMngr::Instance().GetAssetPath(hlslFile).string() << std::endl;

	auto shader = new Shader;
	shader->hlslFile = hlslFile;
	shader->vertexName = "VS";
	shader->fragmentName = "PS";
	shader->targetName = "5_0";
	shader->shaderName = "Default";

	if (!AssetMngr::Instance().CreateAsset(shader, "../assets/shaders/Default.shader"))
		delete shader;

	AssetMngr::Instance().Clear();
	AssetMngr::Instance().ImportAsset(path);
	auto reloadedShader = AssetMngr::Instance().LoadAsset<Shader>("../assets/shaders/Default.shader");
	std::cout << reloadedShader->hlslFile->GetString() << std::endl;
	std::cout << reloadedShader->shaderName << std::endl;
	std::cout << reloadedShader->vertexName << std::endl;
	std::cout << reloadedShader->fragmentName << std::endl;
	std::cout << reloadedShader->targetName << std::endl;

	return 0;
}
