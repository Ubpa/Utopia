#include <Utopia/Render/MeshImporter.h>

#include <Utopia/Render/Mesh.h>

#include <_deps/tinyobjloader/tiny_obj_loader.h>
#ifdef UBPA_UTOPIA_USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif // UBPA_UTOPIA_USE_ASSIMP

#include <filesystem>

using namespace Ubpa::Utopia;

namespace Ubpa::Utopia::details {
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

#ifdef UBPA_UTOPIA_USE_ASSIMP
	static std::shared_ptr<Mesh> AssimpLoadMesh(const void* buffer, size_t len, const char* ext);
	static void AssimpLoadNode(MeshContext& ctx, const aiNode* node, const aiScene* scene);
	static void AssimpLoadMesh(MeshContext& ctx, const aiMesh* mesh, const aiScene* scene);
#endif // UBPA_UTOPIA_USE_ASSIMP
}

void MeshImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	UDRefl::Mngr.RegisterType<GPURsrc>();
	UDRefl::Mngr.RegisterType<Mesh>();
	UDRefl::Mngr.AddBases<Mesh, GPURsrc>();
}

AssetImportContext MeshImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	auto ext = path.stem().extension();
	
	std::shared_ptr<Mesh> mesh;
	if (ext == LR"(.obj)")
		mesh = details::LoadObj(path);
#ifdef UBPA_UTOPIA_USE_ASSIMP
	else if (ext == LR"(.ply)" || ext == LR"(.FBX)" || ext == LR"(.FBX)") {
		std::ifstream ifs(path, std::ios::in | std::ios::binary);
		assert(ifs.is_open());
		std::vector<char> buffer;

		ifs.seekg(0, std::ios::end);
		buffer.resize(ifs.tellg());
		ifs.seekg(0, std::ios::beg);

		buffer.assign(
			std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>()
		);

		auto s_ext = ext.string();
		auto ai_ext = s_ext.substr(1, s_ext.size() - 1);
		mesh = details::AssimpLoadMesh(buffer.data(), buffer.size(), ai_ext.c_str());
	}
#endif // UBPA_UTOPIA_USE_ASSIMP
	else
		assert(false);
	
	ctx.AddObject("main", UDRefl::SharedObject{ Type_of<Mesh>, mesh });
	ctx.SetMainObjectID("main");

	return ctx;
}

std::vector<std::string> MeshImporterCreator::SupportedExtentions() const {
	return {
		".mesh"
	};
}

std::shared_ptr<Mesh> Ubpa::Utopia::details::BuildMesh(MeshContext ctx) {
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

	mesh->SetReadOnly();

	return mesh;
}

std::shared_ptr<Mesh> Ubpa::Utopia::details::LoadObj(const std::filesystem::path& path) {
	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig config;
	config.vertex_color = false;
	bool success = reader.ParseFromFile(path.string(), config);

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
				//tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
				//tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
				//tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];
				//ctx.colors.emplace_back(red, green, blue);

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

#ifdef UBPA_UTOPIA_USE_ASSIMP
void Ubpa::Utopia::details::AssimpLoadNode(MeshContext& ctx, const aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		AssimpLoadMesh(ctx, mesh, scene);
	}

	for (unsigned i = 0; i < node->mNumChildren; i++)
		AssimpLoadNode(ctx, node->mChildren[i], scene);
}

void Ubpa::Utopia::details::AssimpLoadMesh(MeshContext& ctx, const aiMesh* mesh, const aiScene* scene) {
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

std::shared_ptr<Mesh> Ubpa::Utopia::details::AssimpLoadMesh(const void* buffer, size_t len, const char* ext) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(buffer,len, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_Triangulate,ext);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return nullptr;

	MeshContext ctx;
	AssimpLoadNode(ctx, scene->mRootNode, scene);

	return BuildMesh(std::move(ctx));
}
#endif // UBPA_UTOPIA_USE_ASSIMP
