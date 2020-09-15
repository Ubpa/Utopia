#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/ScriptSystem/LuaScript.h>
#include <DustEngine/Core/Mesh.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>
#include <DustEngine/Core/Image.h>
#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Core/TextureCube.h>
#include <DustEngine/Core/Material.h>
#include <DustEngine/Core/TextAsset.h>
#include <DustEngine/Core/Scene.h>
#include <DustEngine/Core/DefaultAsset.h>

#include <DustEngine/_deps/tinyobjloader/tiny_obj_loader.h>

#include <USRefl/USRefl.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <any>
#include <memory>
#include <functional>

using namespace Ubpa::DustEngine;

struct AssetMngr::Impl {
	// asset <-> path <-> guid
	
	struct Asset {
		xg::Guid localID;
		std::unique_ptr<void, std::function<void(void*)>> ptr;
		const std::type_info& typeinfo;

		template<typename T>
		static std::function<void(void*)> DefaultDeletor() noexcept {
			return [](void* p) {
				delete (T*)p;
			};
		}

		template<typename T>
		Asset(T* p)
			:
			ptr{ p, DefaultDeletor<T>() },
			typeinfo{ typeid(std::decay_t<T>) }
		{}

		template<typename T>
		Asset(xg::Guid localID, T* p)
			:
			localID{ localID },
			ptr { p, DefaultDeletor<T>() },
			typeinfo{ typeid(std::decay_t<T>) }
		{}
	};

	std::unordered_map<const void*, std::filesystem::path> asset2path;
	std::multimap<std::filesystem::path, Asset> path2assert;

	std::map<std::filesystem::path, xg::Guid> path2guid;
	std::unordered_map<xg::Guid, std::filesystem::path> guid2path;

	static std::string LoadText(const std::filesystem::path& path);
	static rapidjson::Document LoadJSON(const std::filesystem::path& metapath);
	static Mesh* LoadObj(const std::filesystem::path& path);

	std::map<xg::Guid, std::set<xg::Guid>> assetTree;
};

AssetMngr::AssetMngr()
	: pImpl{ new Impl }
{}

AssetMngr::~AssetMngr() {
	delete pImpl;
}

