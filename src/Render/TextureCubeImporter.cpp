#include <Utopia/Render/TextureCubeImporter.h>
#include <Utopia/Render/TextureCube.h>

#include <Utopia/Core/AssetMngr.h>

#include <filesystem>

using namespace Ubpa::Utopia;

void TextureCubeImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	UDRefl::Mngr.RegisterType<Texture2D>();
	UDRefl::Mngr.RegisterType<TextureCube>();
	UDRefl::Mngr.RegisterType<SharedVar<Texture2D>>();

	UDRefl::Mngr.RegisterType<TextureCubeData>();
	UDRefl::Mngr.SimpleAddField<&TextureCubeData::pos_x>("pos_x");
	UDRefl::Mngr.SimpleAddField<&TextureCubeData::neg_x>("neg_x");
	UDRefl::Mngr.SimpleAddField<&TextureCubeData::pos_y>("pos_y");
	UDRefl::Mngr.SimpleAddField<&TextureCubeData::neg_y>("neg_y");
	UDRefl::Mngr.SimpleAddField<&TextureCubeData::pos_z>("pos_z");
	UDRefl::Mngr.SimpleAddField<&TextureCubeData::neg_z>("neg_z");
}

AssetImportContext TextureCubeImporter::ImportAsset() const {
	AssetImportContext ctx;

	auto path = GetFullPath();
	if (path.empty())
		return {};

	std::string str;
	{ // read file to str
		std::ifstream ifs(path);
		assert(ifs.is_open());

		ifs.seekg(0, std::ios::end);
		str.reserve(ifs.tellg());
		ifs.seekg(0, std::ios::beg);

		str.assign(
			std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>()
		);
	}

	auto data = Serializer::Instance().Deserialize(str).AsShared<TextureCubeData>();
	std::array<Image, 6> images = {
		data->pos_x->image,
		data->neg_x->image,
		data->pos_y->image,
		data->neg_y->image,
		data->pos_z->image,
		data->neg_z->image,
	};

	auto tcube = std::make_shared<TextureCube>(images);

	ctx.AddObject("main", UDRefl::SharedObject{ Type_of<TextureCube>, tcube });
	ctx.AddObject("data", UDRefl::SharedObject{ Type_of<TextureCubeData>, data });

	ctx.SetMainObjectID("main");

	return ctx;
}

std::string TextureCubeImporter::ReserializeAsset() const {
	auto assets = AssetMngr::Instance().GUIDToAllAssets(GetGuid());
	if (assets.empty() || assets.size() > 2)
		return {};
	if (!assets[0].GetType().Is<TextureCube>())
		return {};

	TextureCubeData data;

	if (assets.size() == 1) {
		auto tcube = assets[0].AsShared<TextureCube>();
		const auto& path = AssetMngr::Instance().GetAssetPath(tcube.get());
		// create data
		data.pos_x = std::make_shared<Texture2D>(tcube->GetSixSideImages()[0]);
		data.neg_x = std::make_shared<Texture2D>(tcube->GetSixSideImages()[1]);
		data.pos_y = std::make_shared<Texture2D>(tcube->GetSixSideImages()[2]);
		data.neg_y = std::make_shared<Texture2D>(tcube->GetSixSideImages()[3]);
		data.pos_z = std::make_shared<Texture2D>(tcube->GetSixSideImages()[4]);
		data.neg_z = std::make_shared<Texture2D>(tcube->GetSixSideImages()[5]);

		AssetMngr::Instance().CreateAsset(data.pos_x, AssetMngr::Instance().GetRelativePath(path.wstring() + LR"(.pos_x.png)"));
		AssetMngr::Instance().CreateAsset(data.neg_x, AssetMngr::Instance().GetRelativePath(path.wstring() + LR"(.neg_x.png)"));
		AssetMngr::Instance().CreateAsset(data.pos_y, AssetMngr::Instance().GetRelativePath(path.wstring() + LR"(.pos_y.png)"));
		AssetMngr::Instance().CreateAsset(data.neg_y, AssetMngr::Instance().GetRelativePath(path.wstring() + LR"(.neg_y.png)"));
		AssetMngr::Instance().CreateAsset(data.pos_z, AssetMngr::Instance().GetRelativePath(path.wstring() + LR"(.pos_z.png)"));
		AssetMngr::Instance().CreateAsset(data.neg_z, AssetMngr::Instance().GetRelativePath(path.wstring() + LR"(.neg_z.png)"));
	}
	else {
		if (!assets[1].GetType().Is<TextureCubeData>())
			return {};

		data = assets[1].As<TextureCubeData>();
	}

	return Serializer::Instance().Serialize(&data);
}

std::vector<std::string> TextureCubeImporterCreator::SupportedExtentions() const {
	return { ".texcube" };
}
