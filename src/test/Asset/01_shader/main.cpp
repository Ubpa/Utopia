#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>

#include <iostream>

using namespace Ubpa::DustEngine;

int main() {
	std::filesystem::path path = "../assets/shaders/Default.hlsl";
	AssetMngr::Instance().ImportAsset(path);
	auto hlslFile = AssetMngr::Instance().LoadAsset<HLSLFile>(path);
	std::cout << hlslFile->GetString() << std::endl;

	std::cout << AssetMngr::Instance().Contains(hlslFile) << std::endl;
	auto guid = AssetMngr::Instance().AssetPathToGUID(path);
	std::cout << guid.str() << std::endl;
	std::cout << AssetMngr::Instance().GUIDToAssetPath(guid).string() << std::endl;
	std::cout << AssetMngr::Instance().GetAssetPath(hlslFile).string() << std::endl;

	Shader shader;
	shader.hlslFile = hlslFile;
	shader.vertexName = "vert";
	shader.fragmentName = "frag";
	shader.targetName = "5_0";
	shader.shaderName = "Default";

	AssetMngr::Instance().CreateAsset(&shader, "../assets/shaders/Default.shader");

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