void AssetMngr::Clear() {
	pImpl->asset2path.clear();
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

bool AssetMngr::Contains(const void* ptr) const {
	return pImpl->asset2path.find(ptr) != pImpl->asset2path.end();
}

std::vector<xg::Guid> AssetMngr::FindAssets(const std::wregex& matchRegex) const {
	std::vector<xg::Guid> rst;
	for (const auto& [path, guid] : pImpl->path2guid) {
		if (std::regex_match(path.c_str(), matchRegex))
			rst.push_back(guid);
	}
	return rst;
}

const std::filesystem::path& AssetMngr::GetAssetPath(const void* ptr) const {
	static const std::filesystem::path ERROR;
	auto target = pImpl->asset2path.find(ptr);
	return target == pImpl->asset2path.end() ? ERROR : target->second;
}

const std::map<xg::Guid, std::set<xg::Guid>>& AssetMngr::GetAssetTree() const {
	return pImpl->assetTree;
}

const std::filesystem::path& AssetMngr::GUIDToAssetPath(const xg::Guid& guid) const {
	static const std::filesystem::path ERROR;
	auto target = pImpl->guid2path.find(guid);
	return target == pImpl->guid2path.end() ? ERROR : target->second;
}

void* AssetMngr::GUIDToAsset(const xg::Guid& guid) const {
	const auto& path = GUIDToAssetPath(guid);
	if (path.empty())
		return nullptr;

	return pImpl->path2assert.find(path)->second.ptr.get();
}

void* AssetMngr::GUIDToAsset(const xg::Guid& guid, const std::type_info& type) const {
	const auto& path = GUIDToAssetPath(guid);
	if (path.empty())
		return nullptr;

	auto iter_begin = pImpl->path2assert.lower_bound(path);
	auto iter_end = pImpl->path2assert.upper_bound(path);
	for (auto iter = iter_begin; iter != iter_end; ++iter) {
		if (iter->second.typeinfo == type)
			return iter->second.ptr.get();
	}

	return nullptr;
}

xg::Guid AssetMngr::ImportAsset(const std::filesystem::path& path) {
	assert(!path.empty() && path.is_relative());
	const auto ext = path.extension();

	assert(ext != ".meta");

	/*if (ext != ".lua"
		&& ext != ".obj"
		&& ext != ".hlsl"
		&& ext != ".scene"
		&& ext != ".txt"
		&& ext != ".json"
		&& ext != ".shader"
		&& ext != ".png"
		&& ext != ".jpg"
		&& ext != ".bmp"
		&& ext != ".hdr"
		&& ext != ".tga"
		&& ext != ".tex2d"
		&& ext != ".mat")
	{
		return {};
	}*/

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
	if (dirPath != L"..\\assets") {
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

void* AssetMngr::LoadAsset(const std::filesystem::path& path) {
	ImportAsset(path);
	const auto ext = path.extension();
	if (ext == ".lua") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto str = Impl::LoadText(path);
		auto lua = new LuaScript(std::move(str));
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ lua });
		pImpl->asset2path.emplace(lua, path);
		return lua;
	}
	else if (ext == ".obj") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();
		auto mesh = Impl::LoadObj(path);
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ mesh });
		pImpl->asset2path.emplace(mesh, path);
		return mesh;
	}
	else if (ext == ".hlsl") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto str = Impl::LoadText(path);
		auto hlsl = new HLSLFile(std::move(str), path.parent_path().string());
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ hlsl });
		pImpl->asset2path.emplace(hlsl, path);
		return hlsl;
	}
	else if (ext == ".scene") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto str = Impl::LoadText(path);
		auto scene = new Scene(std::move(str));
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ scene });
		pImpl->asset2path.emplace(scene, path);
		return scene;
	}
	else if (
		ext == ".txt"
		|| ext == ".json"
	) {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto str = Impl::LoadText(path);
		auto text = new TextAsset(std::move(str));
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ text });
		pImpl->asset2path.emplace(text, path);
		return text;
	}
	else if (ext == ".shader") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto shaderJSON = Impl::LoadJSON(path);
		auto guidstr = shaderJSON["hlslFile"].GetString();
		xg::Guid guid{ guidstr };
		auto hlslTarget = pImpl->guid2path.find(guid);
		auto shader = new Shader;
		shader->hlslFile = hlslTarget != pImpl->guid2path.end() ? LoadAsset<HLSLFile>(hlslTarget->second) : nullptr;
		shader->shaderName = shaderJSON["shaderName"].GetString();
		shader->vertexName = shaderJSON["vertexName"].GetString();
		shader->fragmentName = shaderJSON["fragmentName"].GetString();
		shader->targetName = shaderJSON["targetName"].GetString();
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ shader });
		pImpl->asset2path.emplace(shader, path);
		return shader;
	}
	else if (
		ext == ".png"
		|| ext == ".jpg"
		|| ext == ".bmp"
		|| ext == ".hdr"
		|| ext == ".tga"
	) {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();
		auto img = new Image(path.string());
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ img });
		pImpl->asset2path.emplace(img, path);
		return img;
	}
	else if (ext == ".tex2d") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto tex2dJSON = Impl::LoadJSON(path);
		auto guidstr = tex2dJSON["image"].GetString();
		xg::Guid guid{ guidstr };
		auto imgTarget = pImpl->guid2path.find(guid);
		auto tex2d = new Texture2D;
		tex2d->image = imgTarget != pImpl->guid2path.end() ? LoadAsset<Image>(imgTarget->second) : nullptr;
		tex2d->wrapMode = static_cast<Texture2D::WrapMode>(tex2dJSON["wrapMode"].GetUint());
		tex2d->filterMode = static_cast<Texture2D::FilterMode>(tex2dJSON["filterMode"].GetUint());
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ tex2d });
		pImpl->asset2path.emplace(tex2d, path);
		return tex2d;
	}
	else if (ext == ".texcube") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto texcubeJSON = Impl::LoadJSON(path);
		auto mode = static_cast<TextureCube::SourceMode>(texcubeJSON["mode"].GetInt());
		TextureCube* texcube;
		switch (mode)
		{
		case TextureCube::SourceMode::SixSidedImages: {
			auto imageArr = texcubeJSON["images"].GetArray();
			std::array<const Image*, 6> images;
			for (size_t i = 0; i < 6; i++) {
				auto guidstr = imageArr[rapidjson::SizeType(i)].GetString();
				xg::Guid guid{ guidstr };
				auto imgTarget = pImpl->guid2path.find(guid);
				images[i] = imgTarget != pImpl->guid2path.end() ? LoadAsset<Image>(imgTarget->second) : nullptr;
			}
			texcube = new TextureCube{ images };
			break;
		}
		case TextureCube::SourceMode::EquirectangularMap: {
			const Image* equirectangularMap;
			auto guidstr = texcubeJSON["equirectangularMap"].GetString();
			xg::Guid guid{ guidstr };
			auto imgTarget = pImpl->guid2path.find(guid);
			equirectangularMap = imgTarget != pImpl->guid2path.end() ? LoadAsset<Image>(imgTarget->second) : nullptr;
			texcube = new TextureCube{ equirectangularMap };
			break;
		}
		default:
			assert(false);
			break;
		}
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ texcube });
		pImpl->asset2path.emplace(texcube, path);
		return texcube;
	}
	else if (ext == ".mat") {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto materialJSON = Impl::LoadJSON(path);
		auto guidstr = materialJSON["shader"].GetString();
		xg::Guid guid{ guidstr };
		auto shaderTarget = pImpl->guid2path.find(guid);
		auto material = new Material;
		material->shader = shaderTarget != pImpl->guid2path.end() ? LoadAsset<Shader>(shaderTarget->second) : nullptr;
		const auto& texture2DsJSON = materialJSON["texture2Ds"].GetObject();
		for (const auto& [name, value] : texture2DsJSON) {
			xg::Guid guid{ value.GetString() };
			material->texture2Ds.emplace(name.GetString(), LoadAsset<Texture2D>(GUIDToAssetPath(guid)));
		}
		const auto& textureCubesJSON = materialJSON["textureCubes"].GetObject();
		for (const auto& [name, value] : textureCubesJSON) {
			xg::Guid guid{ value.GetString() };
			material->textureCubes.emplace(name.GetString(), LoadAsset<TextureCube>(GUIDToAssetPath(guid)));
		}
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ material });
		pImpl->asset2path.emplace(material, path);
		return material;
	}
	else {
		auto target = pImpl->path2assert.find(path);
		if (target != pImpl->path2assert.end())
			return target->second.ptr.get();

		auto defaultAsset = new DefaultAsset;
		pImpl->path2assert.emplace_hint(target, path, Impl::Asset{ defaultAsset });
		pImpl->asset2path.emplace(defaultAsset, path);
		return defaultAsset;
	}
}

