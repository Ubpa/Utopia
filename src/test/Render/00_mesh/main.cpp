#include <Utopia/Asset/AssetMngr.h>
#include <Utopia/Render/Mesh.h>

#include <iostream>

using namespace Ubpa::Utopia;

int main() {
	std::filesystem::path path = "../assets/models/cube.obj";
	AssetMngr::Instance().ImportAsset(path);
	auto mesh = AssetMngr::Instance().LoadAsset<Mesh>(path);

	for (size_t i = 0; i < mesh->GetPositions().size(); i++)
		std::cout << mesh->GetPositions().at(i) << std::endl;

	AssetMngr::Instance().Clear();
}
