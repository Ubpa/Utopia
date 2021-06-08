#pragma once

#include "../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class ModelImporter final : public TAssetImporter<ModelImporter> {
	public:
		using TAssetImporter<ModelImporter>::TAssetImporter;
		
		virtual AssetImportContext ImportAsset() const override;
		static void RegisterToUDRefl();
		virtual void OnFinish() const override;
		virtual bool SupportReserializeAsset() const override;
	};

	class ModelImporterCreator final : public TAssetImporterCreator<ModelImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
