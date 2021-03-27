#include <Utopia/Render/TextureImporter.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>

#include <filesystem>

using namespace Ubpa::Utopia;

void TextureImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	UDRefl::Mngr.RegisterType<TextureImporter::Mode>();
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::Texture2D>("Texture2D");
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::TextureCube>("TextureCube");

	UDRefl::Mngr.SimpleAddField<&TextureImporter::mode>("mode");
}

AssetImportContext TextureImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	std::string name = path.stem().string();

	Image img(path.string());

	switch (mode)
	{
	case Ubpa::Utopia::TextureImporter::Mode::Texture2D: {
		Texture2D t;
		t.image = std::move(img);
		auto tex = std::make_shared<Texture2D>(std::move(t));
		ctx.AddObject(name, UDRefl::SharedObject{ Type_of<Texture2D>, tex });
	}
		break;
	case Ubpa::Utopia::TextureImporter::Mode::TextureCube: {
		auto tex = std::make_shared<TextureCube>(std::move(img));
		ctx.AddObject(name, UDRefl::SharedObject{ Type_of<TextureCube>, tex });
	}
		break;
	default:
		assert(false);
		return {};
	}

	ctx.SetMainObjectID(name);

	return ctx;
}

std::vector<std::string> TextureImporterCreator::SupportedExtentions() const {
	return { ".png", ".bmp", ".tga", ".jpg", ".hdr" };
}
