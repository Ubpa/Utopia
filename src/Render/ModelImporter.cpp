#include <Utopia/Render/ModelImporter.h>

#include <Utopia/Render/Mesh.h>

#include <Utopia/Core/Core.h>

#include <Utopia/Render/Components/MeshFilter.h>
#include <Utopia/Render/Components/MeshRenderer.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Texture2D.h>

#include <_deps/tinyobjloader/tiny_obj_loader.h>
#ifdef UBPA_UTOPIA_USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
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
	static AssetImportContext AssimpLoadScene(const std::filesystem::path& path);
	static void AssimpLoadNode(AssetImportContext& ctx, UECS::World& world, UECS::Entity parent, const aiNode* node, const aiScene* scene);
	static std::shared_ptr<Mesh> AssimpLoadMesh(const aiMesh* mesh);
#endif // UBPA_UTOPIA_USE_ASSIMP
}

void ModelImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	UDRefl::Mngr.RegisterType<GPURsrc>();
	UDRefl::Mngr.RegisterType<Mesh>();
	UDRefl::Mngr.AddBases<Mesh, GPURsrc>();
}

AssetImportContext ModelImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	return
#ifdef UBPA_UTOPIA_USE_ASSIMP
	details::AssimpLoadScene(path);
#else
	{};
#endif // UBPA_UTOPIA_USE_ASSIMP
}

void ModelImporter::OnFinish() const {
	auto world = AssetMngr::Instance().GUIDToMainAsset(GetGuid());
	auto worldasset = std::make_shared<WorldAsset>(Serializer::Instance().Serialize(world.AsPtr<UECS::World>()));
	AssetMngr::Instance().ReplaceMainAsset(GetGuid(), UDRefl::SharedObject{ Type_of<WorldAsset>, worldasset });
}

std::vector<std::string> ModelImporterCreator::SupportedExtentions() const {
	return {
#ifdef UBPA_UTOPIA_USE_ASSIMP
		".obj",
		".fbx",
		".gltf",
#endif // UBPA_UTOPIA_USE_ASSIMP
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
AssetImportContext Ubpa::Utopia::details::AssimpLoadScene(const std::filesystem::path& path) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.string(), aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return {};

	AssetImportContext ctx;

	bool is_gltf = path.extension() == L".gltf";

	// load materials
	for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
		Material material;
		material.shader = ShaderMngr::Instance().Get("StdPipeline/Geometry");
		material.properties = material.shader->properties;

		// debug
		// material.properties.at("gRoughnessFactor").value = 0.1f;
		// material.properties.at("gMetalnessMap").value = SharedVar<Texture2D>{ AssetMngr::Instance().LoadAsset<Texture2D>(L"_internal\\textures\\white.png") };

		auto loadTex = [&](aiTextureType type, unsigned int uvidx = 0) -> SharedVar<Texture2D> {
			auto cnt = scene->mMaterials[i]->GetTextureCount(type);
			if (uvidx >= cnt)
				return nullptr;

			aiString str;
			scene->mMaterials[i]->GetTexture(type, uvidx, &str);
			auto texpath = path.parent_path();
			texpath += std::string("\\") + str.C_Str();
			auto tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(AssetMngr::Instance().GetRelativePath(texpath));
			assert(tex2d);
			return SharedVar<Texture2D>{tex2d};
		};

		if (auto tex = loadTex(aiTextureType_BASE_COLOR))
			material.properties.at("gAlbedoMap") = ShaderProperty{ tex };
		else if (auto tex = loadTex(aiTextureType_DIFFUSE))
			material.properties.at("gAlbedoMap") = ShaderProperty{ tex };
		else {
			aiColor3D color;
			if (scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS) {
				if (rgbf dc{ color.r, color.g, color.b }; !dc.is_all_zero())
					material.properties.at("gAlbedoFactor").value = dc;
				else {
					if (scene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn_SUCCESS)
						material.properties.at("gAlbedoFactor").value = rgbf{ color.r, color.g, color.b };
					material.properties.at("gMetalnessMap").value = SharedVar<Texture2D>{ AssetMngr::Instance().LoadAsset<Texture2D>(L"_internal\\textures\\white.png") };
				}
			}
		}

		/*if (auto tex = loadTex(aiTextureType_NORMAL_CAMERA))
			material.properties.at("gNormalMap") = ShaderProperty{ tex };
		else */
		if (auto tex = loadTex(aiTextureType_NORMALS))
			material.properties.at("gNormalMap") = ShaderProperty{ tex };

		if (auto tex = loadTex(aiTextureType_DIFFUSE_ROUGHNESS))
			material.properties.at("gRoughnessMap") = ShaderProperty{ tex };

		if (auto tex = loadTex(aiTextureType_METALNESS))
			material.properties.at("gMetalnessMap") = ShaderProperty{ tex };

		if (is_gltf) {
			if (aiColor3D color; scene->mMaterials[i]->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, color) == aiReturn_SUCCESS)
				material.properties.at("gAlbedoFactor").value = rgbf{ color.r,color.g,color.b };
			if (ai_real factor; scene->mMaterials[i]->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, factor) == aiReturn_SUCCESS)
				material.properties.at("gMetalnessFactor").value = static_cast<float>(factor);
			if (ai_real factor; scene->mMaterials[i]->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, factor) == aiReturn_SUCCESS)
				material.properties.at("gRoughnessFactor").value = static_cast<float>(factor);

			// AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE: aiTextureType_UNKNOWN, 0
			if (int uvIndex; scene->mMaterials[i]->Get(AI_MATKEY_GLTF_TEXTURE_TEXCOORD(aiTextureType_UNKNOWN, 0), uvIndex) == aiReturn_SUCCESS) {
				if (auto tex = loadTex(aiTextureType_UNKNOWN, uvIndex)) {
					auto w = tex->image.GetWidth();
					auto h = tex->image.GetHeight();
					assert(tex->image.GetChannel() >= 3); // 1: roughness, 2: metalness
					Image img_roughness(w, h, 1);
					Image img_metalness(w, h, 1);
					for (size_t y = 0; y < h; y++) {
						for (size_t x = 0; x < w; x++) {
							img_roughness.At(x, y, 0) = tex->image.At(x, y, 1);
							img_metalness.At(x, y, 0) = tex->image.At(x, y, 2);
						}
					}
					auto tex2d_roughness = std::make_shared<Texture2D>(std::move(img_roughness));
					auto tex2d_metalness = std::make_shared<Texture2D>(std::move(img_metalness));
					ctx.AddObject("mat" + std::to_string(i) + ":tex_r", UDRefl::SharedObject{ Type_of<Texture2D>, tex2d_roughness });
					ctx.AddObject("mat" + std::to_string(i) + ":tex_m", UDRefl::SharedObject{ Type_of<Texture2D>, tex2d_metalness });
					material.properties.at("gRoughnessMap").value = SharedVar{ tex2d_roughness };
					material.properties.at("gMetalnessMap").value = SharedVar{ tex2d_metalness };
				}
			}
		}

		ctx.AddObject("mat" + std::to_string(i), UDRefl::SharedObject{ Type_of<Material>, std::make_shared<Material>(std::move(material)) });
	}
	
	// load meshes
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		auto mesh = AssimpLoadMesh(scene->mMeshes[i]);
		ctx.AddObject("mesh" + std::to_string(i), UDRefl::SharedObject{ Type_of<Mesh>, mesh });
	}

	auto world = std::make_shared<UECS::World>();
	world->entityMngr.cmptTraits.Register <
		// transform
		Children,
		LocalToParent,
		LocalToWorld,
		Parent,
		Rotation,
		RotationEuler,
		Scale,
		NonUniformScale,
		Translation,
		WorldToLocal,

		// core
		MeshFilter,
		MeshRenderer,
		Name
	> ();
	AssimpLoadNode(ctx, *world, UECS::Entity::Invalid(), scene->mRootNode, scene);

	// replace it to WorldAsset OnFinish()
	ctx.AddObject(
		"main",
		UDRefl::SharedObject{ Type_of<UECS::World>, world }
	);
	ctx.SetMainObjectID("main");

	return ctx;
}

