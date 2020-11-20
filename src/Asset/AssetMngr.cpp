#include <Utopia/Asset/AssetMngr.h>

#include "UShaderCompiler/UShaderCompiler.h"

#include <Utopia/Asset/Serializer.h>

#include <Utopia/ScriptSystem/LuaScript.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Core/Image.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Core/TextAsset.h>
#include <Utopia/Core/Scene.h>
#include <Utopia/Core/DefaultAsset.h>

#include <_deps/tinyobjloader/tiny_obj_loader.h>
#ifdef UBPA_DUSTENGINE_USE_ASSIMP
  #include <assimp/Importer.hpp>
  #include <assimp/scene.h>
  #include <assimp/postprocess.h>
#endif // UBPA_DUSTENGINE_USE_ASSIMP

#include <USRefl/USRefl.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <any>
#include <memory>
#include <functional>

using namespace Ubpa::Utopia;

struct AssetMngr::Impl {
	// N asset <-> path <-> guid
	
	struct Asset {
		//xg::Guid localID;
		std::shared_ptr<Object> ptr;
		const std::type_info& GetTypeInfo() const noexcept {
			return typeid(*ptr);
		}
		template<typename T>
		Asset(std::shared_ptr<T> ptr)
			: ptr{ std::static_pointer_cast<Object>(std::move(ptr)) }
		{
			static_assert(std::is_base_of_v<Object, T>);
		}
	};

	std::unordered_map<size_t, std::filesystem::path> assetID2path;
	std::multimap<std::filesystem::path, Asset> path2assert;

	std::map<std::filesystem::path, xg::Guid> path2guid;
	std::unordered_map<xg::Guid, std::filesystem::path> guid2path;

	static std::string LoadText(const std::filesystem::path& path);
	static rapidjson::Document LoadJSON(const std::filesystem::path& metapath);
	struct MeshContext {
		std::vector<pointf3> positions;
		std::vector<rgbf> colors;
		std::vector<normalf> normals;
		std::vector<vecf3> tangents;
		std::vector<uint32_t> indices;
		std::vector<pointf2> uv;
		std::vector<SubMeshDescriptor> submeshes;
	};
	static std::shared_ptr<Mesh> BuildMesh(MeshContext ctx);
	static std::shared_ptr<Mesh> LoadObj(const std::filesystem::path& path);
#ifdef UBPA_DUSTENGINE_USE_ASSIMP
	static std::shared_ptr<Mesh> AssimpLoadMesh(const std::filesystem::path& path);
	static void AssimpLoadNode(AssetMngr::Impl::MeshContext& ctx, const aiNode* node, const aiScene* scene);
	static void AssimpLoadMesh(AssetMngr::Impl::MeshContext& ctx, const aiMesh* mesh, const aiScene* scene);
#endif // UBPA_DUSTENGINE_USE_ASSIMP

	std::map<xg::Guid, std::set<xg::Guid>> assetTree;

	std::filesystem::path root{ L".." };
};

AssetMngr::AssetMngr()
	: pImpl{ new Impl }
{
	Serializer::Instance().RegisterUserTypes<
		Material
	>();
}

AssetMngr::~AssetMngr() {
	delete pImpl;
}

const std::filesystem::path& AssetMngr::GetRootPath() const noexcept {
	return pImpl->root;
}

void AssetMngr::SetRootPath(std::filesystem::path path) {
	pImpl->root = std::move(path);
}

bool AssetMngr::IsSupported(std::string_view ext) const noexcept {
	return
		// [scripts]
		ext == "lua"

		// [models]
		|| ext == "obj"
#ifdef UBPA_DUSTENGINE_USE_ASSIMP
		|| ext == "ply"
#endif // UBPA_DUSTENGINE_USE_ASSIMP

		// [texts]
		|| ext == "txt"
		|| ext == "json"

		// [images]
		|| ext == "png"
		|| ext == "jpg"
		|| ext == "bmp"
		|| ext == "hdr"
		|| ext == "tga"

		// [rendering]
		|| ext == "hlsl"
		|| ext == "shader"
		|| ext == "tex2d"
		|| ext == "texcube"
		|| ext == "mat"
		|| ext == "scene"
		;
}

