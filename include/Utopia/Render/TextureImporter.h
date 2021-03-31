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
			TextureCube
		};

		Mode mode{ Mode::Texture2D };

	private:
		friend class TAssetImporterCreator<TextureImporter>;
		static void RegisterToUDRefl();
	};

	class TextureImporterCreator final : public TAssetImporterCreator<TextureImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
