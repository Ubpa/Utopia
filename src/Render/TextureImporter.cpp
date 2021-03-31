#include <Utopia/Render/TextureImporter.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>

#include <Utopia/Core/AssetMngr.h>

#include <filesystem>

using namespace Ubpa::Utopia;

void TextureImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	UDRefl::Mngr.RegisterType<TextureImporter::Mode>();
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::Texture2D>("Texture2D");
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::TextureCube>("TextureCube");

	UDRefl::Mngr.RegisterType<Texture2D>();
	UDRefl::Mngr.RegisterType<TextureCube>();

	UDRefl::Mngr.SimpleAddField<&TextureImporter::mode>("mode");
}

AssetImportContext TextureImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	Image img(path.string());

	switch (mode)
	{
	case Ubpa::Utopia::TextureImporter::Mode::Texture2D: {
		Texture2D t;
		t.image = std::move(img);
		auto tex = std::make_shared<Texture2D>(std::move(t));
		ctx.AddObject("main", UDRefl::SharedObject{ Type_of<Texture2D>, tex });
	}
		break;
	case Ubpa::Utopia::TextureImporter::Mode::TextureCube: {
		auto tex = std::make_shared<TextureCube>(std::move(img));
		ctx.AddObject("main", UDRefl::SharedObject{ Type_of<TextureCube>, tex });
	}
		break;
	default:
		assert(false);
		return {};
	}

	ctx.SetMainObjectID("main");

	return ctx;
}

std::string TextureImporter::ReserializeAsset() const {
	auto asset = AssetMngr::Instance().GUIDToMainAsset(GetGuid());
	if (!asset.GetPtr())
		return {};
	
	const auto path = GetFullPath();
	if (mode == Mode::Texture2D) {
		if (!asset.GetType().Is<Texture2D>())
			return {};
		
		asset.As<Texture2D>().image.Save(path.string());
	}
	else if (mode == Mode::TextureCube) {
		if (!asset.GetType().Is<TextureCube>())
			return {};
		if (asset.As<TextureCube>().GetSourceMode() != TextureCube::SourceMode::EquirectangularMap)
			return {};
		asset.As<TextureCube>().GetEquiRectangularMap().Save(path.string());
	}
	else
		assert(false);

	return {};
}

std::vector<std::string> TextureImporterCreator::SupportedExtentions() const {
	return { ".png", ".bmp", ".tga", ".jpg", ".hdr" };
}