void AssetMngr::Clear() {
	pImpl->assetID2path.clear();
	pImpl->path2assert.clear();
	pImpl->path2guid.clear();
	pImpl->guid2path.clear();
	pImpl->assetTree.clear();
}

bool AssetMngr::IsImported(const std::filesystem::path& path) const {
	return pImpl->path2guid.find(path) != pImpl->path2guid.end();
}

xg::Guid AssetMngr::AssetPathToGUID(const std::filesystem::path& path) const {
	auto target = pImpl->path2guid.find(path);
	return target == pImpl->path2guid.end() ? xg::Guid{} : target->second;
}

bool AssetMngr::Contains(const Object& obj) const {
	return pImpl->assetID2path.find(obj.GetInstanceID()) != pImpl->assetID2path.end();
}

std::vector<xg::Guid> AssetMngr::FindAssets(const std::wregex& matchRegex) const {
	std::vector<xg::Guid> rst;
	for (const auto& [path, guid] : pImpl->path2guid) {
		if (std::regex_match(path.c_str(), matchRegex))
			rst.push_back(guid);
	}
	return rst;
}

const std::filesystem::path& AssetMngr::GetAssetPath(const Object& obj) const {
	static const std::filesystem::path ERROR;
	auto target = pImpl->assetID2path.find(obj.GetInstanceID());
	return target == pImpl->assetID2path.end() ? ERROR : target->second;
}

const std::map<xg::Guid, std::set<xg::Guid>>& AssetMngr::GetAssetTree() const {
	return pImpl->assetTree;
}

const std::type_info& AssetMngr::GetAssetType(const std::filesystem::path& path) const {
	static const auto& ERROR = typeid(void);
	auto target = pImpl->path2assert.find(path);
	if (target == pImpl->path2assert.end())
		return ERROR;

	return typeid(*target->second.ptr);
}

const std::filesystem::path& AssetMngr::GUIDToAssetPath(const xg::Guid& guid) const {
	static const std::filesystem::path ERROR;
	auto target = pImpl->guid2path.find(guid);
	return target == pImpl->guid2path.end() ? ERROR : target->second;
}

std::shared_ptr<Object> AssetMngr::GUIDToAsset(const xg::Guid& guid) const {
	const auto& path = GUIDToAssetPath(guid);
	if (path.empty())
		return nullptr;

	return pImpl->path2assert.find(path)->second.ptr;
}

std::shared_ptr<Object> AssetMngr::GUIDToAsset(const xg::Guid& guid, const std::type_info& type) const {
	const auto& path = GUIDToAssetPath(guid);
	if (path.empty())
		return nullptr;

	auto iter_begin = pImpl->path2assert.lower_bound(path);
	auto iter_end = pImpl->path2assert.upper_bound(path);
	for (auto iter = iter_begin; iter != iter_end; ++iter) {
		if (iter->second.GetTypeInfo() == type)
			return iter->second.ptr;
	}

	return nullptr;
}

xg::Guid AssetMngr::ImportAsset(const std::filesystem::path& path) {
	assert(!path.empty() && path.is_relative());
	const auto ext = path.extension();

	assert(ext != ".meta");

	auto target = pImpl->path2guid.find(path);
	if (target != pImpl->path2guid.end())
		return target->second;

	assert(std::filesystem::exists(path));
	auto metapath = std::filesystem::path{ path }.concat(".meta");
	bool existMeta = std::filesystem::exists(metapath);
	assert(!existMeta || !std::filesystem::is_directory(metapath));

	xg::Guid guid;
	if (!existMeta) {
		// generate meta file

		guid = xg::newGuid();

		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		writer.StartObject();
		writer.Key("guid");
		writer.String(guid.str());
		writer.EndObject();

		std::ofstream ofs(metapath);
		assert(ofs.is_open());
		ofs << sb.GetString();
		ofs.close();
	}
	else {
		rapidjson::Document doc = Impl::LoadJSON(metapath);
		guid = xg::Guid{ doc["guid"].GetString() };
	}

	auto dirPath = path.parent_path();
	if (dirPath != GetRootPath()) {
		auto dirGuid = ImportAsset(dirPath);
		pImpl->assetTree[dirGuid].insert(guid);
	}
	else
		pImpl->assetTree[xg::Guid{}].insert(guid);

	pImpl->path2guid.emplace(path, guid);
	pImpl->guid2path.emplace(guid, path);

	return guid;
}