void* AssetMngr::LoadAsset(const std::filesystem::path& path, const std::type_info& typeinfo) {
	ImportAsset(path);
	const auto ext = path.extension();
	if (ext == ".lua") {
		if (typeinfo != typeid(LuaScript))
			return nullptr;

		return LoadAsset(path);
	}
	else if (ext == ".obj") {
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

bool AssetMngr::CreateAsset(void* ptr, const std::filesystem::path& path) {
	if (std::filesystem::exists(path))
		return false;
	const auto ext = path.extension();
	if (ext == ".shader") {
		auto shader = reinterpret_cast<Shader*>(ptr);
		auto guid = AssetPathToGUID(GetAssetPath(shader->hlslFile));
		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		writer.StartObject();
		writer.Key("hlslFile");
		writer.String(guid.str());
		writer.Key("shaderName");
		writer.String(shader->shaderName);
		writer.Key("vertexName");
		writer.String(shader->vertexName);
		writer.Key("fragmentName");
		writer.String(shader->fragmentName);
		writer.Key("targetName");
		writer.String(shader->targetName);
		writer.EndObject();

		auto dirPath = path.parent_path();
		if (!std::filesystem::is_directory(dirPath))
			std::filesystem::create_directories(dirPath);

		std::ofstream ofs(path);
		assert(ofs.is_open());
		ofs << sb.GetString();
		ofs.close();

		ImportAsset(path);

		pImpl->path2assert.emplace(path, Impl::Asset{ shader });
		pImpl->asset2path.emplace(shader, path);
	}
	else if (ext == ".tex2d") {
		auto tex2d = reinterpret_cast<Texture2D*>(ptr);
		auto guid = AssetPathToGUID(GetAssetPath(tex2d->image));
		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		writer.StartObject();
		writer.Key("image");
		writer.String(guid.str());
		writer.Key("wrapMode");
		writer.Uint(static_cast<unsigned int>(tex2d->wrapMode));
		writer.Key("filterMode");
		writer.Uint(static_cast<unsigned int>(tex2d->filterMode));
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
		pImpl->asset2path.emplace(tex2d, path);
	}
	else if (ext == ".mat") {
		auto material = reinterpret_cast<Material*>(ptr);
		auto guid = AssetPathToGUID(GetAssetPath(material->shader));
		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		writer.StartObject();
		writer.Key("shader");
		writer.String(guid.str());
		writer.Key("texture2Ds");
		writer.StartObject();
		for (const auto& [name, tex2D] : material->texture2Ds) {
			writer.Key(name);
			writer.String(AssetPathToGUID(GetAssetPath(tex2D)));
		}
		writer.EndObject(); // texture2Ds
		writer.Key("textureCubes");
		writer.StartObject();
		for (const auto& [name, texcube] : material->textureCubes) {
			writer.Key(name);
			writer.String(AssetPathToGUID(GetAssetPath(texcube)));
		}
		writer.EndObject(); // textureCubes

		writer.EndObject();

		auto dirPath = path.parent_path();
		if (!std::filesystem::is_directory(dirPath))
			std::filesystem::create_directories(dirPath);

		std::ofstream ofs(path);
		assert(ofs.is_open());
		ofs << sb.GetString();
		ofs.close();

		ImportAsset(path);

		pImpl->path2assert.emplace(path, Impl::Asset{ material });
		pImpl->asset2path.emplace(material, path);
	}
	else {
		assert("not support" && false);
		return false;
	}

	return true;
}

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

Mesh* AssetMngr::Impl::LoadObj(const std::filesystem::path& path) {
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

	std::vector<pointf3> positions;
	std::vector<rgbf> colors;
	std::vector<normalf> normals;
	std::vector<vecf3> tangents;
	std::vector<uint32_t> indices;
	std::vector<pointf2> uv;
	std::vector<SubMeshDescriptor> submeshes;
	std::map<valu3, size_t> vertexIndexMap;

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		submeshes.emplace_back(indices.size(), shapes[s].mesh.num_face_vertices.size() * 3);
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
				positions.emplace_back(vx, vy, vz);
				if (idx.normal_index != -1) {
					tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
					tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
					tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
					normals.emplace_back(nx, ny, nz);
				}
				if (idx.texcoord_index != -1) {
					tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					uv.emplace_back(tx, ty);
				}
				// Optional: vertex colors
				tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
				tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
				tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];
				colors.emplace_back(red, green, blue);

				face[v] = static_cast<unsigned>(positions.size() - 1);
				vertexIndexMap[key_idx] = positions.size() - 1;
			}

			index_offset += fv;
			indices.push_back(face[0]);
			indices.push_back(face[1]);
			indices.push_back(face[2]);

			// per-face material
			//shapes[s].mesh.material_ids[f];
		}
	}

	auto mesh = new Mesh;
	mesh->SetPositions(std::move(positions));
	mesh->SetColors(std::move(colors));
	mesh->SetNormals(std::move(normals));
	mesh->SetUV(std::move(uv));
	mesh->SetIndices(std::move(indices));
	mesh->SetSubMeshCount(submeshes.size());
	for (size_t i = 0; i < submeshes.size(); i++)
		mesh->SetSubMesh(i, submeshes[i]);

	mesh->UpdateVertexBuffer();
	mesh->SetToNonEditable();

	return mesh;
}
