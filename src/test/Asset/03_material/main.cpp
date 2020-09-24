#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Core/Image.h>
#include <DustEngine/Core/Material.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>

#include <iostream>

using namespace Ubpa::DustEngine;
using namespace Ubpa;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::filesystem::path imgPath = "../assets/textures/test.png";
	std::filesystem::path tex2dPath = "../assets/textures/test.tex2d";
	std::filesystem::path hlslPath = "../assets/shaders/Default.hlsl";
	std::filesystem::path shaderPath = "../assets/shaders/Default.shader";
	std::filesystem::path matPath = "../assets/materials/Default.mat";

	if (!std::filesystem::is_directory("../assets/textures"))
		std::filesystem::create_directories("../assets/textures");
	if (!std::filesystem::is_directory("../assets/shaders"))
		std::filesystem::create_directories("../assets/shaders");
	if (!std::filesystem::is_directory("../assets/materials"))
		std::filesystem::create_directories("../assets/materials");

	{ // create image
		size_t width = 20;
		size_t height = 10;
		Image img(width, height, 3);
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++)
				img.At<rgbf>(i, j) = { i / (float)width, j / (float)height, 0.f };
		}
		img.Save(imgPath.string());
	}

	{ // create Shader, Texture2D and Material
		auto shader = new Shader;
		shader->hlslFile = AssetMngr::Instance().LoadAsset<HLSLFile>(hlslPath);
		ShaderPass pass;
		pass.vertexName = "VS";
		pass.fragmentName = "PS";
		shader->passes.push_back(pass);
		shader->targetName = "5_0";
		shader->shaderName = "Default";
		if (!AssetMngr::Instance().CreateAsset(shader, shaderPath)) {
			delete shader;
			shader = AssetMngr::Instance().LoadAsset<Shader>(shaderPath);
		}

		auto img = AssetMngr::Instance().LoadAsset<Image>(imgPath);

		auto tex2d = new Texture2D;
		tex2d->image = img;
		tex2d->wrapMode = Texture2D::WrapMode::Clamp;
		tex2d->filterMode = Texture2D::FilterMode::Point;

		if (!AssetMngr::Instance().CreateAsset(tex2d, tex2dPath)) {
			delete tex2d;
			tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(tex2dPath);
		}

		auto material = new Material;
		material->shader = shader;
		material->texture2Ds.emplace("gDiffuseMap", tex2d);

		if (!AssetMngr::Instance().CreateAsset(material, matPath))
			delete material;

		AssetMngr::Instance().Clear();
	}

	AssetMngr::Instance().ImportAsset(imgPath);
	AssetMngr::Instance().ImportAsset(tex2dPath);
	AssetMngr::Instance().ImportAsset(hlslPath);
	AssetMngr::Instance().ImportAsset(shaderPath);
	AssetMngr::Instance().ImportAsset(matPath);

	auto mat = AssetMngr::Instance().LoadAsset<Material>(matPath);
	std::cout << mat->shader->shaderName << std::endl;
	for (const auto& [name, tex2D] : mat->texture2Ds) {
		std::cout << name << std::endl;
		for (size_t j = 0; j < tex2D->image->height; j++) {
			for (size_t i = 0; i < tex2D->image->width; i++)
				std::cout << tex2D->image->At<rgbf>(i, j) << std::endl;
		}
	}

	return 0;
}