void AssetMngr::ImportAssetRecursively(const std::filesystem::path& directory) {
	assert(!directory.has_extension());
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
		auto path = entry.path();
		if(path.extension() == ".meta")
			continue;

		ImportAsset(path);
	}
}

std::shared_ptr<Object> AssetMngr::LoadAsset(const std::filesystem::path& path) {
	ImportAsset(path);
	auto target = pImpl->path2assert.find(path);
	if (target != pImpl->path2assert.end())
		return target->second.ptr;
	const auto ext = path.extension().string();
	if (ext == ".lua") {
		auto str = Impl::LoadText(path);
		auto lua = std::make_shared<LuaScript>(std::move(str));
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ lua });
		pImpl->assetID2path.emplace(lua->GetInstanceID(), path);
		return std::static_pointer_cast<Object>(lua);
	}
	else if (ext == ".obj") {
		auto mesh = Impl::LoadObj(path);
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ mesh });
		pImpl->assetID2path.emplace(mesh->GetInstanceID(), path);
		return mesh;
	}
#ifdef UBPA_DUSTENGINE_USE_ASSIMP
	else if (ext == ".ply") {
		auto mesh = Impl::AssimpLoadMesh(path);
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ mesh });
		pImpl->assetID2path.emplace(mesh->GetInstanceID(), path);
		return mesh;
	}
