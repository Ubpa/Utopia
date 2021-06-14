#pragma once

#include "../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class TextureImporter final : public TAssetImporter<TextureImporter> {
	public:
		using TAssetImporter<TextureImporter>::TAssetImporter;

		virtual AssetImportContext ImportAsset() const override;
		virtual std::string ReserializeAsset() const;


		enum class Mode {
			Texture2D,
			TextureCube,
			RenderTargetTexture2D
		};

		Mode mode{ Mode::Texture2D };
		bool sRGB{ true }; // the image will be converted to linear color space
	private:
		friend class TAssetImporterCreator<TextureImporter>;
		static void RegisterToUDRefl();
	};

	class TextureImporterCreator final : public TAssetImporterCreator<TextureImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	protected:
		virtual std::shared_ptr<AssetImporter> do_CreateAssetImporter(xg::Guid guid, const std::filesystem::path& path) override;
	};
}
