#include <Utopia/Render/TextureImporter.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/RenderTargetTexture2D.h>

#include <Utopia/Core/AssetMngr.h>

#include <filesystem>

using namespace Ubpa::Utopia;

void TextureImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();

	UDRefl::Mngr.RegisterType<TextureImporter::Mode>();
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::Texture2D>("Texture2D");
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::TextureCube>("TextureCube");
	UDRefl::Mngr.SimpleAddField<TextureImporter::Mode::RenderTargetTexture2D>("RenderTargetTexture2D");

	UDRefl::Mngr.SimpleAddField<&TextureImporter::mode>("mode");
	UDRefl::Mngr.SimpleAddField<&TextureImporter::sRGB>("sRGB");
}

AssetImportContext TextureImporter::ImportAsset() const {
	AssetImportContext ctx;
	auto path = GetFullPath();
	if (path.empty())
		return {};

	Image img(path.string());
	if (sRGB) {
		for (size_t y = 0; y < img.GetHeight(); y++) {
			for (size_t x = 0; x < img.GetWidth(); x++) {
				for (size_t c = 0; c < img.GetChannel(); c++) {
					auto& v = img.At(x, y, c);
					v = gamma_to_linear(v);
				}
			}
		}
	}

	switch (mode)
	{
	case Ubpa::Utopia::TextureImporter::Mode::Texture2D: {
		Texture2D tex;
		tex.image = std::move(img);
		auto ptex = std::make_shared<Texture2D>(std::move(tex));
		ctx.AddObject("main", UDRefl::SharedObject{ Type_of<Texture2D>, ptex });
		break;
	}
	case Ubpa::Utopia::TextureImporter::Mode::TextureCube: {
		auto tex = std::make_shared<TextureCube>(std::move(img));
		ctx.AddObject("main", UDRefl::SharedObject{ Type_of<TextureCube>, tex });
		break;
	}
	case Ubpa::Utopia::TextureImporter::Mode::RenderTargetTexture2D: {
		RenderTargetTexture2D tex;
		tex.image = std::move(img);
		auto ptex = std::make_shared<RenderTargetTexture2D>(std::move(tex));
		ctx.AddObject("main", UDRefl::SharedObject{ Type_of<RenderTargetTexture2D>, ptex });
		break;
	}
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

		if (sRGB) {
			Image copied_img = asset.As<Texture2D>().image;
			for (size_t y = 0; y < copied_img.GetHeight(); y++) {
				for (size_t x = 0; x < copied_img.GetWidth(); x++) {
					for (size_t c = 0; c < copied_img.GetChannel(); c++) {
						auto& v = copied_img.At(x, y, c);
						v = linear_to_gamma(v);
					}
				}
			}
			copied_img.Save(path.string());
		}
		else
			asset.As<Texture2D>().image.Save(path.string());
	}
	else if (mode == Mode::RenderTargetTexture2D) {
		if (!asset.GetType().Is<RenderTargetTexture2D>())
			return {};

		if (sRGB) {
			Image copied_img = asset.As<RenderTargetTexture2D>().image;
			for (size_t y = 0; y < copied_img.GetHeight(); y++) {
				for (size_t x = 0; x < copied_img.GetWidth(); x++) {
					for (size_t c = 0; c < copied_img.GetChannel(); c++) {
						auto& v = copied_img.At(x, y, c);
						v = linear_to_gamma(v);
					}
				}
			}
			copied_img.Save(path.string());
		}
		else
			asset.As<RenderTargetTexture2D>().image.Save(path.string());
	}
	else if (mode == Mode::TextureCube) {
		if (!asset.GetType().Is<TextureCube>())
			return {};
		if (asset.As<TextureCube>().GetSourceMode() != TextureCube::SourceMode::EquirectangularMap)
			return {};

		if (sRGB) {
			Image copied_img = asset.As<TextureCube>().GetEquiRectangularMap();
			for (size_t y = 0; y < copied_img.GetHeight(); y++) {
				for (size_t x = 0; x < copied_img.GetWidth(); x++) {
					for (size_t c = 0; c < copied_img.GetChannel(); c++) {
						auto& v = copied_img.At(x, y, c);
						v = linear_to_gamma(v);
					}
				}
			}
			copied_img.Save(path.string());
		}
		else
			asset.As<TextureCube>().GetEquiRectangularMap().Save(path.string());
	}
	else
		assert(false);

	return {};
}

std::vector<std::string> TextureImporterCreator::SupportedExtentions() const {
	return { ".png", ".bmp", ".tga", ".jpg", ".hdr" };
}

std::shared_ptr<AssetImporter> TextureImporterCreator::do_CreateAssetImporter(xg::Guid guid, const std::filesystem::path& path) {
	auto importer = std::make_shared<TextureImporter>(guid);
	if (path.extension() == L".hdr")
		importer->sRGB = false;

	return importer;
}
