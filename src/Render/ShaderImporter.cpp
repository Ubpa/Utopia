#include <Utopia/Render/ShaderImporter.h>

#include <Utopia/Render/Shader.h>

#include "UShaderCompiler/UShaderCompiler.h"
#include "ShaderImporter/ShaderImporterRegisterImpl.h"

#include <filesystem>

using namespace Ubpa::Utopia;

void ShaderImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();
	details::ShaderImporterRegister_Blend();
	details::ShaderImporterRegister_BlendOp();
	details::ShaderImporterRegister_BlendState();
	details::ShaderImporterRegister_CompareFunc();
	details::ShaderImporterRegister_CullMode();
	details::ShaderImporterRegister_DescriptorRange();
	details::ShaderImporterRegister_FillMode();
	details::ShaderImporterRegister_RenderState();
	details::ShaderImporterRegister_RootConstants();
	details::ShaderImporterRegister_RootDescriptor();
	details::ShaderImporterRegister_RootDescriptorType();
	details::ShaderImporterRegister_Shader();
	details::ShaderImporterRegister_Shader_1();
	details::ShaderImporterRegister_Shader_2();
	details::ShaderImporterRegister_Shader_3();
	details::ShaderImporterRegister_ShaderProperty();
	details::ShaderImporterRegister_StencilOp();
	details::ShaderImporterRegister_StencilState();
}

AssetImportContext ShaderImporter::ImportAsset() const {
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

	auto [success, shader] = UShaderCompiler::Instance().Compile(str);

	if (!success)
		return{};

	std::shared_ptr<Shader> s = std::make_shared<Shader>(std::move(shader));
	ctx.AddObject(name, UDRefl::SharedObject{ Type_of<Shader>, s });
	ctx.SetMainObjectID(name);

	return ctx;
}

std::vector<std::string> ShaderImporterCreator::SupportedExtentions() const {
	return { ".shader" };
}