#endif // UBPA_DUSTENGINE_USE_ASSIMP
	else if (ext == ".hlsl") {
		auto str = Impl::LoadText(path);
		auto hlsl = std::make_shared<HLSLFile>(std::move(str), path.parent_path().string());
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ hlsl });
		pImpl->assetID2path.emplace(hlsl->GetInstanceID(), path);
		return std::static_pointer_cast<Object>(hlsl);
	}
	else if (ext == ".scene") {
		auto str = Impl::LoadText(path);
		auto scene = std::make_shared<Scene>(std::move(str));
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ scene });
		pImpl->assetID2path.emplace(scene->GetInstanceID(), path);
		return std::static_pointer_cast<Object>(scene);
	}
	else if (
		ext == ".txt"
		|| ext == ".json"
	) {
		auto str = Impl::LoadText(path);
		auto text = std::make_shared<TextAsset>(std::move(str));
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ text });
		pImpl->assetID2path.emplace(text->GetInstanceID(), path);
		return text;
	}
	else if (ext == ".shader") {
		auto shaderText = Impl::LoadText(path);
		auto [success, rstShader] = UShaderCompiler::Instance().Compile(shaderText);
		if (!success)
			return nullptr;
		auto shader = std::make_shared<Shader>(std::move(rstShader));

		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ shader });
		pImpl->assetID2path.emplace(shader->GetInstanceID(), path);
		return shader;
	}
	else if (
		ext == ".png"
		|| ext == ".jpg"
		|| ext == ".bmp"
		|| ext == ".hdr"
		|| ext == ".tga"
	) {
		auto img = std::make_shared<Image>(path.string());
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ img });
		pImpl->assetID2path.emplace(img->GetInstanceID(), path);
		return img;
	}
	else if (ext == ".tex2d") {
		auto tex2dJSON = Impl::LoadJSON(path);
		auto guidstr = tex2dJSON["image"].GetString();
		xg::Guid guid{ guidstr };
		auto imgTarget = pImpl->guid2path.find(guid);
		auto tex2d = std::make_shared<Texture2D>();
		tex2d->image = imgTarget != pImpl->guid2path.end() ? LoadAsset<Image>(imgTarget->second) : nullptr;
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ tex2d });
		pImpl->assetID2path.emplace(tex2d->GetInstanceID(), path);
		return tex2d;
	}
	else if (ext == ".texcube") {
		auto texcubeJSON = Impl::LoadJSON(path);
		auto mode = static_cast<TextureCube::SourceMode>(texcubeJSON["mode"].GetInt());
		std::shared_ptr<TextureCube> texcube;
		switch (mode)
		{
		case TextureCube::SourceMode::SixSidedImages: {
			auto imageArr = texcubeJSON["images"].GetArray();
			std::array<std::shared_ptr<const Image>, 6> images;
			for (size_t i = 0; i < 6; i++) {
				auto guidstr = imageArr[rapidjson::SizeType(i)].GetString();
				xg::Guid guid{ guidstr };
				auto imgTarget = pImpl->guid2path.find(guid);
				images[i] = imgTarget != pImpl->guid2path.end() ? LoadAsset<Image>(imgTarget->second) : nullptr;
			}
			texcube = std::make_shared<TextureCube>(images);
			break;
		}
		case TextureCube::SourceMode::EquirectangularMap: {
			std::shared_ptr<const Image> equirectangularMap;
			auto guidstr = texcubeJSON["equirectangularMap"].GetString();
			xg::Guid guid{ guidstr };
			auto imgTarget = pImpl->guid2path.find(guid);
			equirectangularMap = imgTarget != pImpl->guid2path.end() ? LoadAsset<Image>(imgTarget->second) : nullptr;
			texcube = std::make_shared<TextureCube>(equirectangularMap);
			break;
		}
		default:
			assert(false);
			break;
		}
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ texcube });
		pImpl->assetID2path.emplace(texcube->GetInstanceID(), path);
		return texcube;
	}
	else if (ext == ".mat") {
		auto materialJSON = Impl::LoadText(path);
		auto material = std::make_shared<Material>();
		if (!Serializer::Instance().ToUserType(materialJSON, material.get()))
			return nullptr;
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ material });
		pImpl->assetID2path.emplace(material->GetInstanceID(), path);
		return material;
	}
	else {
		auto defaultAsset = std::make_shared<DefaultAsset>();
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ defaultAsset });
		pImpl->assetID2path.emplace(defaultAsset->GetInstanceID(), path);
		return defaultAsset;
	}
}

std::shared_ptr<Object> AssetMngr::LoadAsset(const std::filesystem::path& path, const std::type_info& typeinfo) {
	ImportAsset(path);
	const auto ext = path.extension();
	if (ext == ".lua") {
		if (typeinfo != typeid(LuaScript))
			return nullptr;

		return LoadAsset(path);
	}
	else if (
		ext == ".obj"
		|| ext == ".ply" && IsSupported("ply")
	) {
		if (typeinfo != typeid(Mesh))
			return nullptr;

		return LoadAsset(path);
	}
	else if (ext == ".hlsl") {
		if (typeinfo != typeid(HLSLFile))
			return nullptr;
		return LoadAsset(path);
	}
	else if (ext == ".scene") {
		if (typeinfo != typeid(Scene))
			return nullptr;
		return LoadAsset(path);
	}
	else if (
		ext == ".txt"
		|| ext == ".json"
	) {
		if (typeinfo != typeid(TextAsset))
			return nullptr;
		return LoadAsset(path);
	}
	else if (ext == ".shader") {
		if (typeinfo != typeid(Shader))
			return nullptr;
		return LoadAsset(path);
	}
	else if (
		ext == ".png"
		|| ext == ".jpg"
		|| ext == ".bmp"
		|| ext == ".hdr"
		|| ext == ".tga"
	) {
		if (typeinfo != typeid(Image))
			return nullptr;
		return LoadAsset(path);
	}
	else if (ext == ".tex2d") {
		if (typeinfo != typeid(Texture2D))
			return nullptr;
		return LoadAsset(path);
	}
	else if (ext == ".texcube") {
		if (typeinfo != typeid(TextureCube))
			return nullptr;
		return LoadAsset(path);
	}
	else if (ext == ".mat") {
		if (typeinfo != typeid(Material))
			return nullptr;
		return LoadAsset(path);
	}
	else {
		if (typeinfo != typeid(DefaultAsset))
			return nullptr;
		return LoadAsset(path);
	}
}

