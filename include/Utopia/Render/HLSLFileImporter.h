#pragma once

#include "../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class HLSLFileImporter final : public TAssetImporter<HLSLFileImporter> {
	public:
		using TAssetImporter<HLSLFileImporter>::TAssetImporter;
		
		virtual AssetImportContext ImportAsset() const override;
		static void RegisterToUDRefl();
	};

	class HLSLFileImporterCreator final : public TAssetImporterCreator<HLSLFileImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
