#include <Utopia/Render/MaterialImporter.h>

#include "ShaderImporter/ShaderImporterRegisterImpl.h"

#include <Utopia/Render/Material.h>
#include <Utopia/Render/Shader.h>

#include <filesystem>

using namespace Ubpa::Utopia;

void MaterialImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	// Register ShaderProperty and map
	details::ShaderImporterRegister_Shader();

	UDRefl::Mngr.RegisterType<Material>();
	UDRefl::Mngr.AddField<&Material::shader>("shader");
	UDRefl::Mngr.SimpleAddField<&Material::properties>("properties");
}

AssetImportContext MaterialImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	std::string name = path.stem().string();

	std::ifstream ifs(path);
	assert(ifs.is_open());
	std::string str;

	ifs.seekg(0, std::ios::end);
	str.reserve(ifs.tellg());
	ifs.seekg(0, std::ios::beg);

	str.assign(
		std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>()
	);

	auto material = Serializer::Instance().Deserialize(str).AsShared<Material>();

	if (material->shader) {
		for (const auto& [n, prop] : material->shader->properties) {
			auto target = material->properties.find(n);
			if (target != material->properties.end())
				continue;

			material->properties.emplace_hint(target, n, prop);
		}
	}

	ctx.AddObject(name, UDRefl::SharedObject{ Type_of<Material>, material });
	ctx.SetMainObjectID(name);

	return ctx;
}

std::vector<std::string> MaterialImporterCreator::SupportedExtentions() const {
	return { ".mat" };
}