bool AssetMngr::CreateAsset(std::shared_ptr<Object> ptr, const std::filesystem::path& path) {
	if (std::filesystem::exists(path))
		return false;
	const auto ext = path.extension().string();
	if (ext == ".shader") {
		auto shader = std::dynamic_pointer_cast<Shader>(ptr);
		if (!shader)
			return false;
		auto guid = AssetPathToGUID(GetAssetPath(*shader->hlslFile));
		
		auto json = Serializer::Instance().ToJSON(shader.get());

		auto dirPath = path.parent_path();
		if (!std::filesystem::is_directory(dirPath))
			std::filesystem::create_directories(dirPath);

		std::ofstream ofs(path);
		assert(ofs.is_open());
		ofs << json;
		ofs.close();

		ImportAsset(path);

		pImpl->path2assert.emplace(path, Impl::Asset{ shader });
		pImpl->assetID2path.emplace(shader->GetInstanceID(), path);
	}
	else if (ext == ".tex2d") {
		auto tex2d = std::dynamic_pointer_cast<Texture2D>(ptr);
		if (!tex2d)
			return false;
		auto guid = AssetPathToGUID(GetAssetPath(*tex2d->image));
		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		writer.StartObject();
		writer.Key("image");
		writer.String(guid.str());
		writer.EndObject();

		auto dirPath = path.parent_path();
		if (!std::filesystem::is_directory(dirPath))
			std::filesystem::create_directories(dirPath);

		std::ofstream ofs(path);
		assert(ofs.is_open());
		ofs << sb.GetString();
		ofs.close();

		ImportAsset(path);

		pImpl->path2assert.emplace(path, Impl::Asset{ tex2d });
		pImpl->assetID2path.emplace(tex2d->GetInstanceID(), path);
	}
	else if (ext == ".mat") {
		auto material = std::dynamic_pointer_cast<Material>(ptr);
		if (!material)
			return false;
		auto materialJSON = Serializer::Instance().ToJSON(material.get());

		auto dirPath = path.parent_path();
		if (!std::filesystem::is_directory(dirPath))
			std::filesystem::create_directories(dirPath);

		std::ofstream ofs(path);
		assert(ofs.is_open());
		ofs << materialJSON;
		ofs.close();

		ImportAsset(path);

		pImpl->path2assert.emplace(path, Impl::Asset{ material });
		pImpl->assetID2path.emplace(material->GetInstanceID(), path);
	}
	else {
		assert("not support" && false);
		return false;
	}

	return true;
}

void AssetMngr::ReserializeAsset(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path))
		return;
	const auto ext = path.extension();
	if (ext == ".mat") {
		auto material = LoadAsset<Material>(path);

		auto materialJSON = Serializer::Instance().ToJSON(material.get());

		auto dirPath = path.parent_path();
		if (!std::filesystem::is_directory(dirPath))
			std::filesystem::create_directories(dirPath);

		std::ofstream ofs(path);
		assert(ofs.is_open());
		ofs << materialJSON;
		ofs.close();
	}
	else
		assert(false && "not support");
}

