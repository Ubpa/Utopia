#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Core/Mesh.h>

#include <iostream>

using namespace Ubpa::DustEngine;

int main() {
	std::filesystem::path path = "../assets/models/cube.obj";
	AssetMngr::Instance().ImportAsset(path);
	auto mesh = AssetMngr::Instance().LoadAsset<Mesh>(path);

	for (size_t i = 0; i < mesh->GetPositions().size(); i++)
		std::cout << mesh->GetPositions().at(i) << std::endl;

	AssetMngr::Instance().Clear();
}
