#pragma once

#include "../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class ShaderImporter final : public TAssetImporter<ShaderImporter> {
	public:
		using TAssetImporter<ShaderImporter>::TAssetImporter;

		virtual AssetImportContext ImportAsset() const override;
		static void RegisterToUDRefl();
	};

	class ShaderImporterCreator final : public TAssetImporterCreator<ShaderImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