bool AssetMngr::MoveAsset(const std::filesystem::path& src, const std::filesystem::path& dst) {
	auto iter_begin = pImpl->path2assert.lower_bound(src);
	auto iter_end = pImpl->path2assert.upper_bound(src);
	if (iter_begin == iter_end)
		return false;
	if (std::filesystem::exists(dst))
		return false;
	auto target = pImpl->path2guid.find(src);
	auto guid = target->second;
	if (GetAssetTree().find(guid) != GetAssetTree().end())
		return false; // TODO
	try {
		std::filesystem::rename(src, dst);
		std::filesystem::rename(src.wstring() + L".meta", dst.wstring() + L".meta");
	}
	catch (...) {
		return false;
	}
	std::vector<Impl::Asset> assets;
	for (auto iter = iter_begin; iter != iter_end; ++iter)
		assets.push_back(std::move(iter->second));

	pImpl->guid2path.at(guid) = dst;
	pImpl->path2guid.erase(target);
	pImpl->path2guid.emplace(dst, guid);

	for (const auto& asset : assets)
		pImpl->assetID2path.at(asset.ptr->GetInstanceID()) = dst;

	pImpl->path2assert.erase(src);
	for (auto& asset : assets)
		pImpl->path2assert.emplace(dst, std::move(asset));
	
	return true;
}

//bool AssetMngr::DeleteAsset(const std::filesystem::path& path) {
//	auto iter_begin = pImpl->path2assert.lower_bound(path);
//	auto iter_end = pImpl->path2assert.upper_bound(path);
//	if (iter_begin == iter_end)
//		return false;
//
//	auto target = pImpl->path2guid.find(path);
//	auto guid = target->second;
//	if (GetAssetTree().find(guid) != GetAssetTree().end())
//		return false; // TODO
//
//	if (std::filesystem::remove(path))
//		return false;
//
//	std::filesystem::remove(path.wstring() + L".meta");
//
//	pImpl->guid2path.erase(guid);
//	pImpl->path2guid.erase(target);
//
//	for (auto iter = iter_begin; iter != iter_end; ++iter)
//		pImpl->assetID2path.erase(iter->second.ptr.get());
//
//	pImpl->path2assert.erase(path);
//
//	return true;
//}

// ========================

std::string AssetMngr::Impl::LoadText(const std::filesystem::path& path) {
	std::ifstream t(path);
	std::string str;

	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign(
		std::istreambuf_iterator<char>(t),
		std::istreambuf_iterator<char>()
	);

	return str;
}

rapidjson::Document AssetMngr::Impl::LoadJSON(const std::filesystem::path& metapath) {
	// load guid
	auto str = LoadText(metapath);
	rapidjson::Document doc;
	doc.Parse(str.c_str(), str.size());
	return doc;
}

std::shared_ptr<Mesh> AssetMngr::Impl::BuildMesh(MeshContext ctx) {
	auto mesh = std::make_shared<Mesh>();
	mesh->SetPositions(std::move(ctx.positions));
	mesh->SetColors(std::move(ctx.colors));
	mesh->SetNormals(std::move(ctx.normals));
	mesh->SetUV(std::move(ctx.uv));
	mesh->SetIndices(std::move(ctx.indices));
	mesh->SetSubMeshCount(ctx.submeshes.size());
	for (size_t i = 0; i < ctx.submeshes.size(); i++)
		mesh->SetSubMesh(i, ctx.submeshes[i]);

	// generate normals, uv, tangents
	if (mesh->GetNormals().empty())
		mesh->GenNormals();
	if (mesh->GetUV().empty())
		mesh->GenUV();
	if (mesh->GetTangents().empty())
		mesh->GenTangents();

	mesh->SetToNonEditable();

	return mesh;
}