void Ubpa::Utopia::details::AssimpLoadNode(AssetImportContext& ctx, UECS::World& world, UECS::Entity parent, const aiNode* node, const aiScene* scene) {
	//
	// create entity
	//////////////////

	constexpr std::span<const TypeID> initTypeIds = TypeIDs_of<
		Name,
		Translation,
		Rotation,
		RotationEuler,
		Scale,
		NonUniformScale,
		LocalToWorld,
		WorldToLocal
	>;
	std::vector<TypeID> typeids(initTypeIds.begin(), initTypeIds.end());
	if (node->mNumMeshes == 1) {
		typeids.push_back(TypeID_of<MeshFilter>);
		typeids.push_back(TypeID_of<MeshRenderer>);
	}
	if (node->mNumChildren)
		typeids.push_back(TypeID_of<Children>);
	if (parent.Valid()) {
		typeids.push_back(TypeID_of<Parent>);
		typeids.push_back(TypeID_of<LocalToParent>);
	}

	UECS::Entity e = world.entityMngr.Create(typeids);

	//
	// write data
	///////////////

	if (parent.Valid()) {
		auto* children = world.entityMngr.WriteComponent<Children>(parent);
		children->value.insert(e);
		world.entityMngr.WriteComponent<Parent>(e)->value = parent;
	}

	world.entityMngr.WriteComponent<Name>(e)->value = node->mName.C_Str();

	{ // TRS
		aiVector3t<ai_real> ai_scale;
		aiQuaterniont<ai_real> ai_rotation;
		aiVector3t<ai_real> ai_pos;
		node->mTransformation.Decompose(ai_scale, ai_rotation, ai_pos);
		static_assert(std::is_same_v<ai_real, float>);
		scalef3 s = { ai_scale.x, ai_scale.y, ai_scale.z };
		quatf r = { ai_rotation.x,ai_rotation.y,ai_rotation.z,ai_rotation.w };
		vecf3 t = { ai_pos.x, ai_pos.y, ai_pos.z };
		world.entityMngr.WriteComponent<NonUniformScale>(e)->value = s;
		world.entityMngr.WriteComponent<Rotation>(e)->value = r;
		world.entityMngr.WriteComponent<Translation>(e)->value = t;
		world.entityMngr.WriteComponent<RotationEuler>(e)->value = r.to_euler();
		if (parent.Valid()) {
			transformf l2p{ t,r,s };
			world.entityMngr.WriteComponent<LocalToParent>(e)->value = l2p;
			transformf l2w = world.entityMngr.ReadComponent<LocalToWorld>(e)->value * l2p;
			world.entityMngr.WriteComponent<LocalToWorld>(e)->value = l2w;
			world.entityMngr.WriteComponent<WorldToLocal>(e)->value = l2w.inverse();
		}
		else {
			transformf l2w{ t,r,s };
			world.entityMngr.WriteComponent<LocalToWorld>(e)->value = l2w;
			world.entityMngr.WriteComponent<WorldToLocal>(e)->value = l2w.inverse();
		}
	}

	if (node->mNumMeshes == 1) {
		auto meshFilter = world.entityMngr.WriteComponent<MeshFilter>(e);
		auto mesh = ctx.GetAssets().at("mesh" + std::to_string(node->mMeshes[0]));
		assert(mesh.GetType().Is<Mesh>());
		meshFilter->mesh = mesh;
		auto meshRenderer = world.entityMngr.WriteComponent<MeshRenderer>(e);
		auto material = ctx.GetAssets().at("mat" + std::to_string(scene->mMeshes[node->mMeshes[0]]->mMaterialIndex));
		assert(material.GetType().Is<Material>());
		meshRenderer->materials.push_back(material);
	}
	else if (node->mNumMeshes > 1){
		// use a child "meshes" (with meshes children) to store all meshes 

		if (!world.entityMngr.Have(e, TypeID_of<Children>))
			world.entityMngr.Attach(e, TypeIDs_of<Children>);

		auto meshes = world.entityMngr.Create(TypeIDs_of<
			Name,
			Translation,
			Rotation,
			RotationEuler,
			Scale,
			NonUniformScale,
			LocalToWorld,
			WorldToLocal,
			LocalToParent,
			Parent,
			Children
		>);
		world.entityMngr.WriteComponent<Name>(meshes)->value = "_meshes";
		world.entityMngr.WriteComponent<Parent>(meshes)->value = e;
		world.entityMngr.WriteComponent<LocalToWorld>(meshes)->value = world.entityMngr.ReadComponent<LocalToWorld>(e)->value;
		world.entityMngr.WriteComponent<WorldToLocal>(meshes)->value = world.entityMngr.WriteComponent<LocalToWorld>(meshes)->value.inverse();
		world.entityMngr.WriteComponent<Children>(e)->value.insert(meshes);
		
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			auto mesh = ctx.GetAssets().at("mesh" + std::to_string(node->mMeshes[i]));
			assert(mesh.GetType().Is<Mesh>());
			auto material = ctx.GetAssets().at("mat" + std::to_string(scene->mMeshes[node->mMeshes[i]]->mMaterialIndex));
			assert(material.GetType().Is<Material>());

			auto e_mesh = world.entityMngr.Create(TypeIDs_of<
				Name,
				Translation,
				Rotation,
				RotationEuler,
				Scale,
				NonUniformScale,
				LocalToWorld,
				WorldToLocal,
				LocalToParent,
				Parent,
				MeshFilter,
				MeshRenderer
			>);

			world.entityMngr.WriteComponent<Name>(e_mesh)->value = "mesh" + std::to_string(node->mMeshes[i]);
			world.entityMngr.WriteComponent<Parent>(e_mesh)->value = meshes;
			world.entityMngr.WriteComponent<LocalToWorld>(e_mesh)->value = world.entityMngr.ReadComponent<LocalToWorld>(meshes)->value;
			world.entityMngr.WriteComponent<WorldToLocal>(e_mesh)->value = world.entityMngr.WriteComponent<LocalToWorld>(e_mesh)->value.inverse();
			world.entityMngr.WriteComponent<MeshFilter>(e_mesh)->mesh = mesh;
			world.entityMngr.WriteComponent<MeshRenderer>(e_mesh)->materials.push_back(material);
			world.entityMngr.WriteComponent<Children>(meshes)->value.insert(e_mesh);
		}
	}

	for (unsigned i = 0; i < node->mNumChildren; i++)
		AssimpLoadNode(ctx, world, e, node->mChildren[i], scene);
}

std::shared_ptr<Mesh> Ubpa::Utopia::details::AssimpLoadMesh(const aiMesh* mesh) {
	MeshContext ctx;
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

	return BuildMesh(ctx);
}

#endif // UBPA_UTOPIA_USE_ASSIMP