std::shared_ptr<Mesh> AssetMngr::Impl::LoadObj(const std::filesystem::path& path) {
	tinyobj::ObjReader reader;

	bool success = reader.ParseFromFile(path.string());

	if (!reader.Warning().empty())
		std::cout << reader.Warning() << std::endl;

	if (!reader.Error().empty())
		std::cerr << reader.Error() << std::endl;

	if (!success)
		return nullptr;

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();
	const auto& materials = reader.GetMaterials();

	MeshContext ctx;
	std::map<valu3, size_t> vertexIndexMap;

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		ctx.submeshes.emplace_back(ctx.indices.size(), shapes[s].mesh.num_face_vertices.size() * 3);
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			auto fv = shapes[s].mesh.num_face_vertices[f];
			if (fv != 3)
				return nullptr; // only support triangle mesh

			valu3 face;
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				valu3 key_idx = { idx.vertex_index, idx.normal_index, idx.texcoord_index };
				auto target = vertexIndexMap.find(key_idx);
				if (target != vertexIndexMap.end()) {
					face[v] = static_cast<unsigned>(target->second);
					continue;
				}

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				ctx.positions.emplace_back(vx, vy, vz);
				if (idx.normal_index != -1) {
					tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
					tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
					tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
					ctx.normals.emplace_back(nx, ny, nz);
				}
				if (idx.texcoord_index != -1) {
					tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					ctx.uv.emplace_back(tx, ty);
				}
				// Optional: vertex colors
				tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
				tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
				tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];
				ctx.colors.emplace_back(red, green, blue);

				face[v] = static_cast<unsigned>(ctx.positions.size() - 1);
				vertexIndexMap[key_idx] = ctx.positions.size() - 1;
			}

			index_offset += fv;
			ctx.indices.push_back(face[0]);
			ctx.indices.push_back(face[1]);
			ctx.indices.push_back(face[2]);

			// per-face material
			//shapes[s].mesh.material_ids[f];
		}
	}

	return BuildMesh(std::move(ctx));
}

#ifdef UBPA_DUSTENGINE_USE_ASSIMP
void AssetMngr::Impl::AssimpLoadNode(MeshContext& ctx, const aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		AssimpLoadMesh(ctx, mesh, scene);
	}

	for (unsigned i = 0; i < node->mNumChildren; i++)
		AssimpLoadNode(ctx, node->mChildren[i], scene);
}

void AssetMngr::Impl::AssimpLoadMesh(MeshContext& ctx, const aiMesh* mesh, const aiScene* scene) {
	SubMeshDescriptor desc{ ctx.indices.size(), mesh->mNumFaces * 3 };
	desc.baseVertex = ctx.positions.size();
	ctx.submeshes.push_back(desc);

	for (unsigned i = 0; i < mesh->mNumVertices; i++) {
		pointf3 pos;
		pos[0] = mesh->mVertices[i][0];
		pos[1] = mesh->mVertices[i][1];
		pos[2] = mesh->mVertices[i][2];
		ctx.positions.push_back(pos);

		if (mesh->mColors[0]) {
			rgbf color;
			color[0] = mesh->mColors[0][i][0];
			color[1] = mesh->mColors[0][i][1];
			color[2] = mesh->mColors[0][i][2];
			//color[3] = mesh->mColors[0][i][3];
			ctx.colors.push_back(color);
		}

		if (mesh->mNormals) {
			normalf normal;
			normal[0] = mesh->mNormals[i][0];
			normal[1] = mesh->mNormals[i][1];
			normal[2] = mesh->mNormals[i][2];
			ctx.normals.push_back(normal);
		}

		if (mesh->mTextureCoords[0]) {
			pointf2 texcoord;
			texcoord[0] = mesh->mTextureCoords[0][i][0];
			texcoord[1] = mesh->mTextureCoords[0][i][1];
			ctx.uv.push_back(texcoord);
		}

		if (mesh->mTangents) {
			vecf3 tangent;
			tangent[0] = mesh->mTangents[i][0];
			tangent[1] = mesh->mTangents[i][1];
			tangent[2] = mesh->mTangents[i][2];
			ctx.tangents.push_back(tangent);
		}

		// bitangent
		//vector[0] = mesh->mBitangents[i][0];
		//vector[1] = mesh->mBitangents[i][1];
		//vector[2] = mesh->mBitangents[i][2];
		//vertex.Bitangent = vector;
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		assert(face.mNumIndices == 3);
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			ctx.indices.push_back(face.mIndices[j]);
	}
}

std::shared_ptr<Mesh> AssetMngr::Impl::AssimpLoadMesh(const std::filesystem::path& path) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.string(), aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_Triangulate);
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return nullptr;

	MeshContext ctx;
	AssimpLoadNode(ctx, scene->mRootNode, scene);

	return BuildMesh(std::move(ctx));
}
#endif // UBPA_DUSTENGINE_USE_ASSIMP
